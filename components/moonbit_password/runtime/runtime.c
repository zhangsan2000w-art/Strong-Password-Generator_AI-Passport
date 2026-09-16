/*
 * Copyright 2026 International Digital Economy Academy
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef __cplusplus
extern "C" {
#endif

#define MOONBIT_BUILD_RUNTIME
#include "moonbit.h"
#include "moonbit_runtime.h"

#ifdef _MSC_VER
#define _Noreturn __declspec(noreturn)
#endif

MOONBIT_EXPORT _Noreturn void moonbit_panic(void);

#ifdef MOONBIT_NATIVE_NO_SYS_HEADER

void *malloc(size_t size);
void free(void *ptr);
void *memset(void *dst, int c, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
void *memmove(void *dst, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

#ifndef NULL
#define NULL ((void *)0)
#endif

#else

#include <stdlib.h>
#include <string.h>

#endif

MOONBIT_EXPORT void *libc_malloc(size_t size) { return malloc(size); }
MOONBIT_EXPORT void libc_free(void *ptr) { free(ptr); }

void moonbit_free_raw(void *obj) {
  MOONBIT_FREE_RAW(obj);
}

// Static GC layout metadata. Regular object headers store a 22-bit word offset
// into this flattened table. Each layout has variable length:
//
//   word 0: payload size in 4-byte words, excluding the object header
//   word 1: number of following reference offsets
//   word 2+: reference-start offsets in 4-byte payload words
MOONBIT_EXPORT const uint32_t *moonbit_layout_table = 0;

MOONBIT_EXPORT void *moonbit_malloc(size_t size) {
  struct moonbit_object *ptr =
      (struct moonbit_object *)MOONBIT_MALLOC_RAW(sizeof(struct moonbit_object) + size);
  Moonbit_init_dynamic_rc(ptr, moonbit_BLOCK_KIND_REGULAR);
  return ptr + 1;
}

// The low five RC bits store the block kind and cycle-collection bookkeeping,
// so changing the logical reference count by one changes the raw word by 32.
// Runtime hot paths can compare the raw word directly and avoid decoding the
// signed 27-bit count:
//
//   logical count 0: raw rc in [0, 31]
//   logical count 1: raw rc in [32, 63]
//   logical count 2+: raw rc >= 64
//   static/immortal: raw rc is negative
//
// These predicates intentionally use signed comparisons for the dynamic/shared
// thresholds so static/immortal objects are excluded without shifting.
#define MOONBIT_RC_COUNT_UNIT ((int32_t)(1u << MOONBIT_RC_COUNT_SHIFT))
#define raw_rc_is_dynamic(rc) ((int32_t)(rc) >= MOONBIT_RC_COUNT_UNIT)
#define raw_rc_is_shared(rc) ((int32_t)(rc) >= (MOONBIT_RC_COUNT_UNIT * 2))

#define MOONBIT_EXTERNAL_PAYLOAD_SIZE_MASK (((uint32_t)1 << 30) - 1)

// Moonbit_header_layout_class is defined in moonbit_runtime.h so the value-enum retain
// fast path can be inlined from runtime_core.c.

#define Moonbit_header_layout_index(header)                                    \
  (((header) >> MOONBIT_REGULAR_LAYOUT_INDEX_SHIFT) &                         \
   MOONBIT_REGULAR_LAYOUT_INDEX_MASK)

#define Moonbit_header_layout(header)                                          \
  (&moonbit_layout_table[Moonbit_header_layout_index(header)])

// An indexed layout entry is [size_in_word; child_count; tagged_child...].
// [child_count] is the number of tagged children (static refs + embedded value
// enums combined). The i-th tagged child is [(payload_word_offset << 1) |
// is_venum].
#define Moonbit_header_child_count(header)                                      \
  (Moonbit_header_layout(header)[1])

#define Moonbit_header_child_tagged(header, i)                                  \
  (Moonbit_header_layout(header)[2 + (i)])

#define Moonbit_make_external_object_header(payload_size)                      \
  (((uint32_t)MOONBIT_REGULAR_LAYOUT_CLASS_EXTERNAL                           \
    << MOONBIT_REGULAR_LAYOUT_CLASS_SHIFT) |                                  \
   ((uint32_t)(payload_size) & MOONBIT_EXTERNAL_PAYLOAD_SIZE_MASK))

static void **ref_slot_at(uint32_t layout_meta, void *value, int32_t ref_index) {
  int32_t offset_in_word =
      (int32_t)(Moonbit_header_child_tagged(layout_meta, ref_index) >> 1);
  return (void **)((uint32_t *)value + offset_in_word);
}


/* ===========================================
   The drop object algorithm

   When dropping a object, we need to recursively `decref` its reference children,
   which may trigger a recursive drop object operation if the RC become zero.
   So it is possible that a very large object may be dropped within a single `drop_object` call.
   We need to ensure stack safety even in case of arbitrarily deep object.
   The `drop_object` algorithm here is `O(1)` stack space + zero heap allocation.
   It achieves this by reusing corpse of parent object as a worklist for to-be-processed children.

   The whole `drop_object` procedure operate in a DFS manner,
   The `drop_object` procedure maintains a global linked array list of `struct drop_object_worklist` below.
   Each node in the list is a parent object being dropped,
   within the node holds to-be-processed children within that parent
   (note that children stored in the worklist may not necessarily need dropping,
    we perform the RC check when popping objects out of the worklist).
   The memory of each node come from the very parent object it represents.

   The worklist node need two `int32_t` plus one pointer for metadata.
   The object header give us the two `int32_t`,
   but we still need to find space for the next node pointer from the payload.
   A parent object may contain exactly `n` reference children,
   in this case, we have no room for storing the next pointer of the worklist.
   The trick here is to delay the initialization of next pointer
   until we encounter the first child in the parent to drop recursively.
   As we descend into that child, the first slot in the parent is guaranteed to be useless,
   so we can safely store the next pointer there.
 * =========================================== */

struct drop_object_worklist {
  // To obtain a natural drop order, we free children from left to right,
  // hence two `int32_t` values are needed.
  int32_t index; // this correspond to the RC field in the original object
  int32_t count; // this correspond to the meta field in the original object

  union {
    struct drop_object_worklist *next;
    void *data[1]; // Children to process in the worklist
  };
};

static inline
uint32_t scan_regular_object(
  void *obj,
  uint32_t const *layout,
  struct drop_object_worklist *wl_node,
  uint32_t count
);

/* Scan the object starting at address `obj`, whose layout is described by `layout`,
   and append all pointer children of `obj` into `wl_node`.
   The current number of children in `wl_node` should be passed in `count`.

   Return the total number of nodes in `wl_node` after writing references in `obj`.

   Note that in practice `obj` is probably part of `wl_node`,
   but it is guaranteed that the used part in `wl_node` never overlap with `obj`. */
static inline
uint32_t scan_regular_object(
  void *obj,
  uint32_t const *layout,
  struct drop_object_worklist *wl_node,
  uint32_t count
) {
  uint32_t child_count = layout[1];
  for (uint32_t i = 0; i < child_count; ++i) {
    uint32_t ref_field_desc = layout[i + 2];
    uint32_t *obj_start = (uint32_t*)obj + (ref_field_desc >> 1);
    if (ref_field_desc & 1u) {
      // value enum
      uint32_t const venum_header = *obj_start;
      // A value enum may contain reference field only in some of its constructors,
      // so it is possible that we will encounter a all-scalar constructor here.
      // In that case, there will be no meaningful layout index in the header,
      // so we need to test the layout class first.
      if (Moonbit_header_layout_class(venum_header) == MOONBIT_REGULAR_LAYOUT_CLASS_INDEXED) {
        uint32_t const *layout = Moonbit_header_layout(venum_header);
        count = scan_regular_object(obj_start, layout, wl_node, count);
      }
    } else {
      // normal reference, store in `wl_node`
      void *child = *(void**)obj_start;
      wl_node->data[count] = child;
      ++count;
    }
  }
  return count;
}

MOONBIT_EXPORT void moonbit_drop_object(void *obj) {
  // The root of worklist, holding remaining objects to drop.
  struct drop_object_worklist *wl_root = 0;
  // The current worklist node being processed.
  // `wl_node` may be either `wl_root`, or a fresh new node not yet linked.
  struct drop_object_worklist *wl_node = 0;
  uint32_t meta;

process_new_object:
    /* The `process_new_object` block:

       1. Analyze and scan a new object to drop in `obj`

       2. If `obj` have any reference child,
          `process_new_object` should compact the layout of `obj`,
          and store all reference children using the layout of `wl_node`.
          After that, `process_new_object` will jump to `find_next_object`,
          to find a new child in `obj` to recursively drop.

          Note than in this case, `wl_node` will not be linked to `wl_root`.
          Because there is no room for `wl_node->next` before we process its content.

       3. If `obj` does not contain any reference child,
          `process_new_object` should jump to `back_to_parent` directly.
    */
    wl_node = (struct drop_object_worklist*)Moonbit_object_header(obj);
    meta = ((struct moonbit_object*)wl_node)->meta;
    switch (Moonbit_object_kind(obj)) {
      case moonbit_BLOCK_KIND_REGULAR: {
        switch (Moonbit_header_layout_class(meta)) {
          case MOONBIT_REGULAR_LAYOUT_CLASS_SCALAR:
            goto back_to_parent;
          case MOONBIT_REGULAR_LAYOUT_CLASS_EXTERNAL: {
            int32_t const payload_size = meta & MOONBIT_EXTERNAL_PAYLOAD_SIZE_MASK;
            void (**addr_of_finalize)(void *) =
                (void (**)(void *))((uint8_t *)obj + payload_size);
            (**addr_of_finalize)(obj);
            goto back_to_parent;
          }
          case MOONBIT_REGULAR_LAYOUT_CLASS_INDEXED: {
            uint32_t const *layout = Moonbit_header_layout(meta);
            if (layout[0] << 2 == layout[1] * sizeof(void*)) {
              // fast path: all children are simple reference,
              // the layout is already compatible with `wl_node`,
              // no need to compact the layout
              wl_node->index = 0;
              wl_node->count = layout[1];
            } else {
              wl_node->index = 0;
              wl_node->count = scan_regular_object(obj, layout, wl_node, 0);
            }
            goto find_next_object;
          }
        }
        break;
      }

      case moonbit_BLOCK_KIND_VAL_ARRAY: goto back_to_parent;
      case moonbit_BLOCK_KIND_REF_ARRAY: {
        // The layout of reference array is compatible with `wl_node`,
        // so no need to do any compaction here.
        wl_node->index = 0;
        wl_node->count = meta;
        goto find_next_object;
      }

      case moonbit_BLOCK_KIND_REF_VALTYPE_ARRAY: {
        struct moonbit_valtype_array_header *varray_header = Moonbit_valtype_header(obj);
        // valtype array has a longer prefix,
        // so we need to adjust the worklist node accordingly
        wl_node = (struct drop_object_worklist*)varray_header;
        uint32_t const *elem_layout = Moonbit_header_layout(varray_header->elem_header);
        wl_node->index = 0;
        uint32_t const elem_size = elem_layout[0];
        uint32_t count = 0;
        for (uint32_t i = 0; i < meta; ++i) {
          uint32_t *elem_start = (uint32_t*)obj + i * elem_size;
          count = scan_regular_object(elem_start, elem_layout, wl_node, count);
        }
        wl_node->count = count;
        goto find_next_object;
      }
    }

back_to_parent:
  /* The `back_to_parent` block should be executed when current `wl_node` is completely processed.
     `back_to_parent` free this node and fetch a new node from the global `wl_root` list.
   */
  MOONBIT_FREE_RAW(wl_node);
  if (!wl_root)
    // Everything dropped, no more work to do
    return;

  /* Note that we does not remove the new node from `wl_root` here,
     because in case we find a new child to drop below in `find_next_object`,
     `wl_node` should be pushed into `wl_root` again, and this pop-and-push is unnecessary.
     We only pop nodes from `wl_root` when a node is completely processed. */
  wl_node = wl_root;

find_next_object:
  /* The `find_next_object` block tries to find the next object to drop recursively
     from current `wl_node`. If a object is found, it will jump to `process_new_object`.
     If no object is found in current `wl_node`,
     `wl_node` will be dropped and removed from the worklist.

     Note that `wl_node` may or may not be `wl_root` when `find_next_object` is executed.
     If the control flow come from `back_to_parent`, `wl_node` will be `wl_root.
     If the control flow come from `process_new_object`,
     `wl_node` will be a fresh node not yet linked to `wl_root`.
   */
  for (uint32_t i = wl_node->index, count = wl_node->count; i < count; ++i) {
    obj = wl_node->data[i];
    if (!obj)
      continue;

    struct moonbit_object *header = Moonbit_object_header(obj);
    int32_t const rc = header->rc;
    if (raw_rc_is_shared(rc)) {
      // This child is still alive, decrease the count and
      // continue with remaining reference children
      header->rc = rc - MOONBIT_RC_COUNT_UNIT;
      continue;
    }

    if (!raw_rc_is_dynamic(rc))
      // static object
      continue;

    // we have found a object to drop recursively
    ++i;
    if (i == count) {
      // last child in parent, no longer need to keep parent in the worklist
      if (wl_node == wl_root)
        wl_root = wl_node->next;
      MOONBIT_FREE_RAW(wl_node);
    } else {
      wl_node->index = i;
      if (wl_node != wl_root) {
        // `wl_node` come from a new object, link it to the worklist.
        // Note that the first object in `wl_node->data` must have been traversed now,
        // so we can safely store the next pointer in `wl_node->next`,
        // which is an alias to `wl_node->data[0]`.
        wl_node->next = wl_root;
        wl_root = wl_node;
      }
    }
    goto process_new_object;
  }

  // No more object to process in `wl_node`, drop it now.
  if (wl_node == wl_root)
    wl_root = wl_node->next;

  goto back_to_parent;
}

MOONBIT_EXPORT void moonbit_incref(void *ptr) {
  struct moonbit_object *header = Moonbit_object_header(ptr);
  int32_t const rc = header->rc;
  if (raw_rc_is_dynamic(rc)) {
    Moonbit_increase_rc_count(header);
  }
}

MOONBIT_EXPORT void moonbit_decref(void *ptr) {
  struct moonbit_object *header = Moonbit_object_header(ptr);
  int32_t const rc = header->rc;
  if (raw_rc_is_shared(rc)) {
    header->rc = rc - MOONBIT_RC_COUNT_UNIT;
  } else if (raw_rc_is_dynamic(rc)) {
    moonbit_drop_object(ptr);
  }
}

// A value enum's first word is a self-describing object header (the active
// constructor's layout). The constructor layout's tagged children are
// base-relative words, so a SCALAR header means "no references" and an INDEXED
// header lists exactly the children live for the current tag. A null reference
// slot is skipped so nullable references are safe.
//
// Two subtleties of the helpers below:
//
//   - They are emitted only for value enums whose type carries references, but a
//     value can currently hold a scalar constructor -- e.g. `A` or `B(Int)` of
//     `enum E { A; B(Int); C(String) }`. Such a value has a SCALAR header whose
//     layout_index is a dummy 0, so the `!= INDEXED` early return is required,
//     not just a fast path: it both skips the no-reference case and avoids
//     indexing moonbit_layout_table with that dummy index.
//
//   - A tagged child with the low bit clear holds a plain heap reference; with
//     the low bit set it is a nested inline value enum (value types nest), so
//     the helpers recurse through the nested enum's own header. Recursion depth
//     is bounded because the typer rejects recursive value types.

// Adjust the rc of the references of a value enum [value] that fills [len] array
// slots, mirroring moonbit_update_ref_valtype_rc but dispatching on the value
// enum's own (tag-dependent) header. len==0 undoes the init's references;
// len>1 adds (len-1) to each so all copies share ownership.
MOONBIT_EXPORT void moonbit_update_ref_enum_valtype_rc(int32_t len, void *value) {
  uint32_t header = *(uint32_t *)value;
  if (Moonbit_header_layout_class(header) != MOONBIT_REGULAR_LAYOUT_CLASS_INDEXED) {
    return;
  }
  uint32_t *payload =
      (uint32_t *)((char *)value );
  uint32_t n = Moonbit_header_child_count(header);
  if (len == 0) {
    for (uint32_t i = 0; i < n; ++i) {
      uint32_t const tagged = Moonbit_header_child_tagged(header, i);
      if (tagged & 1u) {
        moonbit_update_ref_enum_valtype_rc(len, payload + (tagged >> 1));
        continue;
      }
      void *r = *(void **)(payload + (tagged >> 1));
      if (r) {
        moonbit_decref(r);
      }
    }
  } else if (len > 1) {
    for (uint32_t i = 0; i < n; ++i) {
      uint32_t const tagged = Moonbit_header_child_tagged(header, i);
      if (tagged & 1u) {
        // The nested enum's references also gain (len - 1) owners; the update
        // semantics are depth-independent, so recurse with the same [len].
        moonbit_update_ref_enum_valtype_rc(len, payload + (tagged >> 1));
        continue;
      }
      void *r = *(void **)(payload + (tagged >> 1));
      if (r) {
        struct moonbit_object *value_header = Moonbit_object_header(r);
        const int32_t count = Moonbit_rc_count(value_header);
        if (count > 0) {
          Moonbit_set_rc_count(value_header,
                               (uint32_t)count + (uint32_t)len - 1u);
        }
      }
    }
  }
}

// Slow path: walk the current variant's reference slots. Assumes the header is
// INDEXED (the callers inline that cheap check). Kept out of line so scalar-tag
// moves stay call-free once the check is inlined at the use site.
MOONBIT_EXPORT void moonbit_incref_value_enum_loop(void *p) {
  uint32_t header = *(uint32_t *)p;
  uint32_t *payload =
      (uint32_t *)((char *)p );
  uint32_t n = Moonbit_header_child_count(header);
  for (uint32_t i = 0; i < n; i++) {
    uint32_t const tagged = Moonbit_header_child_tagged(header, i);
    if (tagged & 1u) {
      // A nested inline value enum; recurse through its own header (which may
      // hold a scalar constructor, so go through the checked entry point).
      moonbit_incref_value_enum(payload + (tagged >> 1));
      continue;
    }
    void *r = *(void **)(payload + (tagged >> 1));
    if (r) {
      moonbit_incref(r);
    }
  }
}

MOONBIT_EXPORT void moonbit_incref_value_enum(void *p) {
  if (Moonbit_header_layout_class(*(uint32_t *)p) ==
      MOONBIT_REGULAR_LAYOUT_CLASS_INDEXED) {
    moonbit_incref_value_enum_loop(p);
  }
}

MOONBIT_EXPORT void moonbit_decref_value_enum_loop(void *p) {
  uint32_t header = *(uint32_t *)p;
  uint32_t *payload =
      (uint32_t *)((char *)p );
  uint32_t n = Moonbit_header_child_count(header);
  for (uint32_t i = 0; i < n; i++) {
    uint32_t const tagged = Moonbit_header_child_tagged(header, i);
    if (tagged & 1u) {
      // A nested inline value enum; recurse through its own header (which may
      // hold a scalar constructor, so go through the checked entry point).
      moonbit_decref_value_enum(payload + (tagged >> 1));
      continue;
    }
    void *r = *(void **)(payload + (tagged >> 1));
    if (r) {
      moonbit_decref(r);
    }
  }
}

MOONBIT_EXPORT void moonbit_decref_value_enum(void *p) {
  if (Moonbit_header_layout_class(*(uint32_t *)p) ==
      MOONBIT_REGULAR_LAYOUT_CLASS_INDEXED) {
    moonbit_decref_value_enum_loop(p);
  }
}

MOONBIT_EXPORT void *moonbit_malloc_array(enum moonbit_block_kind kind,
                                          int elem_size_shift, int32_t len) {
  if (len < 0)
    moonbit_panic();
  int padding = elem_size_shift < 2 ? 1 : 0;
  size_t alloc_size = ((size_t)len + padding) << elem_size_shift;
  struct moonbit_object *obj = (struct moonbit_object *)MOONBIT_MALLOC_RAW(
    alloc_size + sizeof(struct moonbit_object));
  Moonbit_init_dynamic_rc(obj, kind);
  Moonbit_set_meta(obj, (uint32_t)len);
  return obj + 1;
}

MOONBIT_EXPORT moonbit_string_t moonbit_make_string_raw(int32_t len) {
  moonbit_string_t result = (moonbit_string_t)moonbit_malloc_array(
    moonbit_BLOCK_KIND_VAL_ARRAY,
    1,
    len
  );
  result[len] = 0;
  return result;
}

MOONBIT_EXPORT moonbit_bytes_t moonbit_make_bytes_raw(int32_t len) {
  moonbit_bytes_t result = (moonbit_bytes_t)moonbit_malloc_array(
    moonbit_BLOCK_KIND_VAL_ARRAY,
    0,
    len
  );
  result[len] = 0;
  return result;
}

MOONBIT_EXPORT moonbit_string_t moonbit_make_string(int32_t len,
                                                    uint16_t value) {
  uint16_t *str =
      (uint16_t *)moonbit_malloc_array(moonbit_BLOCK_KIND_VAL_ARRAY, 1, len);
  for (int32_t i = 0; i < len; ++i) {
    str[i] = value;
  }
  str[len] = 0;
  return str;
}

MOONBIT_EXPORT int moonbit_val_array_equal_sized(const void *lhs,
                                                 const void *rhs,
                                                 int32_t elem_size) {
  int32_t const len = Moonbit_array_length(lhs);
  if (len != Moonbit_array_length(rhs))
    return 0;

  return 0 == memcmp(lhs, rhs, len * elem_size);
}

MOONBIT_EXPORT moonbit_string_t moonbit_add_string(moonbit_string_t s1,
                                                   moonbit_string_t s2) {
  int32_t const len1 = Moonbit_array_length(s1);
  int32_t const len2 = Moonbit_array_length(s2);
  moonbit_string_t result = (moonbit_string_t)moonbit_malloc_array(
      moonbit_BLOCK_KIND_VAL_ARRAY, 1, len1 + len2);
  memcpy(result, s1, len1 * 2);
  memcpy(result + len1, s2, len2 * 2);
  result[len1 + len2] = 0;
  return result;
}

MOONBIT_EXPORT moonbit_bytes_t moonbit_make_bytes(int32_t size, int init) {
  moonbit_bytes_t result = (moonbit_bytes_t)moonbit_malloc_array(
      moonbit_BLOCK_KIND_VAL_ARRAY, 0, size);
  memset(result, init, size);
  result[size] = 0;
  return result;
}

MOONBIT_EXPORT void moonbit_unsafe_bytes_blit(moonbit_bytes_t dst,
                                              int32_t dst_start,
                                              moonbit_bytes_t src,
                                              int32_t src_offset, int32_t len) {
  memmove(dst + dst_start, src + src_offset, len);
  moonbit_decref(dst);
  moonbit_decref(src);
}

MOONBIT_EXPORT moonbit_string_t moonbit_unsafe_bytes_sub_string(
    moonbit_bytes_t bytes, int32_t start, int32_t len) {
  int32_t str_len = len / 2 + (len & 1);
  moonbit_string_t str = (moonbit_string_t)moonbit_malloc_array(
      moonbit_BLOCK_KIND_VAL_ARRAY, 1, str_len);
  memcpy(str, bytes + start, len);
  str[str_len] = 0;
  moonbit_decref(bytes);
  return str;
}

MOONBIT_EXPORT int32_t moonbit_unsafe_ref_array_blit(void *dst,
                                                     int32_t dst_offset,
                                                     void *src,
                                                     int32_t src_offset,
                                                     int32_t len) {
  void **dst_ptrs = (void **)dst;
  void **src_ptrs = (void **)src;
  int32_t dst_end = dst_offset + len;
  int32_t src_end = src_offset + len;
  int32_t dst_len = Moonbit_array_length(dst_ptrs);
  int32_t src_len = Moonbit_array_length(src_ptrs);
  if (dst_offset < 0 || dst_end > dst_len || src_offset < 0 ||
      src_end > src_len) {
    moonbit_panic();
  }
  struct moonbit_object *src_header = Moonbit_object_header(src_ptrs);
  int32_t const src_rc = src_header->rc;
  if (raw_rc_is_shared(src_rc)) {
    src_header->rc = src_rc - MOONBIT_RC_COUNT_UNIT;
  } else if (raw_rc_is_dynamic(src_rc)) {
    for (int32_t i = 0; i < src_offset; ++i) {
      if (src_ptrs[i])
        moonbit_decref(src_ptrs[i]);
    }
    for (int32_t i = src_end; i < src_len; ++i) {
      if (src_ptrs[i])
        moonbit_decref(src_ptrs[i]);
    }
    for (int32_t i = dst_offset; i < dst_end; ++i) {
      if (dst_ptrs[i])
        moonbit_decref(dst_ptrs[i]);
    }
    // since `src` is unique, it must not overlap with `dst`
    memcpy(dst_ptrs + dst_offset, src_ptrs + src_offset, len * sizeof(void *));
    moonbit_free(src_ptrs);
    moonbit_decref(dst_ptrs);
    return 0;
  }
  for (int32_t i = src_offset; i < src_end; ++i) {
    if (src_ptrs[i])
      moonbit_incref(src_ptrs[i]);
  }
  for (int32_t i = dst_offset; i < dst_end; ++i) {
    if (dst_ptrs[i])
      moonbit_decref(dst_ptrs[i]);
  }
  memmove(dst_ptrs + dst_offset, src_ptrs + src_offset, len * sizeof(void *));
  moonbit_decref(dst_ptrs);
  return 0;
}

MOONBIT_EXPORT int32_t moonbit_unsafe_val_array_blit(uint8_t *dst,
                                                     int32_t dst_offset,
                                                     uint8_t *src,
                                                     int32_t src_offset,
                                                     int32_t len,
                                                     int32_t elem_size) {
  int32_t dst_end = dst_offset + len;
  int32_t src_end = src_offset + len;
  int32_t dst_len = Moonbit_array_length(dst);
  int32_t src_len = Moonbit_array_length(src);
  if (dst_offset < 0 || dst_end > dst_len || src_offset < 0 ||
      src_end > src_len) {
    moonbit_panic();
  }
  memmove(dst + dst_offset * elem_size, src + src_offset * elem_size,
          len * elem_size);
  moonbit_decref(src);
  moonbit_decref(dst);
  return 0;
}

MOONBIT_EXPORT void **moonbit_make_ref_array_with_blit(
    int32_t allocate_len, void *value, void *src, int32_t src_offset,
    int32_t dst_offset, int32_t len) {
  void **dst_ptrs = moonbit_make_ref_array_raw(allocate_len);
  void **src_ptrs = (void **)src;
  int32_t dst_end = dst_offset + len;
  int32_t src_end = src_offset + len;
  int32_t src_len = Moonbit_array_length(src_ptrs);
  int32_t init_slots = allocate_len - len;
  if (value) {
    if (init_slots == 0) {
      moonbit_decref(value);
    } else {
      struct moonbit_object *value_header = Moonbit_object_header(value);
      int32_t const count = Moonbit_rc_count(value_header);
      if (count > 0 && init_slots > 1) {
        Moonbit_set_rc_count(value_header,
                             (uint32_t)count + (uint32_t)init_slots - 1u);
      }
    }
  }
  for (int32_t i = 0; i < dst_offset; ++i) {
    dst_ptrs[i] = value;
  }
  for (int32_t i = dst_end; i < allocate_len; ++i) {
    dst_ptrs[i] = value;
  }
  struct moonbit_object *src_header = Moonbit_object_header(src_ptrs);
  int32_t const src_rc = src_header->rc;
  if (raw_rc_is_shared(src_rc)) {
    src_header->rc = src_rc - MOONBIT_RC_COUNT_UNIT;
  } else if (raw_rc_is_dynamic(src_rc)) {
    for (int32_t i = 0; i < src_offset; ++i) {
      if (src_ptrs[i])
        moonbit_decref(src_ptrs[i]);
    }
    for (int32_t i = src_end; i < src_len; ++i) {
      if (src_ptrs[i])
        moonbit_decref(src_ptrs[i]);
    }
    memcpy(dst_ptrs + dst_offset, src_ptrs + src_offset, len * sizeof(void *));
    moonbit_free(src_ptrs);
    return dst_ptrs;
  }
  for (int32_t i = src_offset; i < src_end; ++i) {
    if (src_ptrs[i])
      moonbit_incref(src_ptrs[i]);
  }
  memcpy(dst_ptrs + dst_offset, src_ptrs + src_offset, len * sizeof(void *));
  return dst_ptrs;
}

MOONBIT_EXPORT int32_t *moonbit_make_int32_array_raw(int32_t len) {
  if (len == 0)
    return moonbit_empty_int32_array;
  return (int32_t *)moonbit_malloc_array(moonbit_BLOCK_KIND_VAL_ARRAY, 2, len);
}

MOONBIT_EXPORT int32_t *moonbit_make_int32_array(int32_t len, int32_t value) {
  int32_t *arr = moonbit_make_int32_array_raw(len);
  for (int32_t i = 0; i < len; ++i) {
    arr[i] = value;
  }
  return arr;
}

MOONBIT_EXPORT void **moonbit_make_ref_array_raw(int32_t len) {
  if (len == 0)
    return moonbit_empty_ref_array;
  return (void **)moonbit_malloc_array(moonbit_BLOCK_KIND_REF_ARRAY,
                                       (sizeof(void *) >> 2) + 1, len);
}

MOONBIT_EXPORT void **moonbit_make_ref_array(int32_t len, void *value) {
  if (len == 0) {
    if (value)
      moonbit_decref(value);
    return moonbit_empty_ref_array;
  }

  void **arr = moonbit_make_ref_array_raw(len);

  if (value) {
    struct moonbit_object *value_header = Moonbit_object_header(value);
    const int32_t count = Moonbit_rc_count(value_header);
    if (count > 0 && len > 1) {
      Moonbit_set_rc_count(value_header, (uint32_t)count + (uint32_t)len - 1u);
    }
  }
  for (int32_t i = 0; i < len; ++i) {
    arr[i] = value;
  }
  return arr;
}

MOONBIT_EXPORT void **moonbit_make_extern_ref_array_raw(int32_t len) {
  if (len == 0)
    return moonbit_empty_extern_ref_array;
  return (void **)moonbit_malloc_array(moonbit_BLOCK_KIND_VAL_ARRAY,
                                       (sizeof(void *) >> 2) + 1, len);
}

MOONBIT_EXPORT void **moonbit_make_extern_ref_array(int32_t len, void *value) {
  void **arr = moonbit_make_extern_ref_array_raw(len);
  for (int32_t i = 0; i < len; ++i) {
    arr[i] = value;
  }
  return arr;
}

MOONBIT_EXPORT int64_t *moonbit_make_int64_array_raw(int32_t len) {
  if (len == 0)
    return moonbit_empty_int64_array;
  return (int64_t *)moonbit_malloc_array(moonbit_BLOCK_KIND_VAL_ARRAY, 3, len);
}

MOONBIT_EXPORT int64_t *moonbit_make_int64_array(int32_t len, int64_t value) {
  int64_t *arr = moonbit_make_int64_array_raw(len);
  for (int32_t i = 0; i < len; ++i) {
    arr[i] = value;
  }
  return arr;
}

MOONBIT_EXPORT double *moonbit_make_double_array_raw(int32_t len) {
  if (len == 0)
    return moonbit_empty_double_array;
  return (double *)moonbit_malloc_array(moonbit_BLOCK_KIND_VAL_ARRAY, 3, len);
}

MOONBIT_EXPORT double *moonbit_make_double_array(int32_t len, double value) {
  double *arr = moonbit_make_double_array_raw(len);
  for (int32_t i = 0; i < len; ++i) {
    arr[i] = value;
  }
  return arr;
}

MOONBIT_EXPORT float *moonbit_make_float_array_raw(int32_t len) {
  if (len == 0)
    return moonbit_empty_float_array;
  return (float *)moonbit_malloc_array(moonbit_BLOCK_KIND_VAL_ARRAY, 2, len);
}

MOONBIT_EXPORT float *moonbit_make_float_array(int32_t len, float value) {
  float *arr = moonbit_make_float_array_raw(len);
  for (int32_t i = 0; i < len; ++i) {
    arr[i] = value;
  }
  return arr;
}

static struct {
  int32_t rc;
  uint32_t meta;
  void *data[];
} moonbit_empty_scalar_valtype_array_object = {
    Moonbit_make_static_rc(moonbit_BLOCK_KIND_VAL_ARRAY),
    0};

MOONBIT_EXPORT void *const moonbit_empty_scalar_valtype_array =
    moonbit_empty_scalar_valtype_array_object.data;

static struct {
  int32_t rc;
  uint32_t meta;
  moonbit_v128_storage_t data[];
} moonbit_empty_v128_array_object = {
    Moonbit_make_static_rc(moonbit_BLOCK_KIND_VAL_ARRAY),
    0};

MOONBIT_EXPORT moonbit_v128_storage_t *const moonbit_empty_v128_array =
    moonbit_empty_v128_array_object.data;

MOONBIT_EXPORT void *moonbit_make_scalar_valtype_array(int32_t len,
                                                       size_t valtype_size,
                                                       void *init) {
  void *array = moonbit_make_scalar_valtype_array_raw(len, valtype_size);
  if (array) {
    for (int32_t i = 0; i < len; ++i) {
      memcpy((uint8_t *)array + i * valtype_size, init, valtype_size);
    }
  }
  return array;
}

MOONBIT_EXPORT void *
moonbit_make_scalar_valtype_array_raw(int32_t len, size_t valtype_size) {
  if (len < 0)
    moonbit_panic();
  if (len == 0)
    return moonbit_empty_scalar_valtype_array;
  // All-scalar value-type arrays have no references to scan, so they use the
  // ordinary all-scalar array representation and do not carry the extra
  // value-type-array header.
  struct moonbit_object *obj = (struct moonbit_object *)MOONBIT_MALLOC_RAW(
      len * valtype_size + sizeof(struct moonbit_object));
  Moonbit_init_dynamic_rc(obj, moonbit_BLOCK_KIND_VAL_ARRAY);
  Moonbit_set_meta(obj, (uint32_t)len);
  return (void *)(obj + 1);
}

MOONBIT_EXPORT moonbit_v128_storage_t *
moonbit_make_v128_array(int32_t len, uint64_t lo, uint64_t hi) {
  moonbit_v128_storage_t *array = moonbit_make_v128_array_raw(len);
  if (array) {
    for (int32_t i = 0; i < len; ++i) {
      array[i].lo = lo;
      array[i].hi = hi;
    }
  }
  return array;
}

MOONBIT_EXPORT moonbit_v128_storage_t *moonbit_make_v128_array_raw(int32_t len) {
  if (len < 0)
    moonbit_panic();
  if (len == 0)
    return moonbit_empty_v128_array;
  struct moonbit_object *obj = (struct moonbit_object *)MOONBIT_MALLOC_RAW(
      (size_t)len * sizeof(moonbit_v128_storage_t) +
      sizeof(struct moonbit_object));
  Moonbit_init_dynamic_rc(obj, moonbit_BLOCK_KIND_VAL_ARRAY);
  Moonbit_set_meta(obj, (uint32_t)len);
  return (moonbit_v128_storage_t *)(obj + 1);
}

static struct {
  int32_t rc;
  uint32_t meta;
  void *data[];
} moonbit_empty_ref_valtype_array_object = {
    Moonbit_make_static_rc(moonbit_BLOCK_KIND_REF_VALTYPE_ARRAY),
    0};

MOONBIT_EXPORT void *const moonbit_empty_ref_valtype_array =
    moonbit_empty_ref_valtype_array_object.data;

MOONBIT_EXPORT void moonbit_update_ref_valtype_rc(int32_t len, void *value,
                                                  uint32_t layout_meta) {
  // Static references of a value-struct element. Embedded value enums carry
  // their own tag-dependent layouts, so recurse through their headers.
  uint32_t n = Moonbit_header_child_count(layout_meta);
  if (len == 0) {
    for (uint32_t i = 0; i < n; ++i) {
      uint32_t const tagged = Moonbit_header_child_tagged(layout_meta, i);
      if (tagged & 1u) {
        moonbit_update_ref_enum_valtype_rc(
            len, (uint32_t *)value + (tagged >> 1));
        continue;
      }
      void **ptrs = ref_slot_at(layout_meta, value, (int32_t)i);
      if (*ptrs)
        moonbit_decref(*ptrs);
    }
  } else if (len > 1) {
    for (uint32_t i = 0; i < n; ++i) {
      uint32_t const tagged = Moonbit_header_child_tagged(layout_meta, i);
      if (tagged & 1u) {
        moonbit_update_ref_enum_valtype_rc(
            len, (uint32_t *)value + (tagged >> 1));
        continue;
      }
      void **ptrs = ref_slot_at(layout_meta, value, (int32_t)i);
      if (*ptrs) {
        struct moonbit_object *value_header = Moonbit_object_header(*ptrs);
        const int32_t count = Moonbit_rc_count(value_header);
        if (count > 0) {
          Moonbit_set_rc_count(value_header,
                               (uint32_t)count + (uint32_t)len - 1u);
        }
      }
    }
  }
}

MOONBIT_EXPORT void *moonbit_make_ref_valtype_array(int32_t len,
                                                    size_t valtype_size,
                                                    uint32_t layout_meta,
                                                    void *init) {
  void *array =
      moonbit_make_ref_valtype_array_raw(len, valtype_size, layout_meta);
  if (array) {
    for (int32_t i = 0; i < len; ++i) {
      memcpy((uint8_t *)array + i * valtype_size, init, valtype_size);
    }
  }
  return array;
}

MOONBIT_EXPORT void *moonbit_make_ref_valtype_array_raw(int32_t len,
                                                        size_t valtype_size,
                                                        uint32_t layout_meta) {
  if (len < 0)
    moonbit_panic();
  if (len == 0)
    return moonbit_empty_ref_valtype_array;
  // the extra header is 4-byte but we allocate extra 8-byte for better
  // alignment
  size_t const total_size =
      len * valtype_size + sizeof(struct moonbit_object) + sizeof(void *);
  struct moonbit_object *obj = (struct moonbit_object *)MOONBIT_MALLOC_RAW(total_size);
  *(uint64_t *)obj = (uint64_t)layout_meta;
  obj = (struct moonbit_object *)(((uint64_t *)obj) + 1);
  Moonbit_init_dynamic_rc(obj, moonbit_BLOCK_KIND_REF_VALTYPE_ARRAY);
  Moonbit_set_meta(obj, (uint32_t)len);
  return (void *)(obj + 1);
}

MOONBIT_EXPORT void *moonbit_make_external_object(void (*finalize)(void *self),
                                                  uint32_t payload_size) {
  void *result = moonbit_malloc(sizeof(void (*)(void *)) + payload_size);
  Moonbit_set_meta(Moonbit_object_header(result),
                   Moonbit_make_external_object_header(payload_size));
  void (**addr_of_finalize)(void *) =
      (void (**)(void *))((uint8_t *)result + payload_size);
  *addr_of_finalize = finalize;
  return result;
}

static struct {
  int32_t rc;
  uint32_t meta;
  uint8_t data[];
} moonbit_empty_int8_array_object = {
    Moonbit_make_static_rc(moonbit_BLOCK_KIND_VAL_ARRAY),
    0};

MOONBIT_EXPORT uint8_t *const moonbit_empty_int8_array =
    moonbit_empty_int8_array_object.data;

static struct {
  int32_t rc;
  uint32_t meta;
  uint16_t data[];
} moonbit_empty_int16_array_object = {
    Moonbit_make_static_rc(moonbit_BLOCK_KIND_VAL_ARRAY),
    0};

MOONBIT_EXPORT uint16_t *const moonbit_empty_int16_array =
    moonbit_empty_int16_array_object.data;

static struct {
  int32_t rc;
  uint32_t meta;
  int32_t data[];
} moonbit_empty_int32_array_object = {
    Moonbit_make_static_rc(moonbit_BLOCK_KIND_VAL_ARRAY),
    0};

MOONBIT_EXPORT int32_t *const moonbit_empty_int32_array =
    moonbit_empty_int32_array_object.data;

static struct {
  int32_t rc;
  uint32_t meta;
  int64_t data[];
} moonbit_empty_int64_array_object = {
    Moonbit_make_static_rc(moonbit_BLOCK_KIND_VAL_ARRAY),
    0};

MOONBIT_EXPORT int64_t *const moonbit_empty_int64_array =
    moonbit_empty_int64_array_object.data;

static struct {
  int32_t rc;
  uint32_t meta;
  float data[];
} moonbit_empty_float_array_object = {
    Moonbit_make_static_rc(moonbit_BLOCK_KIND_VAL_ARRAY),
    0};

MOONBIT_EXPORT float *const moonbit_empty_float_array =
    moonbit_empty_float_array_object.data;

static struct {
  int32_t rc;
  uint32_t meta;
  double data[];
} moonbit_empty_double_array_object = {
    Moonbit_make_static_rc(moonbit_BLOCK_KIND_VAL_ARRAY),
    0};

MOONBIT_EXPORT double *const moonbit_empty_double_array =
    moonbit_empty_double_array_object.data;

static struct {
  int32_t rc;
  uint32_t meta;
  void *data[];
} moonbit_empty_ref_array_object = {
    Moonbit_make_static_rc(moonbit_BLOCK_KIND_REF_ARRAY),
    0};

MOONBIT_EXPORT void **const moonbit_empty_ref_array =
    moonbit_empty_ref_array_object.data;

static struct {
  int32_t rc;
  uint32_t meta;
  void *data[];
} moonbit_empty_extern_ref_array_object = {
    Moonbit_make_static_rc(moonbit_BLOCK_KIND_VAL_ARRAY),
    0};

MOONBIT_EXPORT void **const moonbit_empty_extern_ref_array =
    moonbit_empty_extern_ref_array_object.data;

#ifdef __cplusplus
}
#endif

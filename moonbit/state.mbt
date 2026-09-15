///|
let mode_shift : Int = 0

///|
let focus_shift : Int = 2

///|
let editing_shift : Int = 6

///|
let backup_shift : Int = 7

///|
let random_length_shift : Int = 14

///|
let digits_shift : Int = 21

///|
let symbols_shift : Int = 22

///|
let word_count_shift : Int = 23

///|
let capitalize_shift : Int = 26

///|
let complete_word_shift : Int = 27

///|
let separator_shift : Int = 28

///|
let pin_length_shift : Int = 30

///|
let action_shift : Int = 34

///|
let result_shift : Int = 36

///|
let generation_count_shift : Int = 38

///|
pub let state_field_mode : Int = 0

///|
pub let state_field_focus : Int = 1

///|
pub let state_field_editing : Int = 2

///|
pub let state_field_random_length : Int = 3

///|
pub let state_field_digits : Int = 4

///|
pub let state_field_symbols : Int = 5

///|
pub let state_field_word_count : Int = 6

///|
pub let state_field_capitalize : Int = 7

///|
pub let state_field_complete_word : Int = 8

///|
pub let state_field_separator : Int = 9

///|
pub let state_field_pin_length : Int = 10

///|
pub let state_field_result : Int = 11

///|
pub let state_field_generation_count : Int = 12

///|
pub let input_up : Int = 0

///|
pub let input_down : Int = 1

///|
pub let input_ok : Int = 2

///|
pub let input_ok_long : Int = 3

///|
pub let action_none : Int = 0

///|
pub let action_generate : Int = 1

///|
pub let result_empty : Int = 0

///|
pub let result_success : Int = 1

///|
pub let result_failure : Int = 2

///|
fn field(value : UInt64, shift : Int, mask : Int) -> Int {
  ((value >> shift) & mask.to_uint64()).to_int()
}

///|
fn set_field(value : UInt64, shift : Int, mask : Int, next : Int) -> UInt64 {
  let shifted_mask = mask.to_uint64() << shift
  (value & shifted_mask.lnot()) |
  ((next.to_uint64() & mask.to_uint64()) << shift)
}

///|
pub fn initial_state() -> UInt64 {
  let mut state = 0UL
  state = set_field(state, mode_shift, 3, 0)
  state = set_field(state, focus_shift, 15, 0)
  state = set_field(state, random_length_shift, 127, 10)
  state = set_field(state, digits_shift, 1, 1)
  state = set_field(state, symbols_shift, 1, 0)
  state = set_field(state, word_count_shift, 7, 4)
  state = set_field(state, capitalize_shift, 1, 0)
  state = set_field(state, complete_word_shift, 1, 1)
  state = set_field(state, separator_shift, 3, 0)
  state = set_field(state, pin_length_shift, 15, 6)
  state
}

///|
pub fn state_get(state : UInt64, requested : Int) -> Int {
  match requested {
    0 => field(state, mode_shift, 3)
    1 => field(state, focus_shift, 15)
    2 => field(state, editing_shift, 1)
    3 => field(state, random_length_shift, 127)
    4 => field(state, digits_shift, 1)
    5 => field(state, symbols_shift, 1)
    6 => field(state, word_count_shift, 7)
    7 => field(state, capitalize_shift, 1)
    8 => field(state, complete_word_shift, 1)
    9 => field(state, separator_shift, 3)
    10 => field(state, pin_length_shift, 15)
    11 => field(state, result_shift, 3)
    12 => field(state, generation_count_shift, 65535)
    _ => -1
  }
}

///|
pub fn state_action(state : UInt64) -> Int {
  field(state, action_shift, 3)
}

///|
pub fn clear_generation_result(state : UInt64) -> UInt64 {
  set_field(state, result_shift, 3, result_empty)
}

///|
pub fn record_generation_result(state : UInt64, result : Int) -> UInt64 {
  let status = if result == generate_ok {
    result_success
  } else {
    result_failure
  }
  let mut next = set_field(state, result_shift, 3, status)
  if status == result_success {
    let count = state_get(state, state_field_generation_count)
    next = set_field(
      next,
      generation_count_shift,
      65535,
      (count + 1).clamp(min=0, max=65535),
    )
  }
  next
}

///|
pub fn apply_input(state : UInt64, input : Int) -> UInt64 {
  let next = handle_input(state, input)
  if configuration_changed(state, next) {
    clear_generation_result(next)
  } else {
    next
  }
}

///|
pub fn mode_max_focus(mode : Int) -> Int {
  match mode {
    0 => 4
    1 => 5
    _ => 2
  }
}

///|
fn editable_value(state : UInt64, mode : Int, focus : Int) -> Int {
  match (mode, focus) {
    (0, 1) => state_get(state, state_field_random_length)
    (1, 1) => state_get(state, state_field_word_count)
    (1, 4) => state_get(state, state_field_separator)
    (2, 1) => state_get(state, state_field_pin_length)
    _ => 0
  }
}

///|
fn is_editable(mode : Int, focus : Int) -> Bool {
  (mode == 0 && focus == 1) ||
  (mode == 1 && (focus == 1 || focus == 4)) ||
  (mode == 2 && focus == 1)
}

///|
fn is_toggle(mode : Int, focus : Int) -> Bool {
  (mode == 0 && (focus == 2 || focus == 3)) ||
  (mode == 1 && (focus == 2 || focus == 3))
}

///|
fn restore_backup(state : UInt64, mode : Int, focus : Int) -> UInt64 {
  let backup = field(state, backup_shift, 127)
  match (mode, focus) {
    (0, 1) => set_field(state, random_length_shift, 127, backup)
    (1, 1) => set_field(state, word_count_shift, 7, backup)
    (1, 4) => set_field(state, separator_shift, 3, backup)
    (2, 1) => set_field(state, pin_length_shift, 15, backup)
    _ => state
  }
}

///|
fn adjust_editing(
  state : UInt64,
  mode : Int,
  focus : Int,
  delta : Int,
) -> UInt64 {
  match (mode, focus) {
    (0, 1) => {
      let current = state_get(state, state_field_random_length)
      set_field(
        state,
        random_length_shift,
        127,
        (current + delta).clamp(min=random_min_length, max=random_max_length),
      )
    }
    (1, 1) => {
      let current = state_get(state, state_field_word_count)
      set_field(
        state,
        word_count_shift,
        7,
        (current + delta).clamp(
          min=passphrase_min_words,
          max=passphrase_max_words,
        ),
      )
    }
    (1, 4) => {
      let current = state_get(state, state_field_separator)
      set_field(state, separator_shift, 3, (current + delta + 3) % 3)
    }
    (2, 1) => {
      let current = state_get(state, state_field_pin_length)
      set_field(
        state,
        pin_length_shift,
        15,
        (current + delta).clamp(min=pin_min_length, max=pin_max_length),
      )
    }
    _ => state
  }
}

///|
pub fn handle_input(original : UInt64, input : Int) -> UInt64 {
  let mut state = set_field(original, action_shift, 3, action_none)
  let mode = state_get(state, state_field_mode)
  let focus = state_get(state, state_field_focus)
  let editing = state_get(state, state_field_editing) == 1
  if editing {
    if input == input_up {
      return adjust_editing(state, mode, focus, 1)
    }
    if input == input_down {
      return adjust_editing(state, mode, focus, -1)
    }
    if input == input_ok {
      return set_field(state, editing_shift, 1, 0)
    }
    if input == input_ok_long {
      state = restore_backup(state, mode, focus)
      return set_field(state, editing_shift, 1, 0)
    }
    return state
  }
  if input == input_ok_long {
    return state
  }
  if focus == 0 {
    if input == input_up {
      return set_field(state, mode_shift, 3, (mode + 2) % 3)
    }
    if input == input_down {
      return set_field(state, mode_shift, 3, (mode + 1) % 3)
    }
    if input == input_ok {
      return set_field(state, focus_shift, 15, 1)
    }
    return state
  }
  if input == input_up {
    return set_field(state, focus_shift, 15, focus - 1)
  }
  if input == input_down {
    return set_field(
      state,
      focus_shift,
      15,
      (focus + 1).clamp(min=0, max=mode_max_focus(mode)),
    )
  }
  if input != input_ok {
    return state
  }
  if focus == mode_max_focus(mode) {
    return set_field(state, action_shift, 3, action_generate)
  }
  if is_toggle(mode, focus) {
    if mode == 0 && focus == 2 {
      return set_field(
        state,
        digits_shift,
        1,
        1 - state_get(state, state_field_digits),
      )
    }
    if mode == 0 && focus == 3 {
      return set_field(
        state,
        symbols_shift,
        1,
        1 - state_get(state, state_field_symbols),
      )
    }
    if mode == 1 && focus == 2 {
      return set_field(
        state,
        capitalize_shift,
        1,
        1 - state_get(state, state_field_capitalize),
      )
    }
    return set_field(
      state,
      complete_word_shift,
      1,
      1 - state_get(state, state_field_complete_word),
    )
  }
  if is_editable(mode, focus) {
    state = set_field(
      state,
      backup_shift,
      127,
      editable_value(state, mode, focus),
    )
    return set_field(state, editing_shift, 1, 1)
  }
  state
}

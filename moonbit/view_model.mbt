///|
/// Platform-independent presentation model for the fixed 240x320 product UI.
/// LVGL remains in C, while MoonBit decides visibility, focus, values, and
/// layout of every product parameter.
pub let view_mode_count : Int = 3

///|
pub let view_parameter_slots : Int = 5

///|
pub let parameter_hidden : Int = 0

///|
pub let parameter_random_length : Int = 1

///|
pub let parameter_lowercase : Int = 2

///|
pub let parameter_uppercase : Int = 3

///|
pub let parameter_digits : Int = 4

///|
pub let parameter_symbols : Int = 5

///|
pub let parameter_word_count : Int = 6

///|
pub let parameter_capitalize : Int = 7

///|
pub let parameter_complete_word : Int = 8

///|
pub let parameter_separator : Int = 9

///|
pub let parameter_pin_length : Int = 10

///|
pub fn view_mode_valid(mode : Int) -> Bool {
  mode >= 0 && mode < view_mode_count
}

///|
pub fn view_parameter_slot_valid(slot : Int) -> Bool {
  slot >= 0 && slot < view_parameter_slots
}

///|
pub fn view_parameter_kind(mode : Int, slot : Int) -> Int {
  if !view_parameter_slot_valid(slot) {
    return parameter_hidden
  }
  match mode {
    0 =>
      match slot {
        0 => parameter_random_length
        1 => parameter_lowercase
        2 => parameter_uppercase
        3 => parameter_digits
        4 => parameter_symbols
        _ => parameter_hidden
      }
    1 =>
      match slot {
        0 => parameter_word_count
        1 => parameter_capitalize
        2 => parameter_complete_word
        3 => parameter_separator
        _ => parameter_hidden
      }
    2 => if slot == 0 { parameter_pin_length } else { parameter_hidden }
    _ => parameter_hidden
  }
}

///|
pub fn view_parameter_visible(mode : Int, slot : Int) -> Bool {
  view_parameter_kind(mode, slot) != parameter_hidden
}

///|
pub fn view_parameter_focus(mode : Int, slot : Int) -> Int {
  let kind = view_parameter_kind(mode, slot)
  match kind {
    1 => 1
    2 => 2
    3 => 3
    4 => 4
    5 => 5
    6 => 1
    7 => 2
    8 => 3
    9 => 4
    10 => 1
    _ => 0
  }
}

///|
pub fn view_parameter_value(state : UInt64, slot : Int) -> Int {
  let mode = state_get(state, state_field_mode)
  match view_parameter_kind(mode, slot) {
    1 => state_get(state, state_field_random_length)
    2 => state_get(state, state_field_lowercase)
    3 => state_get(state, state_field_uppercase)
    4 => state_get(state, state_field_digits)
    5 => state_get(state, state_field_symbols)
    6 => state_get(state, state_field_word_count)
    7 => state_get(state, state_field_capitalize)
    8 => state_get(state, state_field_complete_word)
    9 => state_get(state, state_field_separator)
    10 => state_get(state, state_field_pin_length)
    _ => -1
  }
}

///|
pub fn view_parameter_x(mode : Int, slot : Int) -> Int {
  if !view_parameter_visible(mode, slot) {
    return 0
  }
  if mode == 2 {
    54
  } else if mode == 0 {
    if slot % 2 == 0 {
      2
    } else {
      106
    }
  } else if slot % 2 == 0 {
    2
  } else {
    106
  }
}

///|
pub fn view_parameter_y(mode : Int, slot : Int) -> Int {
  if !view_parameter_visible(mode, slot) {
    return 0
  }
  if mode == 2 {
    18
  } else if mode == 0 {
    2 + slot / 2 * 25
  } else if slot < 2 {
    2
  } else {
    34
  }
}

///|
pub fn view_parameter_width(mode : Int, slot : Int) -> Int {
  if view_parameter_visible(mode, slot) {
    100
  } else {
    0
  }
}

///|
pub fn view_parameter_height(mode : Int, slot : Int) -> Int {
  if view_parameter_visible(mode, slot) {
    if mode == 0 {
      22
    } else {
      27
    }
  } else {
    0
  }
}

///|
pub fn view_parameter_selected(state : UInt64, slot : Int) -> Bool {
  let mode = state_get(state, state_field_mode)
  let focus = state_get(state, state_field_focus)
  let parameter_focus = view_parameter_focus(mode, slot)
  parameter_focus != 0 && parameter_focus == focus
}

///|
pub fn view_parameter_editing(state : UInt64, slot : Int) -> Bool {
  view_parameter_selected(state, slot) &&
  state_get(state, state_field_editing) == 1
}

///|
pub fn view_mode_selected(state : UInt64, mode : Int) -> Bool {
  view_mode_valid(mode) && state_get(state, state_field_mode) == mode
}

///|
pub fn view_mode_focused(state : UInt64, mode : Int) -> Bool {
  view_mode_selected(state, mode) && state_get(state, state_field_focus) == 0
}

///|
pub fn view_generate_focused(state : UInt64) -> Bool {
  let mode = state_get(state, state_field_mode)
  state_get(state, state_field_focus) == mode_max_focus(mode)
}

///|
pub fn view_theme(state : UInt64) -> Int {
  state_get(state, state_field_theme)
}

///|
pub fn configuration_fingerprint(state : UInt64) -> UInt64 {
  let mut fingerprint = 0UL
  fingerprint = fingerprint | state_get(state, state_field_mode).to_uint64()
  fingerprint = fingerprint |
    (state_get(state, state_field_random_length).to_uint64() << 2)
  fingerprint = fingerprint |
    (state_get(state, state_field_digits).to_uint64() << 9)
  fingerprint = fingerprint |
    (state_get(state, state_field_symbols).to_uint64() << 10)
  fingerprint = fingerprint |
    (state_get(state, state_field_word_count).to_uint64() << 11)
  fingerprint = fingerprint |
    (state_get(state, state_field_capitalize).to_uint64() << 14)
  fingerprint = fingerprint |
    (state_get(state, state_field_complete_word).to_uint64() << 15)
  fingerprint = fingerprint |
    (state_get(state, state_field_separator).to_uint64() << 16)
  fingerprint = fingerprint |
    (state_get(state, state_field_pin_length).to_uint64() << 18)
  fingerprint = fingerprint |
    (state_get(state, state_field_policy_profile).to_uint64() << 22)
  fingerprint = fingerprint |
    (state_get(state, state_field_lowercase).to_uint64() << 24)
  fingerprint = fingerprint |
    (state_get(state, state_field_uppercase).to_uint64() << 25)
  fingerprint = fingerprint |
    (state_get(state, state_field_exclude_ambiguous).to_uint64() << 26)
  fingerprint = fingerprint |
    (state_get(state, state_field_safe_symbols_only).to_uint64() << 27)
  fingerprint
}

///|
pub fn configuration_changed(before : UInt64, after : UInt64) -> Bool {
  configuration_fingerprint(before) != configuration_fingerprint(after)
}

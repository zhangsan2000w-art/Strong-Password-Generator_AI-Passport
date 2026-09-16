///|
/// Platform-independent presentation model for the fixed 240x320 product UI.
/// LVGL remains in C, while MoonBit decides visibility, focus, values, and
/// layout of every product parameter.
pub let view_mode_count : Int = 3

///|
pub let view_parameter_slots : Int = 4

///|
pub let parameter_hidden : Int = 0

///|
pub let parameter_random_length : Int = 1

///|
pub let parameter_letters : Int = 2

///|
pub let parameter_digits : Int = 3

///|
pub let parameter_symbols : Int = 4

///|
pub let parameter_word_count : Int = 5

///|
pub let parameter_capitalize : Int = 6

///|
pub let parameter_complete_word : Int = 7

///|
pub let parameter_separator : Int = 8

///|
pub let parameter_pin_length : Int = 9

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
        1 => parameter_letters
        2 => parameter_digits
        3 => parameter_symbols
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
    2 => 0 // Letters are mandatory, so this row is informational.
    3 => 2
    4 => 3
    5 => 1
    6 => 2
    7 => 3
    8 => 4
    9 => 1
    _ => 0
  }
}

///|
pub fn view_parameter_value(state : UInt64, slot : Int) -> Int {
  let mode = state_get(state, state_field_mode)
  match view_parameter_kind(mode, slot) {
    1 => state_get(state, state_field_random_length)
    2 => 1
    3 => state_get(state, state_field_digits)
    4 => state_get(state, state_field_symbols)
    5 => state_get(state, state_field_word_count)
    6 => state_get(state, state_field_capitalize)
    7 => state_get(state, state_field_complete_word)
    8 => state_get(state, state_field_separator)
    9 => state_get(state, state_field_pin_length)
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
    27
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
  fingerprint | (state_get(state, state_field_pin_length).to_uint64() << 18)
}

///|
pub fn configuration_changed(before : UInt64, after : UInt64) -> Bool {
  configuration_fingerprint(before) != configuration_fingerprint(after)
}

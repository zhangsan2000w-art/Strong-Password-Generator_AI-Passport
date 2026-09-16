///|
/// Compact settings state. MoonBit owns focus and value transitions while C
/// owns rendering and persistence.
let settings_selected_shift : Int = 0

///|
let settings_theme_shift : Int = 1

///|
let settings_sound_shift : Int = 2

///|
pub let settings_option_theme : Int = 0

///|
pub let settings_option_sound : Int = 1

///|
fn settings_field(state : UInt64, shift : Int) -> Int {
  ((state >> shift) & 1UL).to_int()
}

///|
fn settings_set_field(state : UInt64, shift : Int, value : Int) -> UInt64 {
  let mask = 1UL << shift
  (state & mask.lnot()) | ((value.to_uint64() & 1UL) << shift)
}

///|
pub fn settings_initial(theme : Int, sound_enabled : Int) -> UInt64 {
  let mut state = 0UL
  state = settings_set_field(
    state,
    settings_theme_shift,
    if theme == theme_sky {
      1
    } else {
      0
    },
  )
  settings_set_field(
    state,
    settings_sound_shift,
    if sound_enabled != 0 {
      1
    } else {
      0
    },
  )
}

///|
pub fn settings_handle_input(state : UInt64, input : Int) -> UInt64 {
  if input == input_up || input == input_down {
    return settings_set_field(
      state,
      settings_selected_shift,
      1 - settings_field(state, settings_selected_shift),
    )
  }
  if input == input_ok {
    let selected = settings_field(state, settings_selected_shift)
    if selected == settings_option_theme {
      return settings_set_field(
        state,
        settings_theme_shift,
        1 - settings_field(state, settings_theme_shift),
      )
    } else {
      return settings_set_field(
        state,
        settings_sound_shift,
        1 - settings_field(state, settings_sound_shift),
      )
    }
  }
  state
}

///|
pub fn settings_map_button_event(button : Int, event : Int) -> Int {
  if button == button_ok && event == button_event_long {
    return input_ok_long
  }
  if event != button_event_click {
    return input_ignored
  }
  if button == button_up {
    return input_up
  }
  if button == button_down {
    return input_down
  }
  if button == button_ok {
    return input_ok
  }
  input_ignored
}

///|
pub fn settings_selected(state : UInt64) -> Int {
  settings_field(state, settings_selected_shift)
}

///|
pub fn settings_theme(state : UInt64) -> Int {
  settings_field(state, settings_theme_shift)
}

///|
pub fn settings_sound_enabled(state : UInt64) -> Int {
  settings_field(state, settings_sound_shift)
}

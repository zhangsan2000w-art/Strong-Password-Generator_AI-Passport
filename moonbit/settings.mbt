///|
/// Compact settings state. MoonBit owns focus and value transitions while C
/// owns rendering and persistence.
let settings_selected_shift : Int = 0

///|
let settings_theme_shift : Int = 2

///|
let settings_sound_shift : Int = 3

///|
let settings_policy_profile_shift : Int = 4

///|
let settings_exclude_ambiguous_shift : Int = 6

///|
pub let settings_option_theme : Int = 0

///|
pub let settings_option_sound : Int = 1

///|
pub let settings_option_policy_profile : Int = 2

///|
pub let settings_option_exclude_ambiguous : Int = 3

///|
pub let settings_option_count : Int = 4

///|
fn settings_field(state : UInt64, shift : Int, mask : Int) -> Int {
  ((state >> shift) & mask.to_uint64()).to_int()
}

///|
fn settings_set_field(
  state : UInt64,
  shift : Int,
  mask : Int,
  value : Int,
) -> UInt64 {
  let shifted_mask = mask.to_uint64() << shift
  (state & shifted_mask.lnot()) |
  ((value.to_uint64() & mask.to_uint64()) << shift)
}

///|
pub fn settings_initial(
  theme : Int,
  sound_enabled : Int,
  policy_profile : Int,
  exclude_ambiguous : Int,
) -> UInt64 {
  let mut state = 0UL
  state = settings_set_field(
    state,
    settings_theme_shift,
    1,
    if theme == theme_sky {
      1
    } else {
      0
    },
  )
  state = settings_set_field(
    state,
    settings_sound_shift,
    1,
    if sound_enabled != 0 {
      1
    } else {
      0
    },
  )
  state = settings_set_field(
    state,
    settings_policy_profile_shift,
    3,
    normalize_policy_profile(policy_profile),
  )
  settings_set_field(
    state,
    settings_exclude_ambiguous_shift,
    1,
    if exclude_ambiguous != 0 {
      1
    } else {
      0
    },
  )
}

///|
pub fn settings_handle_input(state : UInt64, input : Int) -> UInt64 {
  if input == input_up || input == input_down {
    let selected = settings_field(state, settings_selected_shift, 3)
    let delta = if input == input_up { settings_option_count - 1 } else { 1 }
    return settings_set_field(
      state,
      settings_selected_shift,
      3,
      (selected + delta) % settings_option_count,
    )
  }
  if input == input_ok {
    let selected = settings_field(state, settings_selected_shift, 3)
    if selected == settings_option_theme {
      return settings_set_field(
        state,
        settings_theme_shift,
        1,
        1 - settings_field(state, settings_theme_shift, 1),
      )
    }
    if selected == settings_option_sound {
      return settings_set_field(
        state,
        settings_sound_shift,
        1,
        1 - settings_field(state, settings_sound_shift, 1),
      )
    }
    if selected == settings_option_policy_profile {
      return settings_set_field(
        state,
        settings_policy_profile_shift,
        3,
        (settings_field(state, settings_policy_profile_shift, 3) + 1) %
        policy_profile_count,
      )
    }
    return settings_set_field(
      state,
      settings_exclude_ambiguous_shift,
      1,
      1 - settings_field(state, settings_exclude_ambiguous_shift, 1),
    )
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
  settings_field(state, settings_selected_shift, 3)
}

///|
pub fn settings_theme(state : UInt64) -> Int {
  settings_field(state, settings_theme_shift, 1)
}

///|
pub fn settings_sound_enabled(state : UInt64) -> Int {
  settings_field(state, settings_sound_shift, 1)
}

///|
pub fn settings_policy_profile(state : UInt64) -> Int {
  settings_field(state, settings_policy_profile_shift, 3)
}

///|
pub fn settings_exclude_ambiguous(state : UInt64) -> Int {
  settings_field(state, settings_exclude_ambiguous_shift, 1)
}

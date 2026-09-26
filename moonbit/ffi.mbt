///|
extern "C" fn platform_random_u32() -> UInt = "passport_random_u32"

///|
extern "C" fn platform_output_reset() -> Unit = "passport_output_reset"

///|
extern "C" fn platform_output_push(ch : Int) -> Bool = "passport_output_push"

///|
extern "C" fn platform_dictionary_count() -> Int = "passport_dictionary_count"

///|
extern "C" fn platform_dictionary_length(index : Int) -> Int = "passport_dictionary_length"

///|
extern "C" fn platform_dictionary_char(index : Int, offset : Int) -> Int = "passport_dictionary_char"

///|
#export_name("passport_moonbit_diagnostic_observation_initial")
pub fn passport_moonbit_diagnostic_observation_initial() -> UInt64 {
  diagnostic_observation_initial()
}

///|
#export_name("passport_moonbit_diagnostic_policy_state")
pub fn passport_moonbit_diagnostic_policy_state(state : UInt64) -> UInt64 {
  diagnostic_policy_state(state)
}

///|
#export_name("passport_moonbit_diagnostic_observe_character")
pub fn passport_moonbit_diagnostic_observe_character(
  policy_state : UInt64,
  observation : UInt64,
  ch : Int,
) -> UInt64 {
  diagnostic_observe_character(policy_state, observation, ch)
}

///|
#export_name("passport_moonbit_diagnostic_run_initial")
pub fn passport_moonbit_diagnostic_run_initial(target : Int) -> UInt64 {
  diagnostic_run_initial(target)
}

///|
#export_name("passport_moonbit_diagnostic_record_sample")
pub fn passport_moonbit_diagnostic_record_sample(
  run : UInt64,
  policy_state : UInt64,
  observation : UInt64,
  generation_result : Int,
) -> UInt64 {
  diagnostic_record_sample(run, policy_state, observation, generation_result)
}

///|
#export_name("passport_moonbit_diagnostic_cancel_run")
pub fn passport_moonbit_diagnostic_cancel_run(run : UInt64) -> UInt64 {
  diagnostic_cancel_run(run)
}

///|
#export_name("passport_moonbit_diagnostic_run_target")
pub fn passport_moonbit_diagnostic_run_target(run : UInt64) -> Int {
  diagnostic_run_target(run)
}

///|
#export_name("passport_moonbit_diagnostic_run_completed")
pub fn passport_moonbit_diagnostic_run_completed(run : UInt64) -> Int {
  diagnostic_run_completed(run)
}

///|
#export_name("passport_moonbit_diagnostic_run_passed")
pub fn passport_moonbit_diagnostic_run_passed(run : UInt64) -> Int {
  diagnostic_run_passed(run)
}

///|
#export_name("passport_moonbit_diagnostic_run_failure")
pub fn passport_moonbit_diagnostic_run_failure(run : UInt64) -> Int {
  diagnostic_run_failure(run)
}

///|
#export_name("passport_moonbit_diagnostic_run_status")
pub fn passport_moonbit_diagnostic_run_status(run : UInt64) -> Int {
  diagnostic_run_status(run)
}

///|
#export_name("passport_moonbit_policy_profile_normalize")
pub fn passport_moonbit_policy_profile_normalize(profile : Int) -> Int {
  normalize_policy_profile(profile)
}

///|
#export_name("passport_moonbit_policy_profile_default_length")
pub fn passport_moonbit_policy_profile_default_length(profile : Int) -> Int {
  policy_profile_default_length(profile)
}

///|
#export_name("passport_moonbit_policy_profile_min_length")
pub fn passport_moonbit_policy_profile_min_length(profile : Int) -> Int {
  policy_profile_min_length(profile)
}

///|
#export_name("passport_moonbit_policy_profile_symbols")
pub fn passport_moonbit_policy_profile_symbols(profile : Int) -> Int {
  if policy_profile_symbols(profile) {
    1
  } else {
    0
  }
}

///|
#export_name("passport_moonbit_policy_profile_exclude_ambiguous")
pub fn passport_moonbit_policy_profile_exclude_ambiguous(profile : Int) -> Int {
  if policy_profile_exclude_ambiguous(profile) {
    1
  } else {
    0
  }
}

///|
#export_name("passport_moonbit_policy_profile_safe_symbols_only")
pub fn passport_moonbit_policy_profile_safe_symbols_only(profile : Int) -> Int {
  if policy_profile_safe_symbols_only(profile) {
    1
  } else {
    0
  }
}

///|
#export_name("passport_moonbit_initial_state")
pub fn passport_moonbit_initial_state() -> UInt64 {
  initial_state()
}

///|
#export_name("passport_moonbit_with_policy_profile")
pub fn passport_moonbit_with_policy_profile(
  state : UInt64,
  profile : Int,
) -> UInt64 {
  with_policy_profile(state, profile)
}

///|
#export_name("passport_moonbit_with_policy_settings")
pub fn passport_moonbit_with_policy_settings(
  state : UInt64,
  profile : Int,
  exclude_ambiguous : Int,
) -> UInt64 {
  with_policy_settings(state, profile, exclude_ambiguous)
}

///|
#export_name("passport_moonbit_handle_input")
pub fn passport_moonbit_handle_input(state : UInt64, input : Int) -> UInt64 {
  apply_input(state, input)
}

///|
#export_name("passport_moonbit_state_get")
pub fn passport_moonbit_state_get(state : UInt64, requested : Int) -> Int {
  state_get(state, requested)
}

///|
#export_name("passport_moonbit_state_action")
pub fn passport_moonbit_state_action(state : UInt64) -> Int {
  state_action(state)
}

///|
#export_name("passport_moonbit_ble_keyboard_report")
pub fn passport_moonbit_ble_keyboard_report(character : Int) -> Int {
  ble_keyboard_report(character)
}

///|
#export_name("passport_moonbit_ble_keyboard_status")
pub fn passport_moonbit_ble_keyboard_status(status : Int) -> Int {
  ble_keyboard_status_normalize(status)
}

///|
#export_name("passport_moonbit_ble_keyboard_send_allowed")
pub fn passport_moonbit_ble_keyboard_send_allowed(
  state : UInt64,
  status : Int,
) -> Int {
  if ble_keyboard_send_allowed(state, status) {
    1
  } else {
    0
  }
}

///|
#export_name("passport_moonbit_ble_keyboard_input_allowed")
pub fn passport_moonbit_ble_keyboard_input_allowed(status : Int) -> Int {
  if ble_keyboard_input_allowed(status) {
    1
  } else {
    0
  }
}

///|
#export_name("passport_moonbit_ble_keyboard_should_advertise")
pub fn passport_moonbit_ble_keyboard_should_advertise(
  status : Int,
  connected : Int,
  advertising : Int,
) -> Int {
  if ble_keyboard_should_advertise(status, connected != 0, advertising != 0) {
    1
  } else {
    0
  }
}

///|
#export_name("passport_moonbit_ble_keyboard_retry_ms")
pub fn passport_moonbit_ble_keyboard_retry_ms() -> Int {
  ble_keyboard_advertising_retry_ms
}

///|
#export_name("passport_moonbit_generate")
pub fn passport_moonbit_generate(state : UInt64) -> Int {
  platform_output_reset()
  let next = fn() { platform_random_u32() }
  let emit = fn(ch) { platform_output_push(ch) }
  match state_get(state, state_field_mode) {
    0 =>
      generate_random_password_with_policy(
        next,
        emit,
        state_get(state, state_field_random_length),
        state_get(state, state_field_lowercase) == 1,
        state_get(state, state_field_uppercase) == 1,
        state_get(state, state_field_digits) == 1,
        state_get(state, state_field_symbols) == 1,
        state_get(state, state_field_exclude_ambiguous) == 1,
        state_get(state, state_field_safe_symbols_only) == 1,
      )
    1 =>
      generate_passphrase(
        next,
        emit,
        platform_dictionary_count(),
        fn(index) { platform_dictionary_length(index) },
        fn(index, offset) { platform_dictionary_char(index, offset) },
        state_get(state, state_field_word_count),
        state_get(state, state_field_capitalize) == 1,
        state_get(state, state_field_complete_word) == 1,
        state_get(state, state_field_separator),
      )
    _ => generate_pin(next, emit, state_get(state, state_field_pin_length))
  }
}

///|
#export_name("passport_moonbit_entropy_x10")
pub fn passport_moonbit_entropy_x10(state : UInt64) -> Int {
  state_entropy_x10(state)
}

///|
#export_name("passport_moonbit_strength")
pub fn passport_moonbit_strength(state : UInt64) -> Int {
  state_strength(state)
}

///|
#export_name("passport_moonbit_security_warning")
pub fn passport_moonbit_security_warning(state : UInt64) -> Int {
  state_security_warning(state)
}

///|
#export_name("passport_moonbit_configuration_valid")
pub fn passport_moonbit_configuration_valid(state : UInt64) -> Int {
  if state_configuration_valid(state) {
    1
  } else {
    0
  }
}

///|
#export_name("passport_moonbit_configuration_changed")
pub fn passport_moonbit_configuration_changed(
  before : UInt64,
  after : UInt64,
) -> Int {
  if configuration_changed(before, after) {
    1
  } else {
    0
  }
}

///|
#export_name("passport_moonbit_record_generation")
pub fn passport_moonbit_record_generation(
  state : UInt64,
  result : Int,
) -> UInt64 {
  record_generation_result(state, result)
}

///|
#export_name("passport_moonbit_password_display_begin")
pub fn passport_moonbit_password_display_begin(state : UInt64) -> Int {
  password_display_begin(state_get(state, state_field_result))
}

///|
#export_name("passport_moonbit_password_display_is_visible")
pub fn passport_moonbit_password_display_is_visible(mode : Int) -> Int {
  if password_display_is_visible(mode) {
    1
  } else {
    0
  }
}

///|
#export_name("passport_moonbit_password_display_update")
pub fn passport_moonbit_password_display_update(
  state : UInt64,
  current : Int,
  elapsed_ms : UInt64,
) -> Int {
  password_display_update(
    current,
    state_get(state, state_field_result),
    elapsed_ms,
  )
}

///|
#export_name("passport_moonbit_password_display_timeout_ms")
pub fn passport_moonbit_password_display_timeout_ms() -> UInt64 {
  password_display_timeout_ms
}

///|
#export_name("passport_moonbit_password_display_mask_width")
pub fn passport_moonbit_password_display_mask_width() -> Int {
  password_display_mask_width
}

///|
#export_name("passport_moonbit_password_display_mask_character")
pub fn passport_moonbit_password_display_mask_character() -> Int {
  password_display_mask_character
}

///|
#export_name("passport_moonbit_map_button_event")
pub fn passport_moonbit_map_button_event(
  state : UInt64,
  button : Int,
  event : Int,
) -> Int {
  map_button_event(state, button, event)
}

///|
#export_name("passport_moonbit_mode_max_focus")
pub fn passport_moonbit_mode_max_focus(mode : Int) -> Int {
  mode_max_focus(mode)
}

///|
#export_name("passport_moonbit_view_parameter_kind")
pub fn passport_moonbit_view_parameter_kind(state : UInt64, slot : Int) -> Int {
  view_parameter_kind(state_get(state, state_field_mode), slot)
}

///|
#export_name("passport_moonbit_view_parameter_value")
pub fn passport_moonbit_view_parameter_value(state : UInt64, slot : Int) -> Int {
  view_parameter_value(state, slot)
}

///|
#export_name("passport_moonbit_view_parameter_x")
pub fn passport_moonbit_view_parameter_x(state : UInt64, slot : Int) -> Int {
  view_parameter_x(state_get(state, state_field_mode), slot)
}

///|
#export_name("passport_moonbit_view_parameter_y")
pub fn passport_moonbit_view_parameter_y(state : UInt64, slot : Int) -> Int {
  view_parameter_y(state_get(state, state_field_mode), slot)
}

///|
#export_name("passport_moonbit_view_parameter_width")
pub fn passport_moonbit_view_parameter_width(state : UInt64, slot : Int) -> Int {
  view_parameter_width(state_get(state, state_field_mode), slot)
}

///|
#export_name("passport_moonbit_view_parameter_height")
pub fn passport_moonbit_view_parameter_height(
  state : UInt64,
  slot : Int,
) -> Int {
  view_parameter_height(state_get(state, state_field_mode), slot)
}

///|
#export_name("passport_moonbit_view_parameter_selected")
pub fn passport_moonbit_view_parameter_selected(
  state : UInt64,
  slot : Int,
) -> Int {
  if view_parameter_selected(state, slot) {
    1
  } else {
    0
  }
}

///|
#export_name("passport_moonbit_view_parameter_editing")
pub fn passport_moonbit_view_parameter_editing(
  state : UInt64,
  slot : Int,
) -> Int {
  if view_parameter_editing(state, slot) {
    1
  } else {
    0
  }
}

///|
#export_name("passport_moonbit_view_mode_selected")
pub fn passport_moonbit_view_mode_selected(state : UInt64, mode : Int) -> Int {
  if view_mode_selected(state, mode) {
    1
  } else {
    0
  }
}

///|
#export_name("passport_moonbit_view_mode_focused")
pub fn passport_moonbit_view_mode_focused(state : UInt64, mode : Int) -> Int {
  if view_mode_focused(state, mode) {
    1
  } else {
    0
  }
}

///|
#export_name("passport_moonbit_view_generate_focused")
pub fn passport_moonbit_view_generate_focused(state : UInt64) -> Int {
  if view_generate_focused(state) {
    1
  } else {
    0
  }
}

///|
#export_name("passport_moonbit_view_send_focused")
pub fn passport_moonbit_view_send_focused(state : UInt64) -> Int {
  if view_send_focused(state) {
    1
  } else {
    0
  }
}

///|
#export_name("passport_moonbit_view_theme")
pub fn passport_moonbit_view_theme(state : UInt64) -> Int {
  view_theme(state)
}

///|
#export_name("passport_moonbit_with_theme")
pub fn passport_moonbit_with_theme(state : UInt64, theme : Int) -> UInt64 {
  with_theme(state, theme)
}

///|
#export_name("passport_moonbit_settings_initial")
pub fn passport_moonbit_settings_initial(
  theme : Int,
  sound_enabled : Int,
  policy_profile : Int,
  exclude_ambiguous : Int,
) -> UInt64 {
  settings_initial(theme, sound_enabled, policy_profile, exclude_ambiguous)
}

///|
#export_name("passport_moonbit_settings_handle_input")
pub fn passport_moonbit_settings_handle_input(
  state : UInt64,
  input : Int,
) -> UInt64 {
  settings_handle_input(state, input)
}

///|
#export_name("passport_moonbit_settings_map_button_event")
pub fn passport_moonbit_settings_map_button_event(
  button : Int,
  event : Int,
) -> Int {
  settings_map_button_event(button, event)
}

///|
#export_name("passport_moonbit_settings_selected")
pub fn passport_moonbit_settings_selected(state : UInt64) -> Int {
  settings_selected(state)
}

///|
#export_name("passport_moonbit_settings_action")
pub fn passport_moonbit_settings_action(state : UInt64) -> Int {
  settings_action(state)
}

///|
#export_name("passport_moonbit_settings_theme")
pub fn passport_moonbit_settings_theme(state : UInt64) -> Int {
  settings_theme(state)
}

///|
#export_name("passport_moonbit_settings_sound_enabled")
pub fn passport_moonbit_settings_sound_enabled(state : UInt64) -> Int {
  settings_sound_enabled(state)
}

///|
#export_name("passport_moonbit_settings_policy_profile")
pub fn passport_moonbit_settings_policy_profile(state : UInt64) -> Int {
  settings_policy_profile(state)
}

///|
#export_name("passport_moonbit_settings_exclude_ambiguous")
pub fn passport_moonbit_settings_exclude_ambiguous(state : UInt64) -> Int {
  settings_exclude_ambiguous(state)
}

///|
#export_name("passport_moonbit_battery_resolve")
pub fn passport_moonbit_battery_resolve(percent : Int, millivolts : Int) -> Int {
  battery_resolve(percent, millivolts)
}

///|
#export_name("passport_moonbit_battery_percent")
pub fn passport_moonbit_battery_percent(reading : Int) -> Int {
  battery_reading_percent(reading)
}

///|
#export_name("passport_moonbit_battery_available")
pub fn passport_moonbit_battery_available(reading : Int) -> Int {
  if battery_reading_available(reading) {
    1
  } else {
    0
  }
}

///|
#export_name("passport_moonbit_battery_estimated")
pub fn passport_moonbit_battery_estimated(reading : Int) -> Int {
  if battery_reading_estimated(reading) {
    1
  } else {
    0
  }
}

///|
#export_name("passport_moonbit_battery_low")
pub fn passport_moonbit_battery_low(reading : Int) -> Int {
  if battery_reading_low(reading) {
    1
  } else {
    0
  }
}

///|
#export_name("passport_moonbit_battery_refresh_interval_ms")
pub fn passport_moonbit_battery_refresh_interval_ms() -> Int {
  battery_refresh_interval_ms
}

///|
#export_name("passport_moonbit_sound_sample_rate")
pub fn passport_moonbit_sound_sample_rate() -> Int {
  sound_sample_rate
}

///|
#export_name("passport_moonbit_sound_volume")
pub fn passport_moonbit_sound_volume() -> Int {
  sound_volume_percent
}

///|
#export_name("passport_moonbit_sound_note_count")
pub fn passport_moonbit_sound_note_count() -> Int {
  sound_note_count
}

///|
#export_name("passport_moonbit_sound_note_samples")
pub fn passport_moonbit_sound_note_samples(note : Int) -> Int {
  sound_note_sample_count(note)
}

///|
#export_name("passport_moonbit_sound_sample")
pub fn passport_moonbit_sound_sample(note : Int, sample_index : Int) -> Int {
  sound_sample(note, sample_index)
}

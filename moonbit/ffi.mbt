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
#export_name("passport_moonbit_initial_state")
pub fn passport_moonbit_initial_state() -> UInt64 {
  initial_state()
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
#export_name("passport_moonbit_generate")
pub fn passport_moonbit_generate(state : UInt64) -> Int {
  platform_output_reset()
  let next = fn() { platform_random_u32() }
  let emit = fn(ch) { platform_output_push(ch) }
  match state_get(state, state_field_mode) {
    0 =>
      generate_random_password(
        next,
        emit,
        state_get(state, state_field_random_length),
        state_get(state, state_field_digits) == 1,
        state_get(state, state_field_symbols) == 1,
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

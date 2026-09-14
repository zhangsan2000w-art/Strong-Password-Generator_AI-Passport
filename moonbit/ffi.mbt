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
  handle_input(state, input)
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
  match state_get(state, state_field_mode) {
    0 =>
      random_entropy_bits_x10(
        state_get(state, state_field_random_length),
        state_get(state, state_field_digits) == 1,
        state_get(state, state_field_symbols) == 1,
      )
    1 =>
      passphrase_entropy_bits_x10(
        state_get(state, state_field_word_count),
        state_get(state, state_field_complete_word) == 1,
      )
    _ => pin_entropy_bits_x10(state_get(state, state_field_pin_length))
  }
}

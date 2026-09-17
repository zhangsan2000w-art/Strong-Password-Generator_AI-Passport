///|
pub let strength_unknown : Int = 0

///|
pub let strength_weak : Int = 1

///|
pub let strength_fair : Int = 2

///|
pub let strength_strong : Int = 3

///|
pub let strength_very_strong : Int = 4

///|
pub fn strength_from_entropy_x10(entropy_x10 : Int) -> Int {
  if entropy_x10 <= 0 {
    strength_unknown
  } else if entropy_x10 < 300 {
    strength_weak
  } else if entropy_x10 < 500 {
    strength_fair
  } else if entropy_x10 < 800 {
    strength_strong
  } else {
    strength_very_strong
  }
}

///|
pub fn state_entropy_x10(state : UInt64) -> Int {
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
    2 => pin_entropy_bits_x10(state_get(state, state_field_pin_length))
    _ => 0
  }
}

///|
pub fn state_strength(state : UInt64) -> Int {
  strength_from_entropy_x10(state_entropy_x10(state))
}

///|
pub fn state_configuration_valid(state : UInt64) -> Bool {
  match state_get(state, state_field_mode) {
    0 => {
      let length = state_get(state, state_field_random_length)
      let profile = state_get(state, state_field_policy_profile)
      length >= policy_profile_min_length(profile) &&
      random_policy_valid(
        length,
        state_get(state, state_field_lowercase) == 1,
        state_get(state, state_field_uppercase) == 1,
        state_get(state, state_field_digits) == 1,
        state_get(state, state_field_symbols) == 1,
      )
    }
    1 => {
      let words = state_get(state, state_field_word_count)
      let separator = state_get(state, state_field_separator)
      words >= passphrase_min_words &&
      words <= passphrase_max_words &&
      separator_at(separator) >= 0
    }
    2 => {
      let length = state_get(state, state_field_pin_length)
      length >= pin_min_length && length <= pin_max_length
    }
    _ => false
  }
}

///|
pub let security_warning_none : Int = 0

///|
pub let security_warning_short : Int = 1

///|
pub let security_warning_missing_lowercase : Int = 2

///|
pub let security_warning_missing_uppercase : Int = 3

///|
pub let security_warning_missing_digits : Int = 4

///|
pub let security_warning_missing_symbols : Int = 5

///|
pub let security_warning_invalid : Int = 6

///|
/// Returns a stable reason code for the first actionable random-password
/// issue. Non-random modes use their existing entropy display without policy
/// warnings.
pub fn state_security_warning(state : UInt64) -> Int {
  if state_get(state, state_field_mode) != 0 {
    return security_warning_none
  }
  let profile = state_get(state, state_field_policy_profile)
  if state_get(state, state_field_random_length) <
    policy_profile_min_length(profile) {
    return security_warning_short
  }
  if state_get(state, state_field_lowercase) == 0 {
    return security_warning_missing_lowercase
  }
  if state_get(state, state_field_uppercase) == 0 {
    return security_warning_missing_uppercase
  }
  if state_get(state, state_field_digits) == 0 {
    return security_warning_missing_digits
  }
  if policy_profile_symbols(profile) &&
    state_get(state, state_field_symbols) == 0 {
    return security_warning_missing_symbols
  }
  if !state_configuration_valid(state) {
    return security_warning_invalid
  }
  security_warning_none
}

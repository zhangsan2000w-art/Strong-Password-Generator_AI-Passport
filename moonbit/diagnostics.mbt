///|
pub let diagnostic_sample_ok : Int = 0

///|
pub let diagnostic_failure_generation : Int = 1

///|
pub let diagnostic_failure_length : Int = 2

///|
pub let diagnostic_failure_class : Int = 3

///|
pub let diagnostic_failure_ambiguous : Int = 4

///|
pub let diagnostic_failure_unsafe_symbol : Int = 5

///|
pub let diagnostic_failure_character : Int = 6

///|
pub let diagnostic_status_running : Int = 0

///|
pub let diagnostic_status_passed : Int = 1

///|
pub let diagnostic_status_failed : Int = 2

///|
pub let diagnostic_default_samples : Int = 32

///|
let observation_length_shift : Int = 0

///|
let observation_lowercase_shift : Int = 7

///|
let observation_uppercase_shift : Int = 8

///|
let observation_digits_shift : Int = 9

///|
let observation_symbols_shift : Int = 10

///|
let observation_invalid_shift : Int = 11

///|
let observation_ambiguous_shift : Int = 12

///|
let observation_unsafe_symbol_shift : Int = 13

///|
fn diagnostic_field(value : UInt64, shift : Int, mask : Int) -> Int {
  ((value >> shift) & mask.to_uint64()).to_int()
}

///|
fn diagnostic_set_field(
  value : UInt64,
  shift : Int,
  mask : Int,
  next : Int,
) -> UInt64 {
  let shifted_mask = mask.to_uint64() << shift
  (value & shifted_mask.lnot()) |
  ((next.to_uint64() & mask.to_uint64()) << shift)
}

///|
pub fn diagnostic_observation_initial() -> UInt64 {
  0UL
}

///|
pub fn diagnostic_observe_character(
  policy_state : UInt64,
  observation : UInt64,
  ch : Int,
) -> UInt64 {
  let length = diagnostic_field(observation, observation_length_shift, 127)
  let mut next = diagnostic_set_field(
    observation,
    observation_length_shift,
    127,
    (length + 1).clamp(min=0, max=127),
  )
  if is_ascii_lower(ch) {
    next = diagnostic_set_field(next, observation_lowercase_shift, 1, 1)
  }
  if is_ascii_upper(ch) {
    next = diagnostic_set_field(next, observation_uppercase_shift, 1, 1)
  }
  if is_ascii_digit(ch) {
    next = diagnostic_set_field(next, observation_digits_shift, 1, 1)
  }
  if is_allowed_symbol(ch) {
    next = diagnostic_set_field(next, observation_symbols_shift, 1, 1)
  }
  let exclude_ambiguous = state_get(policy_state, state_field_exclude_ambiguous) ==
    1
  let safe_symbols_only = state_get(policy_state, state_field_safe_symbols_only) ==
    1
  if exclude_ambiguous && is_ambiguous_character(ch) {
    next = diagnostic_set_field(next, observation_ambiguous_shift, 1, 1)
  }
  if safe_symbols_only && is_allowed_symbol(ch) && !is_safe_symbol(ch) {
    next = diagnostic_set_field(next, observation_unsafe_symbol_shift, 1, 1)
  }
  if !is_allowed_random_character_with_policy(
      ch,
      state_get(policy_state, state_field_lowercase) == 1,
      state_get(policy_state, state_field_uppercase) == 1,
      state_get(policy_state, state_field_digits) == 1,
      state_get(policy_state, state_field_symbols) == 1,
      exclude_ambiguous,
      safe_symbols_only,
    ) {
    next = diagnostic_set_field(next, observation_invalid_shift, 1, 1)
  }
  next
}

///|
pub fn diagnostic_sample_failure(
  policy_state : UInt64,
  observation : UInt64,
  generation_result : Int,
) -> Int {
  if generation_result != generate_ok {
    return diagnostic_failure_generation
  }
  if diagnostic_field(observation, observation_length_shift, 127) !=
    state_get(policy_state, state_field_random_length) {
    return diagnostic_failure_length
  }
  if diagnostic_field(observation, observation_ambiguous_shift, 1) == 1 {
    return diagnostic_failure_ambiguous
  }
  if diagnostic_field(observation, observation_unsafe_symbol_shift, 1) == 1 {
    return diagnostic_failure_unsafe_symbol
  }
  if diagnostic_field(observation, observation_invalid_shift, 1) == 1 {
    return diagnostic_failure_character
  }
  if state_get(policy_state, state_field_lowercase) == 1 &&
    diagnostic_field(observation, observation_lowercase_shift, 1) == 0 {
    return diagnostic_failure_class
  }
  if state_get(policy_state, state_field_uppercase) == 1 &&
    diagnostic_field(observation, observation_uppercase_shift, 1) == 0 {
    return diagnostic_failure_class
  }
  if state_get(policy_state, state_field_digits) == 1 &&
    diagnostic_field(observation, observation_digits_shift, 1) == 0 {
    return diagnostic_failure_class
  }
  if state_get(policy_state, state_field_symbols) == 1 &&
    diagnostic_field(observation, observation_symbols_shift, 1) == 0 {
    return diagnostic_failure_class
  }
  diagnostic_sample_ok
}

///|
let run_target_shift : Int = 0

///|
let run_completed_shift : Int = 7

///|
let run_passed_shift : Int = 14

///|
let run_failure_shift : Int = 21

///|
pub fn diagnostic_run_initial(target : Int) -> UInt64 {
  diagnostic_set_field(0UL, run_target_shift, 127, target.clamp(min=1, max=64))
}

///|
pub fn diagnostic_run_target(run : UInt64) -> Int {
  diagnostic_field(run, run_target_shift, 127)
}

///|
pub fn diagnostic_run_completed(run : UInt64) -> Int {
  diagnostic_field(run, run_completed_shift, 127)
}

///|
pub fn diagnostic_run_passed(run : UInt64) -> Int {
  diagnostic_field(run, run_passed_shift, 127)
}

///|
pub fn diagnostic_run_failure(run : UInt64) -> Int {
  diagnostic_field(run, run_failure_shift, 15)
}

///|
pub fn diagnostic_record_sample(
  run : UInt64,
  policy_state : UInt64,
  observation : UInt64,
  generation_result : Int,
) -> UInt64 {
  let target = diagnostic_run_target(run)
  let completed = diagnostic_run_completed(run)
  if completed >= target {
    return run
  }
  let failure = diagnostic_sample_failure(
    policy_state, observation, generation_result,
  )
  let mut next = diagnostic_set_field(
    run,
    run_completed_shift,
    127,
    completed + 1,
  )
  if failure == diagnostic_sample_ok {
    next = diagnostic_set_field(
      next,
      run_passed_shift,
      127,
      diagnostic_run_passed(run) + 1,
    )
  } else if diagnostic_run_failure(run) == diagnostic_sample_ok {
    next = diagnostic_set_field(next, run_failure_shift, 15, failure)
  }
  next
}

///|
pub fn diagnostic_run_status(run : UInt64) -> Int {
  if diagnostic_run_completed(run) < diagnostic_run_target(run) {
    diagnostic_status_running
  } else if diagnostic_run_failure(run) == diagnostic_sample_ok &&
    diagnostic_run_passed(run) == diagnostic_run_target(run) {
    diagnostic_status_passed
  } else {
    diagnostic_status_failed
  }
}

///|
fn observe_codes(policy_state : UInt64, codes : Array[Int]) -> UInt64 {
  let mut observation = diagnostic_observation_initial()
  for code in codes {
    observation = diagnostic_observe_character(policy_state, observation, code)
  }
  observation
}

///|
test "diagnostics accepts a compliant compatible sample" {
  let policy = with_policy_profile(initial_state(), policy_profile_compatible)
  let observation = observe_codes(policy, [
    97, 98, 99, 100, 65, 66, 67, 68, 50, 51, 52, 53,
  ])
  @test.assert_eq(
    diagnostic_sample_failure(policy, observation, generate_ok),
    diagnostic_sample_ok,
  )
}

///|
test "diagnostics distinguishes output constraint failures" {
  let policy = with_policy_profile(initial_state(), policy_profile_standard)
  let valid = observe_codes(policy, [
    97, 98, 99, 100, 65, 66, 67, 68, 50, 51, 52, 53, 33, 35, 45, 63,
  ])
  @test.assert_eq(
    diagnostic_sample_failure(policy, valid, generate_ok),
    diagnostic_sample_ok,
  )
  let short = observe_codes(policy, [97, 65, 50, 33])
  @test.assert_eq(
    diagnostic_sample_failure(policy, short, generate_ok),
    diagnostic_failure_length,
  )
  let ambiguous = observe_codes(policy, [
    97, 98, 99, 100, 65, 66, 67, 68, 48, 51, 52, 53, 33, 35, 45, 63,
  ])
  @test.assert_eq(
    diagnostic_sample_failure(policy, ambiguous, generate_ok),
    diagnostic_failure_ambiguous,
  )
  let unsafe_symbol = observe_codes(policy, [
    97, 98, 99, 100, 65, 66, 67, 68, 50, 51, 52, 53, 94, 35, 45, 63,
  ])
  @test.assert_eq(
    diagnostic_sample_failure(policy, unsafe_symbol, generate_ok),
    diagnostic_failure_unsafe_symbol,
  )
  let missing_symbol = observe_codes(policy, [
    97, 98, 99, 100, 101, 102, 103, 104, 65, 66, 67, 68, 50, 51, 52, 53,
  ])
  @test.assert_eq(
    diagnostic_sample_failure(policy, missing_symbol, generate_ok),
    diagnostic_failure_class,
  )
  let unknown_character = observe_codes(policy, [
    97, 98, 99, 100, 65, 66, 67, 68, 50, 51, 52, 53, 33, 35, 45, 32,
  ])
  @test.assert_eq(
    diagnostic_sample_failure(policy, unknown_character, generate_ok),
    diagnostic_failure_character,
  )
  @test.assert_eq(
    diagnostic_sample_failure(policy, valid, generate_output_failed),
    diagnostic_failure_generation,
  )
}

///|
test "diagnostic run tracks progress without retaining samples" {
  let policy = with_policy_profile(initial_state(), policy_profile_compatible)
  let valid = observe_codes(policy, [
    97, 98, 99, 100, 65, 66, 67, 68, 50, 51, 52, 53,
  ])
  let invalid = observe_codes(policy, [97, 65, 50])
  let mut run = diagnostic_run_initial(2)
  @test.assert_eq(diagnostic_run_status(run), diagnostic_status_running)
  run = diagnostic_record_sample(run, policy, valid, generate_ok)
  @test.assert_eq(diagnostic_run_completed(run), 1)
  @test.assert_eq(diagnostic_run_passed(run), 1)
  run = diagnostic_record_sample(run, policy, invalid, generate_ok)
  @test.assert_eq(diagnostic_run_completed(run), 2)
  @test.assert_eq(diagnostic_run_passed(run), 1)
  @test.assert_eq(diagnostic_run_failure(run), diagnostic_failure_length)
  @test.assert_eq(diagnostic_run_status(run), diagnostic_status_failed)
  @test.assert_eq(
    diagnostic_record_sample(run, policy, valid, generate_ok),
    run,
  )
}

///|
test "diagnostic target is bounded for on-device work" {
  @test.assert_eq(diagnostic_run_target(diagnostic_run_initial(0)), 1)
  @test.assert_eq(diagnostic_run_target(diagnostic_run_initial(99)), 64)
  @test.assert_eq(
    diagnostic_run_target(diagnostic_run_initial(diagnostic_default_samples)),
    diagnostic_default_samples,
  )
}

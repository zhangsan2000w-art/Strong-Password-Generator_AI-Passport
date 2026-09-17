///|
test "policy profiles normalize unknown ids to standard" {
  @test.assert_eq(
    normalize_policy_profile(policy_profile_compatible),
    policy_profile_compatible,
  )
  @test.assert_eq(
    normalize_policy_profile(policy_profile_standard),
    policy_profile_standard,
  )
  @test.assert_eq(
    normalize_policy_profile(policy_profile_strict),
    policy_profile_strict,
  )
  @test.assert_eq(normalize_policy_profile(-1), policy_profile_standard)
  @test.assert_eq(normalize_policy_profile(99), policy_profile_standard)
}

///|
test "policy profiles expose progressively stronger defaults" {
  @test.assert_eq(policy_profile_default_length(policy_profile_compatible), 12)
  @test.assert_eq(policy_profile_default_length(policy_profile_standard), 16)
  @test.assert_eq(policy_profile_default_length(policy_profile_strict), 20)
  @test.assert_eq(policy_profile_min_length(policy_profile_compatible), 8)
  @test.assert_eq(policy_profile_min_length(policy_profile_standard), 12)
  @test.assert_eq(policy_profile_min_length(policy_profile_strict), 16)
  @test.assert_eq(policy_profile_symbols(policy_profile_compatible), false)
  @test.assert_eq(policy_profile_symbols(policy_profile_standard), true)
  @test.assert_eq(policy_profile_symbols(policy_profile_strict), true)
}

///|
test "policy profiles keep character requirements in MoonBit" {
  let profiles = [
    policy_profile_compatible, policy_profile_standard, policy_profile_strict,
  ]
  for profile in profiles {
    @test.assert_eq(policy_profile_lowercase(profile), true)
    @test.assert_eq(policy_profile_uppercase(profile), true)
    @test.assert_eq(policy_profile_digits(profile), true)
  }
  @test.assert_eq(
    policy_profile_exclude_ambiguous(policy_profile_compatible),
    true,
  )
  @test.assert_eq(
    policy_profile_exclude_ambiguous(policy_profile_standard),
    true,
  )
  @test.assert_eq(
    policy_profile_exclude_ambiguous(policy_profile_strict),
    false,
  )
  @test.assert_eq(
    policy_profile_safe_symbols_only(policy_profile_compatible),
    true,
  )
  @test.assert_eq(
    policy_profile_safe_symbols_only(policy_profile_standard),
    true,
  )
  @test.assert_eq(
    policy_profile_safe_symbols_only(policy_profile_strict),
    false,
  )
}

///|
test "policy length normalization preserves valid custom lengths" {
  @test.assert_eq(normalize_policy_length(policy_profile_compatible, 6), 8)
  @test.assert_eq(normalize_policy_length(policy_profile_standard, 10), 12)
  @test.assert_eq(normalize_policy_length(policy_profile_strict, 15), 16)
  @test.assert_eq(normalize_policy_length(policy_profile_standard, 24), 24)
  @test.assert_eq(normalize_policy_length(policy_profile_strict, 99), 30)
}

///|
test "app state applies complete profile defaults atomically" {
  let initial = initial_state()
  let compatible = with_policy_profile(initial, policy_profile_compatible)
  @test.assert_eq(
    state_get(compatible, state_field_policy_profile),
    policy_profile_compatible,
  )
  @test.assert_eq(state_get(compatible, state_field_random_length), 12)
  @test.assert_eq(state_get(compatible, state_field_lowercase), 1)
  @test.assert_eq(state_get(compatible, state_field_uppercase), 1)
  @test.assert_eq(state_get(compatible, state_field_digits), 1)
  @test.assert_eq(state_get(compatible, state_field_symbols), 0)
  @test.assert_eq(state_get(compatible, state_field_exclude_ambiguous), 1)
  @test.assert_eq(state_get(compatible, state_field_safe_symbols_only), 1)

  let strict = with_policy_profile(compatible, policy_profile_strict)
  @test.assert_eq(state_get(strict, state_field_random_length), 20)
  @test.assert_eq(state_get(strict, state_field_symbols), 1)
  @test.assert_eq(state_get(strict, state_field_exclude_ambiguous), 0)
  @test.assert_eq(state_get(strict, state_field_safe_symbols_only), 0)
  @test.assert_eq(state_configuration_valid(strict), true)

  let normalized = with_policy_profile(strict, 99)
  @test.assert_eq(
    state_get(normalized, state_field_policy_profile),
    policy_profile_standard,
  )
}

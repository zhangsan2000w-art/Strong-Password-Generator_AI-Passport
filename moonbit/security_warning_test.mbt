///|
test "standard profile entropy uses its filtered character pool" {
  @test.assert_eq(log2_milli(57) >= 5830, true)
  @test.assert_eq(log2_milli(57) <= 5835, true)
  @test.assert_eq(log2_milli(66) >= 6040, true)
  @test.assert_eq(log2_milli(66) <= 6048, true)
  @test.assert_eq(log2_milli(75) >= 6225, true)
  @test.assert_eq(log2_milli(75) <= 6232, true)

  let standard = initial_state()
  @test.assert_eq(state_entropy_x10(standard) >= 966, true)
  @test.assert_eq(state_entropy_x10(standard) <= 968, true)
  let strict = with_policy_profile(standard, policy_profile_strict)
  @test.assert_eq(state_entropy_x10(strict) >= 1245, true)
  @test.assert_eq(state_entropy_x10(strict) <= 1247, true)
}

///|
test "entropy follows selected classes and filters" {
  let full = random_entropy_bits_x10_with_policy(
    16, true, true, true, true, false, false,
  )
  let filtered = random_entropy_bits_x10_with_policy(
    16, true, true, true, true, true, true,
  )
  let letters_only = random_entropy_bits_x10_with_policy(
    16, true, true, false, false, true, true,
  )
  @test.assert_eq(full > filtered, true)
  @test.assert_eq(filtered > letters_only, true)
  @test.assert_eq(
    random_entropy_bits_x10_with_policy(
      16, false, false, false, false, true, true,
    ),
    0,
  )
}

///|
test "security warning reports actionable class omissions" {
  let standard = initial_state()
  @test.assert_eq(state_security_warning(standard), security_warning_none)
  let no_lowercase = with_random_policy_options(
    standard, false, true, true, true, true, true,
  )
  @test.assert_eq(
    state_security_warning(no_lowercase),
    security_warning_missing_lowercase,
  )
  let no_digits = with_random_policy_options(
    standard, true, true, false, true, true, true,
  )
  @test.assert_eq(
    state_security_warning(no_digits),
    security_warning_missing_digits,
  )
  let no_symbols = with_random_policy_options(
    standard, true, true, true, false, true, true,
  )
  @test.assert_eq(
    state_security_warning(no_symbols),
    security_warning_missing_symbols,
  )
}

///|
test "compatible profile intentionally permits a symbol-free pool" {
  let compatible = with_policy_profile(
    initial_state(),
    policy_profile_compatible,
  )
  @test.assert_eq(state_get(compatible, state_field_symbols), 0)
  @test.assert_eq(state_security_warning(compatible), security_warning_none)
  @test.assert_eq(state_configuration_valid(compatible), true)
}

///|
test "password display starts only after successful generation" {
  @test.assert_eq(password_display_begin(result_empty), password_display_empty)
  @test.assert_eq(
    password_display_begin(result_failure),
    password_display_empty,
  )
  @test.assert_eq(
    password_display_begin(result_success),
    password_display_visible,
  )
  @test.assert_eq(password_display_is_visible(password_display_visible), true)
  @test.assert_eq(password_display_is_visible(password_display_masked), false)
}

///|
test "password display masks at one minute" {
  @test.assert_eq(
    password_display_update(
      password_display_visible,
      result_success,
      password_display_timeout_ms - 1UL,
    ),
    password_display_visible,
  )
  @test.assert_eq(
    password_display_update(
      password_display_visible, result_success, password_display_timeout_ms,
    ),
    password_display_masked,
  )
}

///|
test "masked password cannot become visible without regeneration" {
  @test.assert_eq(
    password_display_update(password_display_masked, result_success, 0UL),
    password_display_masked,
  )
  @test.assert_eq(
    password_display_update(password_display_visible, result_failure, 0UL),
    password_display_empty,
  )
  @test.assert_eq(password_display_mask_width, 12)
  @test.assert_eq(password_display_mask_character, 42)
}

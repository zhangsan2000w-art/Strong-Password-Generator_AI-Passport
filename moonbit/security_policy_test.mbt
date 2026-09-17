///|
fn fixed_candidate(values : Array[Int]) -> FixedArray[Int] {
  let result = FixedArray::make(values.length(), 0)
  let mut index = 0
  while index < values.length() {
    result[index] = values[index]
    index = index + 1
  }
  result
}

///|
test "ASCII classifiers have exact boundaries" {
  @test.assert_eq(is_ascii_lower(96), false)
  @test.assert_eq(is_ascii_lower(97), true)
  @test.assert_eq(is_ascii_lower(122), true)
  @test.assert_eq(is_ascii_lower(123), false)
  @test.assert_eq(is_ascii_upper(64), false)
  @test.assert_eq(is_ascii_upper(65), true)
  @test.assert_eq(is_ascii_upper(90), true)
  @test.assert_eq(is_ascii_upper(91), false)
  @test.assert_eq(is_ascii_digit(47), false)
  @test.assert_eq(is_ascii_digit(48), true)
  @test.assert_eq(is_ascii_digit(57), true)
  @test.assert_eq(is_ascii_digit(58), false)
}

///|
test "allowed symbols match the product policy" {
  @test.assert_eq(is_allowed_symbol(33), true)
  @test.assert_eq(is_allowed_symbol(64), true)
  @test.assert_eq(is_allowed_symbol(63), true)
  @test.assert_eq(is_allowed_symbol(34), false)
  @test.assert_eq(is_allowed_symbol(92), false)
  @test.assert_eq(is_safe_symbol(33), true)
  @test.assert_eq(is_safe_symbol(63), true)
  @test.assert_eq(is_safe_symbol(94), false)
  @test.assert_eq(is_safe_symbol(43), false)
}

///|
test "ambiguous character policy covers visual collisions" {
  @test.assert_eq(is_ambiguous_character(48), true) // 0
  @test.assert_eq(is_ambiguous_character(79), true) // O
  @test.assert_eq(is_ambiguous_character(49), true) // 1
  @test.assert_eq(is_ambiguous_character(73), true) // I
  @test.assert_eq(is_ambiguous_character(108), true) // l
  @test.assert_eq(is_ambiguous_character(105), false)
}

///|
test "random postcondition requires enabled classes" {
  let valid = fixed_candidate([65, 98, 99, 100, 49, 33])
  let no_digit = fixed_candidate([65, 98, 99, 100, 101, 33])
  let no_symbol = fixed_candidate([65, 98, 99, 100, 49, 102])
  @test.assert_eq(random_candidate_valid(valid, true, true), true)
  @test.assert_eq(random_candidate_valid(no_digit, true, true), false)
  @test.assert_eq(random_candidate_valid(no_symbol, true, true), false)
}

///|
test "random postcondition rejects disabled or unknown characters" {
  let digit = fixed_candidate([65, 98, 99, 100, 49, 102])
  let symbol = fixed_candidate([65, 98, 99, 100, 33, 102])
  let space = fixed_candidate([65, 98, 99, 100, 32, 102])
  @test.assert_eq(random_candidate_valid(digit, false, false), false)
  @test.assert_eq(random_candidate_valid(symbol, true, false), false)
  @test.assert_eq(random_candidate_valid(space, true, true), false)
}

///|
test "policy postcondition validates class and filtering constraints" {
  let valid = fixed_candidate([97, 66, 50, 33, 99, 68, 51, 63])
  let ambiguous = fixed_candidate([97, 66, 48, 33, 99, 68, 51, 63])
  let unsafe_symbol = fixed_candidate([97, 66, 50, 94, 99, 68, 51, 63])
  @test.assert_eq(
    random_candidate_valid_with_policy(
      valid, true, true, true, true, true, true,
    ),
    true,
  )
  @test.assert_eq(
    random_candidate_valid_with_policy(
      ambiguous, true, true, true, true, true, true,
    ),
    false,
  )
  @test.assert_eq(
    random_candidate_valid_with_policy(
      unsafe_symbol, true, true, true, true, true, true,
    ),
    false,
  )
}

///|
test "PIN postcondition rejects length and non-digits" {
  @test.assert_eq(pin_candidate_valid(fixed_candidate([49, 50, 51, 52])), true)
  @test.assert_eq(pin_candidate_valid(fixed_candidate([49, 50, 51])), false)
  @test.assert_eq(pin_candidate_valid(fixed_candidate([49, 50, 65, 52])), false)
}

///|
test "dictionary characters remain printable lowercase ASCII" {
  @test.assert_eq(dictionary_character_valid(97), true)
  @test.assert_eq(dictionary_character_valid(122), true)
  @test.assert_eq(dictionary_character_valid(65), false)
  @test.assert_eq(dictionary_character_valid(45), false)
  @test.assert_eq(uppercase_ascii(97), 65)
  @test.assert_eq(uppercase_ascii(65), 65)
}

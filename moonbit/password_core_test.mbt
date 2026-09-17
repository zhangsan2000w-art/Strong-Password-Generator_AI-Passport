///|
struct DeterministicSource {
  values : Array[UInt]
  mut cursor : Int
}

///|
fn DeterministicSource::next(self : DeterministicSource) -> UInt {
  let value = self.values[self.cursor % self.values.length()]
  self.cursor = self.cursor + 1
  value
}

///|
struct Collector {
  values : Array[Int]
}

///|
fn Collector::emit(self : Collector, ch : Int) -> Bool {
  self.values.push(ch)
  true
}

///|
fn contains_digit(values : Array[Int]) -> Bool {
  for value in values {
    if value >= 48 && value <= 57 {
      return true
    }
  }
  false
}

///|
fn contains_lowercase(values : Array[Int]) -> Bool {
  for value in values {
    if is_ascii_lower(value) {
      return true
    }
  }
  false
}

///|
fn contains_uppercase(values : Array[Int]) -> Bool {
  for value in values {
    if is_ascii_upper(value) {
      return true
    }
  }
  false
}

///|
fn contains_ambiguous(values : Array[Int]) -> Bool {
  for value in values {
    if is_ambiguous_character(value) {
      return true
    }
  }
  false
}

///|
fn contains_unsafe_symbol(values : Array[Int]) -> Bool {
  for value in values {
    if is_allowed_symbol(value) && !is_safe_symbol(value) {
      return true
    }
  }
  false
}

///|
fn contains_symbol(values : Array[Int]) -> Bool {
  for value in values {
    if value == 33 ||
      value == 64 ||
      value == 35 ||
      value == 36 ||
      value == 37 ||
      value == 94 ||
      value == 38 ||
      value == 42 ||
      value == 95 ||
      value == 45 ||
      value == 43 ||
      value == 61 ||
      value == 63 {
      return true
    }
  }
  false
}

///|
fn all_digits(values : Array[Int]) -> Bool {
  for value in values {
    if value < 48 || value > 57 {
      return false
    }
  }
  true
}

///|
fn test_word_length(index : Int) -> Int {
  match index {
    0 => 5
    1 => 4
    _ => -1
  }
}

///|
fn test_word_char(index : Int, offset : Int) -> Int {
  match (index, offset) {
    (0, 0) => 97 // apple
    (0, 1) => 112
    (0, 2) => 112
    (0, 3) => 108
    (0, 4) => 101
    (1, 0) => 108 // lime
    (1, 1) => 105
    (1, 2) => 109
    (1, 3) => 101
    _ => -1
  }
}

///|
test "random password validates length boundaries" {
  let source = DeterministicSource::{ values: [0U], cursor: 0, }
  let output = Collector::{ values: [], }
  @test.assert_eq(
    generate_random_password(
      fn() { source.next() },
      fn(ch) { output.emit(ch) },
      5,
      true,
      false,
    ),
    generate_invalid_config,
  )
  @test.assert_eq(output.values.length(), 0)
  @test.assert_eq(
    generate_random_password(
      fn() { source.next() },
      fn(ch) { output.emit(ch) },
      6,
      true,
      false,
    ),
    generate_ok,
  )
  @test.assert_eq(output.values.length(), 6)
  let maximum = Collector::{ values: [], }
  @test.assert_eq(
    generate_random_password(
      fn() { source.next() },
      fn(ch) { maximum.emit(ch) },
      30,
      false,
      false,
    ),
    generate_ok,
  )
  @test.assert_eq(maximum.values.length(), 30)
  @test.assert_eq(
    generate_random_password(
      fn() { source.next() },
      fn(ch) { maximum.emit(ch) },
      31,
      false,
      false,
    ),
    generate_invalid_config,
  )
}

///|
test "random password enforces enabled character classes" {
  let source = DeterministicSource::{ values: [0U, 1U, 2U, 3U], cursor: 0, }
  let output = Collector::{ values: [], }
  @test.assert_eq(
    generate_random_password(
      fn() { source.next() },
      fn(ch) { output.emit(ch) },
      10,
      true,
      true,
    ),
    generate_ok,
  )
  @test.assert_eq(output.values.length(), 10)
  @test.assert_eq(contains_digit(output.values), true)
  @test.assert_eq(contains_symbol(output.values), true)
}

///|
test "policy generator enforces every class and filters risky characters" {
  let source = DeterministicSource::{
    values: [0U, 1U, 2U, 3U, 4U, 5U],
    cursor: 0,
  }
  let output = Collector::{ values: [], }
  @test.assert_eq(
    generate_random_password_with_policy(
      fn() { source.next() },
      fn(ch) { output.emit(ch) },
      16,
      true,
      true,
      true,
      true,
      true,
      true,
    ),
    generate_ok,
  )
  @test.assert_eq(output.values.length(), 16)
  @test.assert_eq(contains_lowercase(output.values), true)
  @test.assert_eq(contains_uppercase(output.values), true)
  @test.assert_eq(contains_digit(output.values), true)
  @test.assert_eq(contains_symbol(output.values), true)
  @test.assert_eq(contains_ambiguous(output.values), false)
  @test.assert_eq(contains_unsafe_symbol(output.values), false)
}

///|
test "policy generator supports individual class selection" {
  let source = DeterministicSource::{ values: [8U, 13U, 23U], cursor: 0, }
  let output = Collector::{ values: [], }
  @test.assert_eq(
    generate_random_password_with_policy(
      fn() { source.next() },
      fn(ch) { output.emit(ch) },
      8,
      false,
      true,
      false,
      false,
      true,
      true,
    ),
    generate_ok,
  )
  for value in output.values {
    @test.assert_eq(is_ascii_upper(value), true)
    @test.assert_eq(is_ambiguous_character(value), false)
  }
  @test.assert_eq(
    generate_random_password_with_policy(
      fn() { source.next() },
      fn(ch) { output.emit(ch) },
      8,
      false,
      false,
      false,
      false,
      true,
      true,
    ),
    generate_invalid_config,
  )
}

///|
test "random password omits disabled character classes" {
  let source = DeterministicSource::{ values: [53U, 61U, 74U], cursor: 0, }
  let output = Collector::{ values: [], }
  @test.assert_eq(
    generate_random_password(
      fn() { source.next() },
      fn(ch) { output.emit(ch) },
      16,
      false,
      false,
    ),
    generate_ok,
  )
  @test.assert_eq(contains_digit(output.values), false)
  @test.assert_eq(contains_symbol(output.values), false)
}

///|
test "unbiased mapping rejects the incomplete modulo bucket" {
  let source = DeterministicSource::{ values: [0xffffffffU, 7U], cursor: 0, }
  @test.assert_eq(unbiased_index(fn() { source.next() }, 10), 7)
  @test.assert_eq(source.cursor, 2)
  @test.assert_eq(unbiased_index(fn() { source.next() }, 1), 0)
  @test.assert_eq(unbiased_index(fn() { source.next() }, 0), -1)
}

///|
test "pin output is numeric and validates boundaries" {
  let source = DeterministicSource::{ values: [0U, 9U, 10U], cursor: 0, }
  let minimum = Collector::{ values: [], }
  @test.assert_eq(
    generate_pin(fn() { source.next() }, fn(ch) { minimum.emit(ch) }, 4),
    generate_ok,
  )
  @test.assert_eq(minimum.values.length(), 4)
  @test.assert_eq(all_digits(minimum.values), true)
  let maximum = Collector::{ values: [], }
  @test.assert_eq(
    generate_pin(fn() { source.next() }, fn(ch) { maximum.emit(ch) }, 12),
    generate_ok,
  )
  @test.assert_eq(maximum.values.length(), 12)
  @test.assert_eq(all_digits(maximum.values), true)
  @test.assert_eq(
    generate_pin(fn() { source.next() }, fn(ch) { maximum.emit(ch) }, 3),
    generate_invalid_config,
  )
  @test.assert_eq(
    generate_pin(fn() { source.next() }, fn(ch) { maximum.emit(ch) }, 13),
    generate_invalid_config,
  )
}

///|
test "passphrase applies count separator capitalization and abbreviation" {
  let source = DeterministicSource::{ values: [0U, 1U], cursor: 0, }
  let output = Collector::{ values: [], }
  @test.assert_eq(
    generate_passphrase(
      fn() { source.next() },
      fn(ch) { output.emit(ch) },
      2,
      test_word_length,
      test_word_char,
      4,
      true,
      false,
      2,
    ),
    generate_ok,
  )
  @test.assert_eq(output.values.length(), 19)
  @test.assert_eq(output.values[0], 65)
  @test.assert_eq(output.values[4], 95)
  @test.assert_eq(output.values[5], 76)
  @test.assert_eq(output.values[9], 95)
  @test.assert_eq(output.values[10], 65)
  @test.assert_eq(output.values[14], 95)
  @test.assert_eq(output.values[15], 76)
}

///|
test "passphrase rejects invalid configuration and dictionary indexes" {
  let source = DeterministicSource::{ values: [0U], cursor: 0, }
  let output = Collector::{ values: [], }
  @test.assert_eq(
    generate_passphrase(
      fn() { source.next() },
      fn(ch) { output.emit(ch) },
      2,
      test_word_length,
      test_word_char,
      2,
      false,
      true,
      0,
    ),
    generate_invalid_config,
  )
  @test.assert_eq(
    generate_passphrase(
      fn() { source.next() },
      fn(ch) { output.emit(ch) },
      0,
      test_word_length,
      test_word_char,
      4,
      false,
      true,
      0,
    ),
    generate_invalid_config,
  )
}

///|
test "state switches modes edits toggles generates and regenerates" {
  let mut state = initial_state()
  @test.assert_eq(state_get(state, state_field_mode), 0)
  @test.assert_eq(state_get(state, state_field_random_length), 16)
  @test.assert_eq(
    state_get(state, state_field_policy_profile),
    policy_profile_standard,
  )
  @test.assert_eq(state_get(state, state_field_lowercase), 1)
  @test.assert_eq(state_get(state, state_field_uppercase), 1)
  @test.assert_eq(state_get(state, state_field_exclude_ambiguous), 1)
  state = handle_input(state, input_down)
  @test.assert_eq(state_get(state, state_field_mode), 1)
  state = handle_input(state, input_up)
  @test.assert_eq(state_get(state, state_field_mode), 0)
  state = handle_input(state, input_ok)
  @test.assert_eq(state_get(state, state_field_focus), 1)
  state = handle_input(state, input_ok)
  @test.assert_eq(state_get(state, state_field_editing), 1)
  state = handle_input(state, input_up)
  @test.assert_eq(state_get(state, state_field_random_length), 17)
  state = handle_input(state, input_ok_long)
  @test.assert_eq(state_get(state, state_field_editing), 0)
  @test.assert_eq(state_get(state, state_field_random_length), 16)
  state = handle_input(state, input_down)
  @test.assert_eq(state_get(state, state_field_focus), 2)
  state = handle_input(state, input_ok)
  @test.assert_eq(state_get(state, state_field_digits), 0)
  state = handle_input(state, input_down)
  state = handle_input(state, input_down)
  @test.assert_eq(state_get(state, state_field_focus), 4)
  state = handle_input(state, input_ok)
  @test.assert_eq(state_action(state), action_generate)
  state = handle_input(state, input_ok)
  @test.assert_eq(state_action(state), action_generate)
  state = handle_input(state, input_ok_long)
  @test.assert_eq(state_action(state), action_none)
  @test.assert_eq(state_get(state, state_field_focus), 4)
}

///|
test "repeated editing input clamps password length" {
  let mut state = initial_state()
  state = handle_input(state, input_ok)
  state = handle_input(state, input_ok)
  @test.assert_eq(state_get(state, state_field_editing), 1)
  let mut repeats = 0
  while repeats < 20 {
    state = handle_input(state, input_down)
    repeats = repeats + 1
  }
  @test.assert_eq(
    state_get(state, state_field_random_length),
    policy_profile_min_length(policy_profile_standard),
  )
  state = handle_input(state, input_ok)
  @test.assert_eq(state_get(state, state_field_editing), 0)
}

///|
test "entropy estimates respect mode bounds" {
  @test.assert_eq(random_entropy_bits_x10(5, true, false), 0)
  @test.assert_eq(random_entropy_bits_x10(10, true, false) > 500, true)
  @test.assert_eq(pin_entropy_bits_x10(3), 0)
  @test.assert_eq(pin_entropy_bits_x10(6) > 190, true)
  @test.assert_eq(passphrase_entropy_bits_x10(4, true) > 400, true)
  @test.assert_eq(
    passphrase_entropy_bits_x10(4, true) > passphrase_entropy_bits_x10(4, false),
    true,
  )
}

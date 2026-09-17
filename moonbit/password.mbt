///|
pub let generate_ok : Int = 0

///|
pub let generate_invalid_config : Int = -1

///|
pub let generate_output_failed : Int = -2

///|
pub fn generate_random_password(
  next_u32 : () -> UInt,
  emit : (Int) -> Bool,
  length : Int,
  digits : Bool,
  symbols : Bool,
) -> Int {
  generate_random_password_with_policy(
    next_u32, emit, length, true, true, digits, symbols, false, false,
  )
}

///|
pub fn generate_random_password_with_policy(
  next_u32 : () -> UInt,
  emit : (Int) -> Bool,
  length : Int,
  lowercase : Bool,
  uppercase : Bool,
  digits : Bool,
  symbols : Bool,
  exclude_ambiguous : Bool,
  safe_symbols_only : Bool,
) -> Int {
  if !random_policy_valid(length, lowercase, uppercase, digits, symbols) {
    return generate_invalid_config
  }
  let output = FixedArray::make(length, 0)
  let mut cursor = 0
  if lowercase {
    output[cursor] = lowercase_at(
      unbiased_index(next_u32, lowercase_count(exclude_ambiguous)),
      exclude_ambiguous,
    )
    cursor = cursor + 1
  }
  if uppercase {
    output[cursor] = uppercase_at(
      unbiased_index(next_u32, uppercase_count(exclude_ambiguous)),
      exclude_ambiguous,
    )
    cursor = cursor + 1
  }
  if digits {
    output[cursor] = digit_at(
      unbiased_index(next_u32, digit_count(exclude_ambiguous)),
      exclude_ambiguous,
    )
    cursor = cursor + 1
  }
  if symbols {
    output[cursor] = policy_symbol_at(
      unbiased_index(next_u32, policy_symbol_count(safe_symbols_only)),
      safe_symbols_only,
    )
    cursor = cursor + 1
  }
  let pool_size = random_pool_size(
    lowercase, uppercase, digits, symbols, exclude_ambiguous, safe_symbols_only,
  )
  while cursor < length {
    output[cursor] = random_pool_at(
      unbiased_index(next_u32, pool_size),
      lowercase,
      uppercase,
      digits,
      symbols,
      exclude_ambiguous,
      safe_symbols_only,
    )
    cursor = cursor + 1
  }
  let mut i = length - 1
  while i > 0 {
    let j = unbiased_index(next_u32, i + 1)
    let temporary = output[i]
    output[i] = output[j]
    output[j] = temporary
    i = i - 1
  }
  if !random_candidate_valid_with_policy(
      output, lowercase, uppercase, digits, symbols, exclude_ambiguous, safe_symbols_only,
    ) {
    return generate_invalid_config
  }
  i = 0
  while i < length {
    if !emit(output[i]) {
      return generate_output_failed
    }
    i = i + 1
  }
  generate_ok
}

///|
pub fn generate_pin(
  next_u32 : () -> UInt,
  emit : (Int) -> Bool,
  length : Int,
) -> Int {
  if length < pin_min_length || length > pin_max_length {
    return generate_invalid_config
  }
  let output = FixedArray::make(length, 0)
  let mut i = 0
  while i < length {
    output[i] = 48 + unbiased_index(next_u32, 10)
    i = i + 1
  }
  if !pin_candidate_valid(output) {
    return generate_invalid_config
  }
  i = 0
  while i < length {
    if !emit(output[i]) {
      return generate_output_failed
    }
    i = i + 1
  }
  generate_ok
}

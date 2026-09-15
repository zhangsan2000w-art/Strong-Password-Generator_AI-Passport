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
  if length < random_min_length || length > random_max_length {
    return generate_invalid_config
  }
  let output = FixedArray::make(length, 0)
  let mut cursor = 0
  if digits {
    output[cursor] = 48 + unbiased_index(next_u32, 10)
    cursor = cursor + 1
  }
  if symbols {
    output[cursor] = symbol_at(unbiased_index(next_u32, symbol_count))
    cursor = cursor + 1
  }
  let pool_size = random_pool_size(digits, symbols)
  while cursor < length {
    output[cursor] = random_pool_at(
      unbiased_index(next_u32, pool_size),
      digits,
      symbols,
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
  if !random_candidate_valid(output, digits, symbols) {
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

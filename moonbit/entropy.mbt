///|
pub fn random_entropy_bits_x10(
  length : Int,
  digits : Bool,
  symbols : Bool,
) -> Int {
  random_entropy_bits_x10_with_policy(
    length, true, true, digits, symbols, false, false,
  )
}

///|
/// Integer binary logarithm in thousandths. Ten squaring steps retain enough
/// precision for the one-decimal-bit product display without floating point.
pub fn log2_milli(value : Int) -> Int {
  if value <= 1 {
    return 0
  }
  let mut integer = 0
  let mut base = 1
  while base * 2 <= value {
    base = base * 2
    integer = integer + 1
  }
  let mut normalized = value.to_uint64() * 65536UL / base.to_uint64()
  let mut fraction_1024 = 0
  let mut step = 0
  while step < 10 {
    normalized = normalized * normalized / 65536UL
    if normalized >= 131072UL {
      normalized = normalized / 2UL
      fraction_1024 = fraction_1024 + (1 << (9 - step))
    }
    step = step + 1
  }
  integer * 1000 + fraction_1024 * 1000 / 1024
}

///|
pub fn random_entropy_bits_x10_with_policy(
  length : Int,
  lowercase : Bool,
  uppercase : Bool,
  digits : Bool,
  symbols : Bool,
  exclude_ambiguous : Bool,
  safe_symbols_only : Bool,
) -> Int {
  if !random_policy_valid(length, lowercase, uppercase, digits, symbols) {
    return 0
  }
  let pool_size = random_pool_size(
    lowercase, uppercase, digits, symbols, exclude_ambiguous, safe_symbols_only,
  )
  let milli_bits_per_character = log2_milli(pool_size)
  length * milli_bits_per_character / 100
}

///|
pub fn pin_entropy_bits_x10(length : Int) -> Int {
  if length < pin_min_length || length > pin_max_length {
    0
  } else {
    length * 3322 / 100 // log2(10)
  }
}

///|
pub fn passphrase_entropy_bits_x10(words : Int, complete_word : Bool) -> Int {
  if words < passphrase_min_words || words > passphrase_max_words {
    return 0
  }
  // The bundled EFF list has 1296 entries. Four-character prefixes have a
  // worst-case multiplicity of three, so abbreviated output is reported with
  // a conservative min-entropy estimate of log2(1296 / 3).
  let milli_bits_per_word = if complete_word { 10340 } else { 8755 }
  words * milli_bits_per_word / 100
}

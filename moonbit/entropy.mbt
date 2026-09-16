///|
pub fn random_entropy_bits_x10(
  length : Int,
  digits : Bool,
  symbols : Bool,
) -> Int {
  if length < random_min_length || length > random_max_length {
    return 0
  }
  let milli_bits_per_character = if digits && symbols {
    6229 // log2(75)
  } else if digits {
    5954 // log2(62)
  } else if symbols {
    6022 // log2(65)
  } else {
    5700 // log2(52)
  }
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

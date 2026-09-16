///|
/// Character classification and postcondition checks shared by generation,
/// tests, and future adapters. These are security rules, not display details.
pub fn is_ascii_lower(ch : Int) -> Bool {
  ch >= 97 && ch <= 122
}

///|
pub fn is_ascii_upper(ch : Int) -> Bool {
  ch >= 65 && ch <= 90
}

///|
pub fn is_ascii_letter(ch : Int) -> Bool {
  is_ascii_lower(ch) || is_ascii_upper(ch)
}

///|
pub fn is_ascii_digit(ch : Int) -> Bool {
  ch >= 48 && ch <= 57
}

///|
pub fn is_allowed_symbol(ch : Int) -> Bool {
  let mut index = 0
  while index < symbol_count {
    if symbol_at(index) == ch {
      return true
    }
    index = index + 1
  }
  false
}

///|
pub fn is_allowed_random_character(
  ch : Int,
  digits : Bool,
  symbols : Bool,
) -> Bool {
  is_ascii_letter(ch) ||
  (digits && is_ascii_digit(ch)) ||
  (symbols && is_allowed_symbol(ch))
}

///|
pub fn random_candidate_valid(
  output : FixedArray[Int],
  digits : Bool,
  symbols : Bool,
) -> Bool {
  if output.length() < random_min_length || output.length() > random_max_length {
    return false
  }
  let mut has_digit = false
  let mut has_symbol = false
  let mut index = 0
  while index < output.length() {
    let ch = output[index]
    if !is_allowed_random_character(ch, digits, symbols) {
      return false
    }
    if is_ascii_digit(ch) {
      has_digit = true
    }
    if is_allowed_symbol(ch) {
      has_symbol = true
    }
    index = index + 1
  }
  (!digits || has_digit) && (!symbols || has_symbol)
}

///|
pub fn pin_candidate_valid(output : FixedArray[Int]) -> Bool {
  if output.length() < pin_min_length || output.length() > pin_max_length {
    return false
  }
  let mut index = 0
  while index < output.length() {
    if !is_ascii_digit(output[index]) {
      return false
    }
    index = index + 1
  }
  true
}

///|
pub fn dictionary_character_valid(ch : Int) -> Bool {
  is_ascii_lower(ch)
}

///|
pub fn uppercase_ascii(ch : Int) -> Int {
  if is_ascii_lower(ch) {
    ch - 32
  } else {
    ch
  }
}

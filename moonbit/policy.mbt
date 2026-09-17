///|
pub let random_min_length : Int = 6

///|
pub let random_max_length : Int = 30

///|
pub let pin_min_length : Int = 4

///|
pub let pin_max_length : Int = 12

///|
pub let passphrase_min_words : Int = 3

///|
pub let passphrase_max_words : Int = 6

///|
pub let symbol_count : Int = 13

///|
pub let safe_symbol_count : Int = 9

///|
fn symbol_at(index : Int) -> Int {
  match index {
    0 => 33 // !
    1 => 64 // @
    2 => 35 // #
    3 => 36 // $
    4 => 37 // %
    5 => 94 // ^
    6 => 38 // &
    7 => 42 // *
    8 => 95 // _
    9 => 45 // -
    10 => 43 // +
    11 => 61 // =
    12 => 63 // ?
    _ => -1
  }
}

///|
fn safe_symbol_at(index : Int) -> Int {
  match index {
    0 => 33 // !
    1 => 64 // @
    2 => 35 // #
    3 => 36 // $
    4 => 37 // %
    5 => 42 // *
    6 => 95 // _
    7 => 45 // -
    8 => 63 // ?
    _ => -1
  }
}

///|
pub fn is_ambiguous_character(ch : Int) -> Bool {
  ch == 48 || ch == 79 || ch == 49 || ch == 73 || ch == 108
}

///|
fn lowercase_count(exclude_ambiguous : Bool) -> Int {
  if exclude_ambiguous {
    25
  } else {
    26
  }
}

///|
fn lowercase_at(index : Int, exclude_ambiguous : Bool) -> Int {
  if exclude_ambiguous && index >= 11 {
    98 + index // Skip lowercase l.
  } else {
    97 + index
  }
}

///|
fn uppercase_count(exclude_ambiguous : Bool) -> Int {
  if exclude_ambiguous {
    24
  } else {
    26
  }
}

///|
fn uppercase_at(index : Int, exclude_ambiguous : Bool) -> Int {
  if !exclude_ambiguous {
    return 65 + index
  }
  if index < 8 {
    65 + index
  } else if index < 13 {
    66 + index // Skip uppercase I.
  } else {
    67 + index // Skip uppercase O as well.
  }
}

///|
fn digit_count(exclude_ambiguous : Bool) -> Int {
  if exclude_ambiguous {
    8
  } else {
    10
  }
}

///|
fn digit_at(index : Int, exclude_ambiguous : Bool) -> Int {
  if exclude_ambiguous {
    50 + index
  } else {
    48 + index
  }
}

///|
fn policy_symbol_count(safe_symbols_only : Bool) -> Int {
  if safe_symbols_only {
    safe_symbol_count
  } else {
    symbol_count
  }
}

///|
fn policy_symbol_at(index : Int, safe_symbols_only : Bool) -> Int {
  if safe_symbols_only {
    safe_symbol_at(index)
  } else {
    symbol_at(index)
  }
}

///|
pub fn random_required_class_count(
  lowercase : Bool,
  uppercase : Bool,
  digits : Bool,
  symbols : Bool,
) -> Int {
  (if lowercase { 1 } else { 0 }) +
  (if uppercase { 1 } else { 0 }) +
  (if digits { 1 } else { 0 }) +
  (if symbols { 1 } else { 0 })
}

///|
pub fn random_policy_valid(
  length : Int,
  lowercase : Bool,
  uppercase : Bool,
  digits : Bool,
  symbols : Bool,
) -> Bool {
  let required = random_required_class_count(
    lowercase, uppercase, digits, symbols,
  )
  length >= random_min_length &&
  length <= random_max_length &&
  required > 0 &&
  length >= required
}

///|
fn random_pool_size(
  lowercase : Bool,
  uppercase : Bool,
  digits : Bool,
  symbols : Bool,
  exclude_ambiguous : Bool,
  safe_symbols_only : Bool,
) -> Int {
  (if lowercase { lowercase_count(exclude_ambiguous) } else { 0 }) +
  (if uppercase { uppercase_count(exclude_ambiguous) } else { 0 }) +
  (if digits { digit_count(exclude_ambiguous) } else { 0 }) +
  (if symbols { policy_symbol_count(safe_symbols_only) } else { 0 })
}

///|
fn random_pool_at(
  original_index : Int,
  lowercase : Bool,
  uppercase : Bool,
  digits : Bool,
  _symbols : Bool,
  exclude_ambiguous : Bool,
  safe_symbols_only : Bool,
) -> Int {
  let mut index = original_index
  if lowercase {
    let count = lowercase_count(exclude_ambiguous)
    if index < count {
      return lowercase_at(index, exclude_ambiguous)
    }
    index = index - count
  }
  if uppercase {
    let count = uppercase_count(exclude_ambiguous)
    if index < count {
      return uppercase_at(index, exclude_ambiguous)
    }
    index = index - count
  }
  if digits {
    let count = digit_count(exclude_ambiguous)
    if index < count {
      return digit_at(index, exclude_ambiguous)
    }
    index = index - count
  }
  policy_symbol_at(index, safe_symbols_only)
}

///|
fn separator_at(index : Int) -> Int {
  match index {
    0 => 45 // -
    1 => 46 // .
    2 => 95 // _
    _ => -1
  }
}

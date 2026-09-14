///|
pub let random_min_length : Int = 8

///|
pub let random_max_length : Int = 64

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
fn letter_at(index : Int) -> Int {
  if index < 26 {
    97 + index
  } else {
    65 + index - 26
  }
}

///|
fn random_pool_size(digits : Bool, symbols : Bool) -> Int {
  52 + (if digits { 10 } else { 0 }) + (if symbols { symbol_count } else { 0 })
}

///|
fn random_pool_at(index : Int, digits : Bool, _symbols : Bool) -> Int {
  if index < 52 {
    letter_at(index)
  } else if digits && index < 62 {
    48 + index - 52
  } else {
    symbol_at(index - 52 - (if digits { 10 } else { 0 }))
  }
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

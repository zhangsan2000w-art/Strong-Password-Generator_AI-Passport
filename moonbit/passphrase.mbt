///|
pub fn generate_passphrase(
  next_u32 : () -> UInt,
  emit : (Int) -> Bool,
  dictionary_count : Int,
  dictionary_length : (Int) -> Int,
  dictionary_char : (Int, Int) -> Int,
  word_count : Int,
  capitalize : Bool,
  complete_word : Bool,
  separator_index : Int,
) -> Int {
  if word_count < passphrase_min_words ||
    word_count > passphrase_max_words ||
    dictionary_count <= 0 ||
    separator_at(separator_index) < 0 {
    return generate_invalid_config
  }
  let separator = separator_at(separator_index)
  let mut word_number = 0
  while word_number < word_count {
    let word_index = unbiased_index(next_u32, dictionary_count)
    let source_length = dictionary_length(word_index)
    if source_length <= 0 {
      return generate_invalid_config
    }
    let output_length = if complete_word || source_length <= 4 {
      source_length
    } else {
      4
    }
    let mut character_index = 0
    while character_index < output_length {
      let mut ch = dictionary_char(word_index, character_index)
      if !dictionary_character_valid(ch) {
        return generate_invalid_config
      }
      if capitalize && character_index == 0 {
        ch = uppercase_ascii(ch)
      }
      if !emit(ch) {
        return generate_output_failed
      }
      character_index = character_index + 1
    }
    if word_number + 1 < word_count && !emit(separator) {
      return generate_output_failed
    }
    word_number = word_number + 1
  }
  generate_ok
}

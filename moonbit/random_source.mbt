///|
/// Map a full-width UInt sample into [0, bound) without modulo bias.
/// Rejected values are drawn again. The callback is the RandomSource boundary:
/// tests provide deterministic values and firmware provides ESP-IDF entropy.
pub fn unbiased_index(next_u32 : () -> UInt, bound : Int) -> Int {
  if bound <= 0 {
    return -1
  }
  if bound == 1 {
    return 0
  }
  let divisor = bound.reinterpret_as_uint()
  let limit = 0xffffffffU - 0xffffffffU % divisor
  let mut sample = next_u32()
  while sample >= limit {
    sample = next_u32()
  }
  (sample % divisor).reinterpret_as_int()
}

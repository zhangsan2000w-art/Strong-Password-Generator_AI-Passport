///|
/// Product-owned success chime. C schedules playback and writes PCM to the
/// codec; MoonBit owns the audible sequence and every generated sample.
pub let sound_sample_rate : Int = 16000

///|
pub let sound_volume_percent : Int = 45

///|
pub let sound_note_count : Int = 2

///|
pub let sound_attack_samples : Int = 80

///|
pub let sound_release_samples : Int = 120

///|
pub fn sound_note_frequency(note : Int) -> Int {
  match note {
    0 => 880
    1 => 1320
    _ => 0
  }
}

///|
pub fn sound_note_duration_ms(note : Int) -> Int {
  match note {
    0 => 70
    1 => 90
    _ => 0
  }
}

///|
pub fn sound_note_sample_count(note : Int) -> Int {
  sound_note_duration_ms(note) * sound_sample_rate / 1000
}

///|
fn sine_32(index : Int) -> Int {
  match index {
    0 => 0
    1 => 975
    2 => 1913
    3 => 2778
    4 => 3536
    5 => 4157
    6 => 4619
    7 => 4904
    8 => 5000
    9 => 4904
    10 => 4619
    11 => 4157
    12 => 3536
    13 => 2778
    14 => 1913
    15 => 975
    16 => 0
    17 => -975
    18 => -1913
    19 => -2778
    20 => -3536
    21 => -4157
    22 => -4619
    23 => -4904
    24 => -5000
    25 => -4904
    26 => -4619
    27 => -4157
    28 => -3536
    29 => -2778
    30 => -1913
    31 => -975
    _ => 0
  }
}

///|
pub fn sound_envelope_percent(note : Int, sample_index : Int) -> Int {
  let total = sound_note_sample_count(note)
  if sample_index < 0 || sample_index >= total {
    return 0
  }
  let remaining = total - sample_index
  let attack = if sample_index < sound_attack_samples {
    sample_index * 100 / sound_attack_samples
  } else {
    100
  }
  let release = if remaining < sound_release_samples {
    remaining * 100 / sound_release_samples
  } else {
    100
  }
  if attack < release {
    attack
  } else {
    release
  }
}

///|
pub fn sound_sample(note : Int, sample_index : Int) -> Int {
  let frequency = sound_note_frequency(note)
  let total = sound_note_sample_count(note)
  if frequency <= 0 || sample_index < 0 || sample_index >= total {
    return 0
  }
  let phase_step = frequency * 65536 / sound_sample_rate
  let table_index = sample_index * phase_step / 2048 % 32
  sine_32(table_index) * sound_envelope_percent(note, sample_index) / 100
}

///|
pub fn sound_total_duration_ms() -> Int {
  let mut total = 0
  let mut note = 0
  while note < sound_note_count {
    total = total + sound_note_duration_ms(note)
    note = note + 1
  }
  total
}

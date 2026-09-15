///|
test "battery keeps valid CW2017 SOC authoritative" {
  let reading = battery_resolve(77, 3900)
  @test.assert_eq(battery_reading_available(reading), true)
  @test.assert_eq(battery_reading_estimated(reading), false)
  @test.assert_eq(battery_reading_percent(reading), 77)
  @test.assert_eq(battery_reading_low(reading), false)
}

///|
test "battery rejects implausible zero SOC at healthy voltage" {
  let reading = battery_resolve(0, 4000)
  @test.assert_eq(battery_reading_available(reading), true)
  @test.assert_eq(battery_reading_estimated(reading), true)
  @test.assert_eq(battery_reading_percent(reading), 78)
}

///|
test "battery voltage fallback clamps endpoints" {
  let empty = battery_resolve(-1, 3200)
  let full = battery_resolve(-1, 4300)
  @test.assert_eq(battery_reading_percent(empty), 0)
  @test.assert_eq(battery_reading_low(empty), true)
  @test.assert_eq(battery_reading_percent(full), 100)
  @test.assert_eq(battery_reading_low(full), false)
}

///|
test "battery reports unavailable only when both sources are unusable" {
  let unavailable = battery_resolve(-1, -1)
  @test.assert_eq(battery_reading_available(unavailable), false)
  @test.assert_eq(battery_reading_estimated(unavailable), false)
  @test.assert_eq(battery_reading_percent(unavailable), -1)
}

///|
test "battery accepts genuine empty SOC at empty-cell voltage" {
  let reading = battery_resolve(0, 3300)
  @test.assert_eq(battery_reading_available(reading), true)
  @test.assert_eq(battery_reading_estimated(reading), false)
  @test.assert_eq(battery_reading_percent(reading), 0)
}

///|
test "success sound declares a compact two-note sequence" {
  @test.assert_eq(sound_sample_rate, 16000)
  @test.assert_eq(sound_volume_percent, 45)
  @test.assert_eq(sound_note_count, 2)
  @test.assert_eq(sound_note_frequency(0), 880)
  @test.assert_eq(sound_note_frequency(1), 1320)
  @test.assert_eq(sound_total_duration_ms(), 160)
}

///|
test "success sound sample counts follow duration and sample rate" {
  @test.assert_eq(sound_note_sample_count(0), 1120)
  @test.assert_eq(sound_note_sample_count(1), 1440)
  @test.assert_eq(sound_note_sample_count(2), 0)
}

///|
test "success sound envelope fades in and out" {
  let total = sound_note_sample_count(0)
  @test.assert_eq(sound_envelope_percent(0, 0), 0)
  @test.assert_eq(sound_envelope_percent(0, 40), 50)
  @test.assert_eq(sound_envelope_percent(0, 100), 100)
  @test.assert_eq(sound_envelope_percent(0, total - 60), 50)
  @test.assert_eq(sound_envelope_percent(0, total), 0)
}

///|
test "success sound waveform is bounded and non-silent" {
  let total = sound_note_sample_count(1)
  let mut peak = 0
  let mut index = 0
  while index < total {
    let sample = sound_sample(1, index)
    let magnitude = if sample < 0 { -sample } else { sample }
    if magnitude > peak {
      peak = magnitude
    }
    @test.assert_eq(magnitude <= 5000, true)
    index = index + 1
  }
  @test.assert_eq(peak > 4000, true)
  @test.assert_eq(sound_sample(9, 0), 0)
  @test.assert_eq(sound_sample(0, -1), 0)
}

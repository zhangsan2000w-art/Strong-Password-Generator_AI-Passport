///|
/// Pure battery presentation policy. The ESP32 adapter only supplies raw
/// CW2017 SOC and millivolts; MoonBit decides what is safe to show.
pub let battery_valid_min_mv : Int = 2500

///|
pub let battery_valid_max_mv : Int = 5000

///|
pub let battery_empty_mv : Int = 3300

///|
pub let battery_full_mv : Int = 4200

///|
pub let battery_zero_implausible_mv : Int = 3500

///|
pub let battery_low_percent : Int = 20

///|
pub let battery_refresh_interval_ms : Int = 10000

///|
pub let battery_flag_available : Int = 1

///|
pub let battery_flag_estimated : Int = 2

///|
pub fn battery_voltage_valid(millivolts : Int) -> Bool {
  millivolts >= battery_valid_min_mv && millivolts <= battery_valid_max_mv
}

///|
pub fn battery_soc_valid(percent : Int) -> Bool {
  percent >= 0 && percent <= 100
}

///|
pub fn battery_soc_implausible(percent : Int, millivolts : Int) -> Bool {
  percent == 0 &&
  battery_voltage_valid(millivolts) &&
  millivolts >= battery_zero_implausible_mv
}

///|
pub fn battery_estimate_percent(millivolts : Int) -> Int {
  if millivolts <= battery_empty_mv {
    0
  } else if millivolts >= battery_full_mv {
    100
  } else {
    let span = battery_full_mv - battery_empty_mv
    ((millivolts - battery_empty_mv) * 100 + span / 2) / span
  }
}

///|
/// Packed result layout: bit 0 available, bit 1 estimated, remaining bits are
/// percentage. Zero therefore means unavailable and never looks like a valid
/// empty battery.
pub fn battery_resolve(percent : Int, millivolts : Int) -> Int {
  let voltage_valid = battery_voltage_valid(millivolts)
  let soc_usable = battery_soc_valid(percent) &&
    !battery_soc_implausible(percent, millivolts)
  if soc_usable {
    percent * 4 + battery_flag_available
  } else if voltage_valid {
    battery_estimate_percent(millivolts) * 4 +
    battery_flag_available +
    battery_flag_estimated
  } else {
    0
  }
}

///|
pub fn battery_reading_available(reading : Int) -> Bool {
  reading % 2 == 1
}

///|
pub fn battery_reading_estimated(reading : Int) -> Bool {
  reading / 2 % 2 == 1
}

///|
pub fn battery_reading_percent(reading : Int) -> Int {
  if battery_reading_available(reading) {
    reading / 4
  } else {
    -1
  }
}

///|
pub fn battery_reading_low(reading : Int) -> Bool {
  battery_reading_available(reading) &&
  battery_reading_percent(reading) < battery_low_percent
}

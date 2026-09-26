///|
/// BLE keyboard transport states reported by the ESP-IDF adapter.
pub let ble_keyboard_starting : Int = 0

///|
pub let ble_keyboard_advertising : Int = 1

///|
pub let ble_keyboard_pairing : Int = 2

///|
pub let ble_keyboard_connected : Int = 3

///|
pub let ble_keyboard_sending : Int = 4

///|
pub let ble_keyboard_sent : Int = 5

///|
pub let ble_keyboard_error : Int = 6

///|
/// The ESP-IDF adapter retries a stopped advertisement at this cadence. The
/// timing policy lives here so transport recovery stays deterministic and
/// host-testable.
pub let ble_keyboard_advertising_retry_ms : Int = 2000

///|
pub fn ble_keyboard_status_normalize(status : Int) -> Int {
  status.clamp(min=ble_keyboard_starting, max=ble_keyboard_error)
}

///|
pub fn ble_keyboard_send_allowed(state : UInt64, status : Int) -> Bool {
  state_get(state, state_field_result) == result_success &&
  (status == ble_keyboard_connected || status == ble_keyboard_sent)
}

///|
pub fn ble_keyboard_input_allowed(status : Int) -> Bool {
  status != ble_keyboard_sending
}

///|
/// A disconnected keyboard must remain discoverable. `advertising` reflects
/// the NimBLE controller state; the adapter only performs the requested radio
/// operation.
pub fn ble_keyboard_should_advertise(
  status : Int,
  connected : Bool,
  advertising : Bool,
) -> Bool {
  !connected &&
  !advertising &&
  ble_keyboard_status_normalize(status) != ble_keyboard_starting
}

///|
fn shifted_key(usage : Int) -> Int {
  0x0200 | usage
}

///|
/// Returns modifier in bits 8..15 and USB HID usage in bits 0..7.
/// The printable ASCII mapping follows a US keyboard layout.
pub fn ble_keyboard_report(character : Int) -> Int {
  if character >= 97 && character <= 122 {
    return 0x04 + character - 97
  }
  if character >= 65 && character <= 90 {
    return shifted_key(0x04 + character - 65)
  }
  if character >= 49 && character <= 57 {
    return 0x1e + character - 49
  }
  if character == 48 {
    return 0x27
  }
  match character {
    32 => 0x2c
    33 => shifted_key(0x1e)
    34 => shifted_key(0x34)
    35 => shifted_key(0x20)
    36 => shifted_key(0x21)
    37 => shifted_key(0x22)
    38 => shifted_key(0x24)
    39 => 0x34
    40 => shifted_key(0x26)
    41 => shifted_key(0x27)
    42 => shifted_key(0x25)
    43 => shifted_key(0x2e)
    44 => 0x36
    45 => 0x2d
    46 => 0x37
    47 => 0x38
    58 => shifted_key(0x33)
    59 => 0x33
    60 => shifted_key(0x36)
    61 => 0x2e
    62 => shifted_key(0x37)
    63 => shifted_key(0x38)
    64 => shifted_key(0x1f)
    91 => 0x2f
    92 => 0x31
    93 => 0x30
    94 => shifted_key(0x23)
    95 => shifted_key(0x2d)
    96 => 0x35
    123 => shifted_key(0x2f)
    124 => shifted_key(0x31)
    125 => shifted_key(0x30)
    126 => shifted_key(0x35)
    _ => -1
  }
}

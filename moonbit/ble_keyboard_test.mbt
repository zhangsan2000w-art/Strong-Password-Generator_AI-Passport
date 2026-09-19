///|
test "BLE keyboard maps letters digits and symbols" {
  @test.assert_eq(ble_keyboard_report(97), 0x04)
  @test.assert_eq(ble_keyboard_report(122), 0x1d)
  @test.assert_eq(ble_keyboard_report(65), 0x0204)
  @test.assert_eq(ble_keyboard_report(90), 0x021d)
  @test.assert_eq(ble_keyboard_report(49), 0x1e)
  @test.assert_eq(ble_keyboard_report(48), 0x27)
  @test.assert_eq(ble_keyboard_report(33), 0x021e)
  @test.assert_eq(ble_keyboard_report(64), 0x021f)
  @test.assert_eq(ble_keyboard_report(45), 0x2d)
  @test.assert_eq(ble_keyboard_report(95), 0x022d)
  @test.assert_eq(ble_keyboard_report(126), 0x0235)
}

///|
test "BLE keyboard covers every printable ASCII character" {
  let mut character = 0x20
  while character <= 0x7e {
    @test.assert_eq(ble_keyboard_report(character) >= 0, true)
    character = character + 1
  }
  @test.assert_eq(ble_keyboard_report(0x1f), -1)
  @test.assert_eq(ble_keyboard_report(0x7f), -1)
}

///|
test "BLE send requires a generated password and encrypted connection" {
  let empty = initial_state()
  let generated = record_generation_result(empty, generate_ok)
  @test.assert_eq(
    ble_keyboard_send_allowed(empty, ble_keyboard_connected),
    false,
  )
  @test.assert_eq(
    ble_keyboard_send_allowed(generated, ble_keyboard_advertising),
    false,
  )
  @test.assert_eq(
    ble_keyboard_send_allowed(generated, ble_keyboard_pairing),
    false,
  )
  @test.assert_eq(
    ble_keyboard_send_allowed(generated, ble_keyboard_connected),
    true,
  )
  @test.assert_eq(ble_keyboard_send_allowed(generated, ble_keyboard_sent), true)
  @test.assert_eq(
    ble_keyboard_send_allowed(generated, ble_keyboard_sending),
    false,
  )
  @test.assert_eq(ble_keyboard_input_allowed(ble_keyboard_connected), true)
  @test.assert_eq(ble_keyboard_input_allowed(ble_keyboard_sending), false)
}

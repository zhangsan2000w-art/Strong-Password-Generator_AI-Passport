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

///|
test "BLE advertisement recovery only runs while disconnected" {
  @test.assert_eq(
    ble_keyboard_should_advertise(ble_keyboard_advertising, false, false),
    true,
  )
  @test.assert_eq(
    ble_keyboard_should_advertise(ble_keyboard_error, false, false),
    true,
  )
  @test.assert_eq(
    ble_keyboard_should_advertise(ble_keyboard_starting, false, false),
    false,
  )
  @test.assert_eq(
    ble_keyboard_should_advertise(ble_keyboard_connected, true, false),
    false,
  )
  @test.assert_eq(
    ble_keyboard_should_advertise(ble_keyboard_advertising, false, true),
    false,
  )
  @test.assert_eq(
    ble_keyboard_should_advertise(ble_keyboard_released, false, false),
    false,
  )
  @test.assert_eq(ble_keyboard_advertising_retry_ms, 2000)
}

///|
test "BLE link becomes ready after encryption and resets on disconnect" {
  let connected = ble_keyboard_link_event(0, ble_keyboard_link_connected)
  @test.assert_eq(ble_keyboard_link_status(connected), ble_keyboard_pairing)
  @test.assert_eq(ble_keyboard_link_ready(connected), false)
  let encrypted = ble_keyboard_link_event(
    connected, ble_keyboard_link_encrypted,
  )
  @test.assert_eq(ble_keyboard_link_status(encrypted), ble_keyboard_connected)
  @test.assert_eq(ble_keyboard_link_ready(encrypted), true)
  let disconnected = ble_keyboard_link_event(
    encrypted, ble_keyboard_link_disconnected,
  )
  @test.assert_eq(ble_keyboard_link_status(disconnected), ble_keyboard_error)
  @test.assert_eq(ble_keyboard_link_ready(disconnected), false)
}

///|
test "BLE link supports reboot reconnect and fresh pairing order" {
  let first_connection = ble_keyboard_link_event(0, ble_keyboard_link_connected)
  let first_ready = ble_keyboard_link_event(
    first_connection, ble_keyboard_link_encrypted,
  )
  let disconnected = ble_keyboard_link_event(
    first_ready, ble_keyboard_link_disconnected,
  )
  let reconnected = ble_keyboard_link_event(
    disconnected, ble_keyboard_link_connected,
  )
  @test.assert_eq(ble_keyboard_link_ready(reconnected), false)
  let restored = ble_keyboard_link_event(
    reconnected, ble_keyboard_link_encrypted,
  )
  @test.assert_eq(ble_keyboard_link_ready(restored), true)

  let released = ble_keyboard_link_event(restored, ble_keyboard_link_released)
  @test.assert_eq(ble_keyboard_link_status(released), ble_keyboard_released)
  @test.assert_eq(ble_keyboard_link_ready(released), false)
  let late_security_failure = ble_keyboard_link_event(
    released, ble_keyboard_link_encryption_failed,
  )
  @test.assert_eq(
    ble_keyboard_link_status(late_security_failure),
    ble_keyboard_released,
  )
  let released_and_disconnected = ble_keyboard_link_event(
    released, ble_keyboard_link_disconnected,
  )
  @test.assert_eq(
    ble_keyboard_link_status(released_and_disconnected),
    ble_keyboard_released,
  )
  let resumed = ble_keyboard_link_event(
    released_and_disconnected, ble_keyboard_link_resumed,
  )
  @test.assert_eq(ble_keyboard_link_status(resumed), ble_keyboard_error)

  let ignored = ble_keyboard_link_event(0, ble_keyboard_link_encrypted)
  @test.assert_eq(ble_keyboard_link_ready(ignored), false)
  let failed = ble_keyboard_link_event(
    restored, ble_keyboard_link_encryption_failed,
  )
  @test.assert_eq(ble_keyboard_link_status(failed), ble_keyboard_pairing)
  @test.assert_eq(ble_keyboard_link_ready(failed), false)
}

///|
test "BLE action reconnects a released keyboard before sending again" {
  let generated = record_generation_result(initial_state(), generate_ok)
  @test.assert_eq(
    ble_keyboard_action(generated, ble_keyboard_connected),
    ble_keyboard_action_send,
  )
  @test.assert_eq(
    ble_keyboard_action(generated, ble_keyboard_released),
    ble_keyboard_action_reconnect,
  )
  @test.assert_eq(
    ble_keyboard_action(initial_state(), ble_keyboard_connected),
    ble_keyboard_action_disabled,
  )
}

///|
test "settings normalizes persisted values" {
  let settings = settings_initial(99, -1)
  @test.assert_eq(settings_selected(settings), settings_option_theme)
  @test.assert_eq(settings_theme(settings), theme_cyber)
  @test.assert_eq(settings_sound_enabled(settings), 1)
}

///|
test "settings navigation wraps between the two options" {
  let initial = settings_initial(theme_cyber, 1)
  let up = settings_handle_input(initial, input_up)
  @test.assert_eq(settings_selected(up), settings_option_sound)
  let down = settings_handle_input(up, input_down)
  @test.assert_eq(settings_selected(down), settings_option_theme)
}

///|
test "settings OK toggles only the focused value" {
  let initial = settings_initial(theme_cyber, 1)
  let sky = settings_handle_input(initial, input_ok)
  @test.assert_eq(settings_theme(sky), theme_sky)
  @test.assert_eq(settings_sound_enabled(sky), 1)

  let sound_focused = settings_handle_input(sky, input_down)
  let muted = settings_handle_input(sound_focused, input_ok)
  @test.assert_eq(settings_theme(muted), theme_sky)
  @test.assert_eq(settings_sound_enabled(muted), 0)
}

///|
test "settings ignores unsupported input" {
  let initial = settings_initial(theme_sky, 0)
  @test.assert_eq(settings_handle_input(initial, input_ok_long), initial)
  @test.assert_eq(settings_handle_input(initial, 99), initial)
}

///|
test "settings maps only supported button gestures" {
  @test.assert_eq(
    settings_map_button_event(button_up, button_event_click),
    input_up,
  )
  @test.assert_eq(
    settings_map_button_event(button_down, button_event_click),
    input_down,
  )
  @test.assert_eq(
    settings_map_button_event(button_ok, button_event_click),
    input_ok,
  )
  @test.assert_eq(
    settings_map_button_event(button_ok, button_event_long),
    input_ok_long,
  )
  @test.assert_eq(
    settings_map_button_event(button_up, button_event_long_hold),
    input_ignored,
  )
}

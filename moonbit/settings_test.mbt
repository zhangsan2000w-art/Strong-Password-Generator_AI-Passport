///|
test "settings normalizes persisted values" {
  let settings = settings_initial(99, -1, 99, -1)
  @test.assert_eq(settings_selected(settings), settings_option_theme)
  @test.assert_eq(settings_theme(settings), theme_cyber)
  @test.assert_eq(settings_sound_enabled(settings), 1)
  @test.assert_eq(settings_policy_profile(settings), policy_profile_standard)
  @test.assert_eq(settings_exclude_ambiguous(settings), 1)
}

///|
test "settings navigation wraps across all options" {
  let initial = settings_initial(theme_cyber, 1, policy_profile_standard, 1)
  let up = settings_handle_input(initial, input_up)
  @test.assert_eq(settings_selected(up), settings_option_exclude_ambiguous)
  let down = settings_handle_input(up, input_down)
  @test.assert_eq(settings_selected(down), settings_option_theme)
  let sound = settings_handle_input(down, input_down)
  @test.assert_eq(settings_selected(sound), settings_option_sound)
  let policy = settings_handle_input(sound, input_down)
  @test.assert_eq(settings_selected(policy), settings_option_policy_profile)
}

///|
test "settings OK toggles only the focused value" {
  let initial = settings_initial(theme_cyber, 1, policy_profile_compatible, 1)
  let sky = settings_handle_input(initial, input_ok)
  @test.assert_eq(settings_theme(sky), theme_sky)
  @test.assert_eq(settings_sound_enabled(sky), 1)

  let sound_focused = settings_handle_input(sky, input_down)
  let muted = settings_handle_input(sound_focused, input_ok)
  @test.assert_eq(settings_theme(muted), theme_sky)
  @test.assert_eq(settings_sound_enabled(muted), 0)

  let policy_focused = settings_handle_input(muted, input_down)
  let standard = settings_handle_input(policy_focused, input_ok)
  @test.assert_eq(settings_policy_profile(standard), policy_profile_standard)
  @test.assert_eq(settings_exclude_ambiguous(standard), 1)

  let exclude_focused = settings_handle_input(standard, input_down)
  let ambiguous_allowed = settings_handle_input(exclude_focused, input_ok)
  @test.assert_eq(settings_exclude_ambiguous(ambiguous_allowed), 0)
}

///|
test "settings ignores unsupported input" {
  let initial = settings_initial(theme_sky, 0, policy_profile_strict, 0)
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

///|
test "app policy settings clear stale generation results" {
  let generated = record_generation_result(initial_state(), generate_ok)
  let applied = with_policy_settings(generated, policy_profile_compatible, 0)
  @test.assert_eq(
    state_get(applied, state_field_policy_profile),
    policy_profile_compatible,
  )
  @test.assert_eq(state_get(applied, state_field_exclude_ambiguous), 0)
  @test.assert_eq(state_get(applied, state_field_result), result_empty)
}

///|
test "unchanged profile preserves session character overrides" {
  let custom = with_random_policy_options(
    initial_state(),
    false,
    true,
    true,
    true,
    true,
    true,
  )
  let applied = with_policy_settings(custom, policy_profile_standard, 0)
  @test.assert_eq(state_get(applied, state_field_lowercase), 0)
  @test.assert_eq(state_get(applied, state_field_uppercase), 1)
  @test.assert_eq(state_get(applied, state_field_exclude_ambiguous), 0)
}

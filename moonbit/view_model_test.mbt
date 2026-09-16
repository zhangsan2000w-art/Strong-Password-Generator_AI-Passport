///|
fn state_for_mode(mode : Int) -> UInt64 {
  let mut state = initial_state()
  let mut current = 0
  while current < mode {
    state = handle_input(state, input_down)
    current = current + 1
  }
  state
}

///|
test "Random view exposes four product parameter slots" {
  let state = state_for_mode(0)
  @test.assert_eq(view_parameter_kind(0, 0), parameter_random_length)
  @test.assert_eq(view_parameter_kind(0, 1), parameter_letters)
  @test.assert_eq(view_parameter_kind(0, 2), parameter_digits)
  @test.assert_eq(view_parameter_kind(0, 3), parameter_symbols)
  @test.assert_eq(view_parameter_value(state, 0), 10)
  @test.assert_eq(view_parameter_value(state, 1), 1)
  @test.assert_eq(view_parameter_value(state, 2), 1)
  @test.assert_eq(view_parameter_value(state, 3), 0)
}

///|
test "Memorable view maps state values to semantic slots" {
  let state = state_for_mode(1)
  @test.assert_eq(view_parameter_kind(1, 0), parameter_word_count)
  @test.assert_eq(view_parameter_kind(1, 1), parameter_capitalize)
  @test.assert_eq(view_parameter_kind(1, 2), parameter_complete_word)
  @test.assert_eq(view_parameter_kind(1, 3), parameter_separator)
  @test.assert_eq(view_parameter_value(state, 0), 4)
  @test.assert_eq(view_parameter_value(state, 1), 0)
  @test.assert_eq(view_parameter_value(state, 2), 1)
  @test.assert_eq(view_parameter_value(state, 3), 0)
}

///|
test "PIN view hides unused slots" {
  let state = state_for_mode(2)
  @test.assert_eq(view_parameter_kind(2, 0), parameter_pin_length)
  @test.assert_eq(view_parameter_value(state, 0), 6)
  @test.assert_eq(view_parameter_visible(2, 0), true)
  @test.assert_eq(view_parameter_visible(2, 1), false)
  @test.assert_eq(view_parameter_kind(2, 9), parameter_hidden)
}

///|
test "view layout matches the 240x320 parameter grid" {
  @test.assert_eq(view_parameter_x(0, 0), 2)
  @test.assert_eq(view_parameter_x(0, 1), 106)
  @test.assert_eq(view_parameter_y(0, 0), 2)
  @test.assert_eq(view_parameter_y(0, 2), 34)
  @test.assert_eq(view_parameter_width(1, 3), 100)
  @test.assert_eq(view_parameter_height(1, 3), 27)
  @test.assert_eq(view_parameter_x(2, 0), 54)
  @test.assert_eq(view_parameter_y(2, 0), 18)
  @test.assert_eq(view_parameter_width(2, 1), 0)
}

///|
test "view selection follows navigation and editing state" {
  let initial = initial_state()
  @test.assert_eq(view_mode_selected(initial, 0), true)
  @test.assert_eq(view_mode_focused(initial, 0), true)
  let parameter_focus = handle_input(initial, input_ok)
  @test.assert_eq(view_parameter_selected(parameter_focus, 0), true)
  @test.assert_eq(view_parameter_editing(parameter_focus, 0), false)
  let editing = handle_input(parameter_focus, input_ok)
  @test.assert_eq(view_parameter_selected(editing, 0), true)
  @test.assert_eq(view_parameter_editing(editing, 0), true)
}

///|
test "generate focus is mode-specific" {
  let mut random = initial_state()
  random = handle_input(random, input_ok)
  let mut step = 0
  while step < 3 {
    random = handle_input(random, input_down)
    step = step + 1
  }
  @test.assert_eq(view_generate_focused(random), true)
  let mut pin = state_for_mode(2)
  pin = handle_input(pin, input_ok)
  pin = handle_input(pin, input_down)
  @test.assert_eq(view_generate_focused(pin), true)
}

///|
test "configuration fingerprint ignores focus-only movement" {
  let initial = initial_state()
  let focus_only = handle_input(initial, input_ok)
  @test.assert_eq(configuration_changed(initial, focus_only), false)
  let editing = handle_input(focus_only, input_ok)
  let changed = handle_input(editing, input_up)
  @test.assert_eq(configuration_changed(editing, changed), true)
}

///|
test "strength derives from each mode entropy" {
  let random = initial_state()
  let memorable = state_for_mode(1)
  let pin = state_for_mode(2)
  @test.assert_eq(state_strength(random), strength_strong)
  @test.assert_eq(state_strength(memorable), strength_fair)
  @test.assert_eq(state_strength(pin), strength_weak)
  @test.assert_eq(state_configuration_valid(random), true)
  @test.assert_eq(state_configuration_valid(memorable), true)
  @test.assert_eq(state_configuration_valid(pin), true)
}

///|
test "MoonBit owns generation result lifecycle" {
  let initial = initial_state()
  let success = record_generation_result(initial, generate_ok)
  @test.assert_eq(state_get(success, state_field_result), result_success)
  @test.assert_eq(state_get(success, state_field_generation_count), 1)
  let focus_only = apply_input(success, input_ok)
  @test.assert_eq(state_get(focus_only, state_field_result), result_success)
  let editing = apply_input(focus_only, input_ok)
  let changed = apply_input(editing, input_up)
  @test.assert_eq(state_get(changed, state_field_result), result_empty)
  let failed = record_generation_result(success, generate_output_failed)
  @test.assert_eq(state_get(failed, state_field_result), result_failure)
  @test.assert_eq(state_get(failed, state_field_generation_count), 1)
}

///|
test "MoonBit maps click and held gestures by editing state" {
  let initial = initial_state()
  @test.assert_eq(
    map_button_event(initial, button_up, button_event_click),
    input_up,
  )
  @test.assert_eq(
    map_button_event(initial, button_up, button_event_long_hold),
    input_ignored,
  )
  @test.assert_eq(
    map_button_event(initial, button_ok, button_event_long),
    input_ok_long,
  )
  let focused = apply_input(initial, input_ok)
  let editing = apply_input(focused, input_ok)
  @test.assert_eq(
    map_button_event(editing, button_down, button_event_long_hold),
    input_down,
  )
  @test.assert_eq(
    map_button_event(editing, button_up, button_event_long),
    input_up,
  )
  @test.assert_eq(
    map_button_event(editing, 99, button_event_click),
    input_ignored,
  )
}

///|
test "theme defaults to cyber" {
  @test.assert_eq(view_theme(initial_state()), theme_cyber)
}

///|
test "with_theme sets the theme field" {
  let initial = initial_state()
  @test.assert_eq(view_theme(with_theme(initial, theme_sky)), theme_sky)
  @test.assert_eq(view_theme(with_theme(initial, theme_cyber)), theme_cyber)
  @test.assert_eq(view_theme(with_theme(initial, 99)), theme_cyber)
  @test.assert_eq(
    configuration_changed(initial, with_theme(initial, theme_sky)),
    false,
  )
}

///|
test "long OK when not editing no longer toggles theme" {
  let initial = initial_state()
  @test.assert_eq(view_theme(handle_input(initial, input_ok_long)), theme_cyber)
}

///|
test "long OK while editing cancels without changing theme" {
  let initial = initial_state()
  let focused = handle_input(initial, input_ok)
  let editing = handle_input(focused, input_ok)
  @test.assert_eq(state_get(editing, state_field_editing), 1)
  let cancelled = handle_input(editing, input_ok_long)
  @test.assert_eq(state_get(cancelled, state_field_editing), 0)
  @test.assert_eq(view_theme(cancelled), theme_cyber)
}

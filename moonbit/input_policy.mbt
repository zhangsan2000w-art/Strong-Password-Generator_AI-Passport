///|
/// Abstract buttons and events. The BSP adapter converts hardware enums to
/// these stable values; MoonBit owns which gestures affect product state.
pub let button_up : Int = 0

///|
pub let button_down : Int = 1

///|
pub let button_ok : Int = 2

///|
pub let button_event_click : Int = 0

///|
pub let button_event_long : Int = 1

///|
pub let button_event_long_hold : Int = 2

///|
pub let input_ignored : Int = -1

///|
pub fn map_button_event(state : UInt64, button : Int, event : Int) -> Int {
  let editing = state_get(state, state_field_editing) == 1
  if button == button_ok && event == button_event_long {
    return input_ok_long
  }
  if !editing && button == button_down && event == button_event_long {
    return input_open_settings
  }
  let held = event == button_event_long || event == button_event_long_hold
  if editing && held && button == button_up {
    return input_up
  }
  if editing && held && button == button_down {
    return input_down
  }
  if event != button_event_click {
    return input_ignored
  }
  match button {
    0 => input_up
    1 => input_down
    2 => input_ok
    _ => input_ignored
  }
}

///|
/// Passwords remain readable only for this short, explicit display window.
pub let password_display_timeout_ms : UInt64 = 60_000UL

///|
pub let password_display_empty : Int = 0

///|
pub let password_display_visible : Int = 1

///|
pub let password_display_masked : Int = 2

///|
/// A fixed-width mask avoids exposing the generated password's exact length.
pub let password_display_mask_width : Int = 12

///|
/// Printable ASCII keeps the mask renderable by the device's built-in font.
pub let password_display_mask_character : Int = 42

///|
pub fn password_display_begin(result : Int) -> Int {
  if result == result_success {
    password_display_visible
  } else {
    password_display_empty
  }
}

///|
pub fn password_display_is_visible(mode : Int) -> Bool {
  mode == password_display_visible
}

///|
/// Masking is terminal for the current password. A timer wrap or UI rebuild
/// must never make an already hidden password visible again.
pub fn password_display_update(
  current : Int,
  result : Int,
  elapsed_ms : UInt64,
) -> Int {
  if result != result_success {
    return password_display_empty
  }
  if current == password_display_masked ||
    elapsed_ms >= password_display_timeout_ms {
    password_display_masked
  } else {
    password_display_visible
  }
}

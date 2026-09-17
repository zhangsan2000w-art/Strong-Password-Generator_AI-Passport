///|
/// Website-oriented presets for random-password generation. The profile owns
/// security defaults; platform adapters only persist and render its integer id.
pub let policy_profile_compatible : Int = 0

///|
pub let policy_profile_standard : Int = 1

///|
pub let policy_profile_strict : Int = 2

///|
pub let policy_profile_count : Int = 3

///|
pub fn normalize_policy_profile(profile : Int) -> Int {
  if profile >= policy_profile_compatible && profile <= policy_profile_strict {
    profile
  } else {
    policy_profile_standard
  }
}

///|
pub fn policy_profile_default_length(profile : Int) -> Int {
  match normalize_policy_profile(profile) {
    0 => 12
    1 => 16
    _ => 20
  }
}

///|
pub fn policy_profile_min_length(profile : Int) -> Int {
  match normalize_policy_profile(profile) {
    0 => 8
    1 => 12
    _ => 16
  }
}

///|
pub fn policy_profile_lowercase(_profile : Int) -> Bool {
  true
}

///|
pub fn policy_profile_uppercase(_profile : Int) -> Bool {
  true
}

///|
pub fn policy_profile_digits(_profile : Int) -> Bool {
  true
}

///|
pub fn policy_profile_symbols(profile : Int) -> Bool {
  normalize_policy_profile(profile) != policy_profile_compatible
}

///|
pub fn policy_profile_exclude_ambiguous(profile : Int) -> Bool {
  normalize_policy_profile(profile) != policy_profile_strict
}

///|
pub fn policy_profile_safe_symbols_only(profile : Int) -> Bool {
  normalize_policy_profile(profile) != policy_profile_strict
}

///|
pub fn normalize_policy_length(profile : Int, length : Int) -> Int {
  length.clamp(min=policy_profile_min_length(profile), max=random_max_length)
}

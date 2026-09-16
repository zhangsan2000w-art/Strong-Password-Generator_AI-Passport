#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void moonbit_init(void);
uint64_t passport_moonbit_initial_state(void);
uint64_t passport_moonbit_handle_input(uint64_t state, int32_t input);
int32_t passport_moonbit_state_get(uint64_t state, int32_t requested);
int32_t passport_moonbit_state_action(uint64_t state);
int32_t passport_moonbit_generate(uint64_t state);
int32_t passport_moonbit_entropy_x10(uint64_t state);
int32_t passport_moonbit_strength(uint64_t state);
int32_t passport_moonbit_configuration_valid(uint64_t state);
int32_t passport_moonbit_configuration_changed(uint64_t before, uint64_t after);
uint64_t passport_moonbit_record_generation(uint64_t state, int32_t result);
int32_t passport_moonbit_map_button_event(uint64_t state, int32_t button, int32_t event);
int32_t passport_moonbit_mode_max_focus(int32_t mode);
int32_t passport_moonbit_view_parameter_kind(uint64_t state, int32_t slot);
int32_t passport_moonbit_view_parameter_value(uint64_t state, int32_t slot);
int32_t passport_moonbit_view_parameter_x(uint64_t state, int32_t slot);
int32_t passport_moonbit_view_parameter_y(uint64_t state, int32_t slot);
int32_t passport_moonbit_view_parameter_width(uint64_t state, int32_t slot);
int32_t passport_moonbit_view_parameter_height(uint64_t state, int32_t slot);
int32_t passport_moonbit_view_parameter_selected(uint64_t state, int32_t slot);
int32_t passport_moonbit_view_parameter_editing(uint64_t state, int32_t slot);
int32_t passport_moonbit_view_mode_selected(uint64_t state, int32_t mode);
int32_t passport_moonbit_view_mode_focused(uint64_t state, int32_t mode);
int32_t passport_moonbit_view_generate_focused(uint64_t state);
int32_t passport_moonbit_view_theme(uint64_t state);
uint64_t passport_moonbit_with_theme(uint64_t state, int32_t theme);
uint64_t passport_moonbit_settings_initial(int32_t theme, int32_t sound_enabled);
uint64_t passport_moonbit_settings_handle_input(uint64_t state, int32_t input);
int32_t passport_moonbit_settings_map_button_event(int32_t button, int32_t event);
int32_t passport_moonbit_settings_selected(uint64_t state);
int32_t passport_moonbit_settings_theme(uint64_t state);
int32_t passport_moonbit_settings_sound_enabled(uint64_t state);
int32_t passport_moonbit_battery_resolve(int32_t percent, int32_t millivolts);
int32_t passport_moonbit_battery_percent(int32_t reading);
int32_t passport_moonbit_battery_available(int32_t reading);
int32_t passport_moonbit_battery_estimated(int32_t reading);
int32_t passport_moonbit_battery_low(int32_t reading);
int32_t passport_moonbit_battery_refresh_interval_ms(void);
int32_t passport_moonbit_sound_sample_rate(void);
int32_t passport_moonbit_sound_volume(void);
int32_t passport_moonbit_sound_note_count(void);
int32_t passport_moonbit_sound_note_samples(int32_t note);
int32_t passport_moonbit_sound_sample(int32_t note, int32_t sample_index);

#ifdef __cplusplus
}
#endif

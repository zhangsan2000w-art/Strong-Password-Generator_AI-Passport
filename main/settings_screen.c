#include "settings_screen.h"

#include <stdbool.h>
#include <stddef.h>

#include "bsp_display.h"
#include "lvgl.h"
#include "moonbit_password.h"
#include "settings_store.h"
#include "ui_pixel.h"

enum {
    OPTION_COUNT = 4,
    INPUT_UP = 0,
    INPUT_DOWN = 1,
    INPUT_OK = 2,
    INPUT_OK_LONG = 3,
    BUTTON_UP = 0,
    BUTTON_DOWN = 1,
    BUTTON_OK = 2,
    BUTTON_EVENT_CLICK = 0,
    BUTTON_EVENT_LONG = 1,
    BUTTON_EVENT_LONG_HOLD = 2,
};

static lv_obj_t *s_screen;
static lv_obj_t *s_panels[OPTION_COUNT];
static lv_obj_t *s_name_labels[OPTION_COUNT];
static lv_obj_t *s_value_labels[OPTION_COUNT];
static uint64_t s_state;
static settings_screen_on_exit_t s_on_exit;

static const char *theme_text(int theme)
{
    return theme == SETTINGS_THEME_SKY ? "Sky" : "Cyber";
}

static const char *sound_text(bool enabled)
{
    return enabled ? "ON" : "OFF";
}

static const char *policy_text(int profile)
{
    static const char *names[] = {"Compat", "Standard", "Strict"};
    return profile >= SETTINGS_POLICY_COMPATIBLE &&
        profile <= SETTINGS_POLICY_STRICT ? names[profile] : names[1];
}

static void teardown(void)
{
    if (s_screen) {
        lv_obj_delete(s_screen);
        s_screen = NULL;
    }
    for (int i = 0; i < OPTION_COUNT; i++) {
        s_panels[i] = NULL;
        s_name_labels[i] = NULL;
        s_value_labels[i] = NULL;
    }
}

static void refresh_options(void)
{
    int selected = passport_moonbit_settings_selected(s_state);
    lv_label_set_text(
        s_value_labels[0],
        theme_text(passport_moonbit_settings_theme(s_state))
    );
    lv_label_set_text(
        s_value_labels[1],
        sound_text(passport_moonbit_settings_sound_enabled(s_state) != 0)
    );
    lv_label_set_text(
        s_value_labels[2],
        policy_text(passport_moonbit_settings_policy_profile(s_state))
    );
    lv_label_set_text(
        s_value_labels[3],
        sound_text(passport_moonbit_settings_exclude_ambiguous(s_state) != 0)
    );
    for (int i = 0; i < OPTION_COUNT; i++) {
        bool focused = i == selected;
        lv_obj_set_style_bg_color(
            s_panels[i], lv_color_hex(focused ? UI_YELLOW : UI_PAPER), 0
        );
        lv_obj_set_style_border_color(
            s_panels[i], lv_color_hex(focused ? UI_GRASS_DARK : UI_SKY_DARK), 0
        );
        lv_obj_set_style_text_color(
            s_name_labels[i], lv_color_hex(focused ? UI_INK : UI_TEXT), 0
        );
        lv_obj_set_style_text_color(
            s_value_labels[i], lv_color_hex(focused ? UI_INK : UI_TEXT), 0
        );
    }
}

static void build(void)
{
    static const char *names[OPTION_COUNT] = {
        "Theme", "Sound", "Policy", "Clear chars"
    };
    lv_obj_t *old_screen = s_screen;

    s_screen = NULL;
    ui_pixel_set_theme(
        (ui_pixel_theme_t)passport_moonbit_settings_theme(s_state)
    );
    s_screen = ui_pixel_screen_create_with_font("Settings", &lv_font_montserrat_20);

    for (int i = 0; i < OPTION_COUNT; i++) {
        s_panels[i] = ui_pixel_panel_create(s_screen, 7, 50 + i * 51, 226, 43, UI_PAPER);
        s_name_labels[i] = ui_pixel_label(
            s_panels[i], names[i], &lv_font_montserrat_14, UI_TEXT
        );
        lv_obj_align(s_name_labels[i], LV_ALIGN_LEFT_MID, 0, 0);
        s_value_labels[i] = ui_pixel_label(
            s_panels[i], "", &lv_font_montserrat_14, UI_TEXT
        );
        lv_obj_align(s_value_labels[i], LV_ALIGN_RIGHT_MID, 0, 0);
    }

    lv_obj_t *hint = ui_pixel_label(
        s_screen, "OK: change   LONG: back", &lv_font_montserrat_14, UI_TEXT
    );
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -12);

    refresh_options();
    lv_screen_load(s_screen);
    if (old_screen) lv_obj_delete(old_screen);
}

bool settings_screen_enter(settings_screen_on_exit_t on_exit)
{
    s_on_exit = on_exit;
    s_state = passport_moonbit_settings_initial(
        settings_store_theme(),
        settings_store_sound_enabled() ? 1 : 0,
        settings_store_policy_profile(),
        settings_store_exclude_ambiguous() ? 1 : 0
    );
    if (!bsp_lvgl_lock(500)) {
        s_on_exit = NULL;
        return false;
    }
    build();
    bsp_lvgl_unlock();
    return true;
}

static void apply_input(int input)
{
    uint64_t previous = s_state;
    s_state = passport_moonbit_settings_handle_input(s_state, input);
    int previous_theme = passport_moonbit_settings_theme(previous);
    int next_theme = passport_moonbit_settings_theme(s_state);
    bool previous_sound =
        passport_moonbit_settings_sound_enabled(previous) != 0;
    bool next_sound = passport_moonbit_settings_sound_enabled(s_state) != 0;
    int previous_policy = passport_moonbit_settings_policy_profile(previous);
    int next_policy = passport_moonbit_settings_policy_profile(s_state);
    bool previous_exclude =
        passport_moonbit_settings_exclude_ambiguous(previous) != 0;
    bool next_exclude =
        passport_moonbit_settings_exclude_ambiguous(s_state) != 0;

    if (previous_theme != next_theme) {
        (void)settings_store_set_theme(next_theme);
        if (bsp_lvgl_lock(500)) {
            build();
            bsp_lvgl_unlock();
        }
        return;
    }
    if (previous_sound != next_sound) {
        (void)settings_store_set_sound(next_sound);
    }
    if (previous_policy != next_policy) {
        (void)settings_store_set_policy_profile(next_policy);
    }
    if (previous_exclude != next_exclude) {
        (void)settings_store_set_exclude_ambiguous(next_exclude);
    }
    if (bsp_lvgl_lock(500)) {
        refresh_options();
        bsp_lvgl_unlock();
    }
}

static void exit_screen(void)
{
    settings_screen_on_exit_t callback = s_on_exit;
    s_on_exit = NULL;
    if (callback && !callback()) {
        s_on_exit = callback;
        return;
    }
    if (bsp_lvgl_lock(500)) {
        teardown();
        bsp_lvgl_unlock();
    }
}

void settings_screen_handle_button(bsp_btn_t button, bsp_btn_ev_t event)
{
    if (!s_screen) return;

    int button_code = button == BSP_BTN_UP ? BUTTON_UP
        : (button == BSP_BTN_DOWN ? BUTTON_DOWN
        : (button == BSP_BTN_OK ? BUTTON_OK : -1));
    int event_code = event == BSP_BTN_CLICK ? BUTTON_EVENT_CLICK
        : (event == BSP_BTN_LONG ? BUTTON_EVENT_LONG
        : (event == BSP_BTN_LONG_HOLD ? BUTTON_EVENT_LONG_HOLD : -1));
    int input = passport_moonbit_settings_map_button_event(
        button_code, event_code
    );
    if (input == INPUT_OK_LONG) {
        exit_screen();
    } else if (input >= INPUT_UP && input <= INPUT_OK) {
        apply_input(input);
    }
}

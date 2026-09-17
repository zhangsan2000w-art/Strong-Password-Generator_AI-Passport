#include "settings_screen.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bsp_display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "moonbit_password.h"
#include "password_platform.h"
#include "settings_store.h"
#include "ui_pixel.h"

enum {
    OPTION_COUNT = 5,
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
    SETTINGS_ACTION_START_SELF_CHECK = 1,
    DIAGNOSTIC_STATUS_RUNNING = 0,
    DIAGNOSTIC_STATUS_PASSED = 1,
    DIAGNOSTIC_STATUS_FAILED = 2,
    DIAGNOSTIC_STATUS_CANCELLED = 3,
    DIAGNOSTIC_SAMPLE_TARGET = 32,
    DIAGNOSTIC_TASK_STACK = 4096,
};

static lv_obj_t *s_screen;
static lv_obj_t *s_panels[OPTION_COUNT];
static lv_obj_t *s_name_labels[OPTION_COUNT];
static lv_obj_t *s_value_labels[OPTION_COUNT];
static lv_obj_t *s_diagnostic_status_label;
static lv_obj_t *s_diagnostic_progress_label;
static lv_obj_t *s_diagnostic_detail_label;
static uint64_t s_state;
static uint64_t s_policy_state;
static uint64_t s_diagnostic_run;
static settings_screen_on_exit_t s_on_exit;
static TaskHandle_t s_diagnostic_task;
static volatile bool s_diagnostic_cancel;
static volatile bool s_diagnostic_active;
static bool s_diagnostic_view;

static void build(void);

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
    s_diagnostic_status_label = NULL;
    s_diagnostic_progress_label = NULL;
    s_diagnostic_detail_label = NULL;
    s_diagnostic_view = false;
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
    lv_label_set_text(s_value_labels[4], "RUN");
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
        "Theme", "Sound", "Policy", "Clear chars", "Self-check"
    };
    lv_obj_t *old_screen = s_screen;

    s_screen = NULL;
    s_diagnostic_view = false;
    s_diagnostic_status_label = NULL;
    s_diagnostic_progress_label = NULL;
    s_diagnostic_detail_label = NULL;
    ui_pixel_set_theme(
        (ui_pixel_theme_t)passport_moonbit_settings_theme(s_state)
    );
    s_screen = ui_pixel_screen_create_with_font("Settings", &lv_font_montserrat_20);

    for (int i = 0; i < OPTION_COUNT; i++) {
        s_panels[i] = ui_pixel_panel_create(
            s_screen, 7, 45 + i * 42, 226, 35, UI_PAPER
        );
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
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -10);

    refresh_options();
    lv_screen_load(s_screen);
    if (old_screen) lv_obj_delete(old_screen);
}

bool settings_screen_enter(
    uint64_t policy_state,
    settings_screen_on_exit_t on_exit
)
{
    s_on_exit = on_exit;
    s_policy_state = policy_state;
    s_diagnostic_run = passport_moonbit_diagnostic_run_initial(
        DIAGNOSTIC_SAMPLE_TARGET
    );
    s_diagnostic_cancel = false;
    s_diagnostic_active = false;
    s_diagnostic_task = NULL;
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

static void refresh_diagnostics_locked(void)
{
    if (!s_diagnostic_view || !s_diagnostic_status_label) return;

    int status = passport_moonbit_diagnostic_run_status(s_diagnostic_run);
    int completed = passport_moonbit_diagnostic_run_completed(s_diagnostic_run);
    int target = passport_moonbit_diagnostic_run_target(s_diagnostic_run);
    int passed = passport_moonbit_diagnostic_run_passed(s_diagnostic_run);
    int failure = passport_moonbit_diagnostic_run_failure(s_diagnostic_run);

    lv_label_set_text_fmt(
        s_diagnostic_progress_label,
        "%d / %d samples",
        completed,
        target
    );
    if (status == DIAGNOSTIC_STATUS_RUNNING) {
        lv_label_set_text(s_diagnostic_status_label, "Checking...");
        lv_label_set_text(s_diagnostic_detail_label, "No samples are stored");
        lv_obj_set_style_text_color(
            s_diagnostic_status_label, lv_color_hex(UI_SKY_DARK), 0
        );
    } else if (status == DIAGNOSTIC_STATUS_PASSED) {
        lv_label_set_text(s_diagnostic_status_label, "PASS");
        lv_label_set_text_fmt(
            s_diagnostic_detail_label, "%d / %d passed", passed, target
        );
        lv_obj_set_style_text_color(
            s_diagnostic_status_label, lv_color_hex(UI_LIME), 0
        );
    } else if (status == DIAGNOSTIC_STATUS_CANCELLED) {
        lv_label_set_text(s_diagnostic_status_label, "Canceled");
        lv_label_set_text_fmt(
            s_diagnostic_detail_label, "Stopped after %d samples", completed
        );
        lv_obj_set_style_text_color(
            s_diagnostic_status_label, lv_color_hex(UI_ORANGE), 0
        );
    } else {
        lv_label_set_text(s_diagnostic_status_label, "FAIL");
        lv_label_set_text_fmt(
            s_diagnostic_detail_label,
            "%d passed  Error %d",
            passed,
            failure
        );
        lv_obj_set_style_text_color(
            s_diagnostic_status_label, lv_color_hex(UI_RED), 0
        );
    }
}

static void build_diagnostics_locked(void)
{
    lv_obj_t *old_screen = s_screen;

    s_screen = NULL;
    s_diagnostic_view = true;
    for (int i = 0; i < OPTION_COUNT; i++) {
        s_panels[i] = NULL;
        s_name_labels[i] = NULL;
        s_value_labels[i] = NULL;
    }
    ui_pixel_set_theme(
        (ui_pixel_theme_t)passport_moonbit_settings_theme(s_state)
    );
    s_screen = ui_pixel_screen_create_with_font(
        "Security Check", &lv_font_montserrat_20
    );
    lv_obj_t *panel = ui_pixel_panel_create(
        s_screen, 7, 70, 226, 145, UI_PAPER
    );
    s_diagnostic_status_label = ui_pixel_label(
        panel, "Checking...", &lv_font_montserrat_20, UI_SKY_DARK
    );
    lv_obj_align(s_diagnostic_status_label, LV_ALIGN_TOP_MID, 0, 8);
    s_diagnostic_progress_label = ui_pixel_label(
        panel, "", &lv_font_montserrat_14, UI_TEXT
    );
    lv_obj_align(s_diagnostic_progress_label, LV_ALIGN_TOP_MID, 0, 48);
    s_diagnostic_detail_label = ui_pixel_label(
        panel, "", &lv_font_montserrat_14, UI_TEXT
    );
    lv_obj_set_width(s_diagnostic_detail_label, 200);
    lv_obj_set_style_text_align(
        s_diagnostic_detail_label, LV_TEXT_ALIGN_CENTER, 0
    );
    lv_obj_align(s_diagnostic_detail_label, LV_ALIGN_TOP_MID, 0, 78);
    lv_obj_t *hint = ui_pixel_label(
        s_screen, "LONG: cancel / back", &lv_font_montserrat_14, UI_TEXT
    );
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -20);

    refresh_diagnostics_locked();
    lv_screen_load(s_screen);
    if (old_screen) lv_obj_delete(old_screen);
}

static void refresh_diagnostics_from_worker(void)
{
    if (!bsp_lvgl_lock(500)) return;
    refresh_diagnostics_locked();
    bsp_lvgl_unlock();
}

static void diagnostic_task(void *argument)
{
    (void)argument;
    uint64_t policy = passport_moonbit_diagnostic_policy_state(s_policy_state);
    uint64_t run = s_diagnostic_run;

    password_platform_begin_diagnostics();
    while (passport_moonbit_diagnostic_run_status(run) ==
        DIAGNOSTIC_STATUS_RUNNING) {
        if (s_diagnostic_cancel) {
            run = passport_moonbit_diagnostic_cancel_run(run);
            break;
        }

        int result = passport_moonbit_generate(policy);
        uint64_t observation =
            passport_moonbit_diagnostic_observation_initial();
        const char *sample = password_platform_diagnostic_output();
        for (size_t index = 0; sample[index] != '\0'; index++) {
            observation = passport_moonbit_diagnostic_observe_character(
                policy,
                observation,
                (unsigned char)sample[index]
            );
        }
        run = passport_moonbit_diagnostic_record_sample(
            run, policy, observation, result
        );
        s_diagnostic_run = run;
        if (passport_moonbit_diagnostic_run_completed(run) % 4 == 0) {
            refresh_diagnostics_from_worker();
            vTaskDelay(1);
        }
    }
    if (s_diagnostic_cancel &&
        passport_moonbit_diagnostic_run_status(run) ==
        DIAGNOSTIC_STATUS_RUNNING) {
        run = passport_moonbit_diagnostic_cancel_run(run);
    }
    password_platform_end_diagnostics();

    if (bsp_lvgl_lock(500)) {
        s_diagnostic_run = run;
        s_diagnostic_active = false;
        refresh_diagnostics_locked();
        bsp_lvgl_unlock();
    } else {
        s_diagnostic_run = run;
        s_diagnostic_active = false;
    }
    s_diagnostic_task = NULL;
    vTaskDelete(NULL);
}

static void start_diagnostics(void)
{
    if (s_diagnostic_active) return;

    s_diagnostic_cancel = false;
    s_diagnostic_run = passport_moonbit_diagnostic_run_initial(
        DIAGNOSTIC_SAMPLE_TARGET
    );
    s_diagnostic_active = true;
    if (!bsp_lvgl_lock(500)) {
        s_diagnostic_active = false;
        return;
    }
    build_diagnostics_locked();
    bsp_lvgl_unlock();

    if (xTaskCreate(
            diagnostic_task,
            "password_check",
            DIAGNOSTIC_TASK_STACK,
            NULL,
            4,
            &s_diagnostic_task
        ) != pdPASS) {
        uint64_t policy = passport_moonbit_diagnostic_policy_state(
            s_policy_state
        );
        uint64_t failed = passport_moonbit_diagnostic_run_initial(1);
        failed = passport_moonbit_diagnostic_record_sample(
            failed,
            policy,
            passport_moonbit_diagnostic_observation_initial(),
            -1
        );
        s_diagnostic_run = failed;
        s_diagnostic_active = false;
        refresh_diagnostics_from_worker();
    }
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

    if (passport_moonbit_settings_action(s_state) ==
        SETTINGS_ACTION_START_SELF_CHECK) {
        start_diagnostics();
        return;
    }

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
    if (previous_policy != next_policy || previous_exclude != next_exclude) {
        s_policy_state = passport_moonbit_with_policy_settings(
            s_policy_state,
            next_policy,
            next_exclude ? 1 : 0
        );
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
    if (s_diagnostic_view) {
        if (input == INPUT_OK_LONG) {
            if (s_diagnostic_active) {
                s_diagnostic_cancel = true;
            } else if (bsp_lvgl_lock(500)) {
                build();
                bsp_lvgl_unlock();
            }
        }
        return;
    }
    if (input == INPUT_OK_LONG) {
        exit_screen();
    } else if (input >= INPUT_UP && input <= INPUT_OK) {
        apply_input(input);
    }
}

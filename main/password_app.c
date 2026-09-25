#include "password_app.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bsp_battery.h"
#include "bsp_display.h"
#include "lvgl.h"
#include "moonbit_password.h"
#include "password_ble_keyboard.h"
#include "password_platform.h"
#include "password_sound.h"
#include "settings_screen.h"
#include "settings_store.h"
#include "ui_pixel.h"

LV_FONT_DECLARE(passport_font_zh_16);

enum {
    FIELD_MODE = 0,
    FIELD_RESULT = 11,
    FIELD_POLICY_PROFILE = 14,
};

enum {
    ACTION_GENERATE = 1,
    ACTION_SEND = 2,
    ACTION_OPEN_SETTINGS = 3,
    RESULT_SUCCESS = 1,
    RESULT_FAILURE = 2,
    BUTTON_UP = 0,
    BUTTON_DOWN = 1,
    BUTTON_OK = 2,
    BUTTON_EVENT_CLICK = 0,
    BUTTON_EVENT_LONG = 1,
    BUTTON_EVENT_LONG_HOLD = 2,
    PASSWORD_MASK_CAPACITY = 32,
};

enum {
    PARAMETER_HIDDEN = 0,
    PARAMETER_RANDOM_LENGTH = 1,
    PARAMETER_LOWERCASE = 2,
    PARAMETER_UPPERCASE = 3,
    PARAMETER_DIGITS = 4,
    PARAMETER_SYMBOLS = 5,
    PARAMETER_WORD_COUNT = 6,
    PARAMETER_CAPITALIZE = 7,
    PARAMETER_COMPLETE_WORD = 8,
    PARAMETER_SEPARATOR = 9,
    PARAMETER_PIN_LENGTH = 10,
};

static lv_obj_t *s_screen;
static lv_obj_t *s_mode_panels[3];
static lv_obj_t *s_mode_labels[3];
static lv_obj_t *s_parameter_labels[5];
static lv_obj_t *s_status_label;
static lv_obj_t *s_entropy_label;
static lv_obj_t *s_result_label;
static lv_obj_t *s_generate_panel;
static lv_obj_t *s_generate_label;
static lv_obj_t *s_send_panel;
static lv_obj_t *s_send_label;
static lv_obj_t *s_ble_label;
static lv_obj_t *s_battery_label;
static lv_timer_t *s_battery_timer;
static lv_timer_t *s_ble_timer;
static lv_timer_t *s_visibility_timer;
static uint64_t s_state;
static ui_pixel_theme_t s_theme;
static bool s_in_settings;
static bool s_has_password;
static bool s_send_focused;
static bool s_password_reveal_started;
static uint32_t s_password_reveal_started_at;
static int s_password_display_mode;

static int state_value(int field)
{
    return passport_moonbit_state_get(s_state, field);
}

static void set_focus_style(lv_obj_t *object, bool focused, bool editing)
{
    lv_obj_set_style_bg_opa(object, focused ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_color(
        object,
        lv_color_hex(editing ? UI_ORANGE : UI_YELLOW),
        0
    );
    lv_obj_set_style_border_width(object, focused ? 2 : 0, 0);
    lv_obj_set_style_border_color(
        object,
        lv_color_hex(editing ? UI_YELLOW : UI_GRASS_DARK),
        0
    );
    lv_obj_set_style_text_color(
        object,
        lv_color_hex(focused ? UI_INK : UI_TEXT),
        0
    );
}

static void configure_parameter_label(int index)
{
    lv_obj_t *label = s_parameter_labels[index];
    lv_obj_remove_flag(label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(
        label,
        passport_moonbit_view_parameter_x(s_state, index),
        passport_moonbit_view_parameter_y(s_state, index)
    );
    lv_obj_set_size(
        label,
        passport_moonbit_view_parameter_width(s_state, index),
        passport_moonbit_view_parameter_height(s_state, index)
    );
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_top(label, 3, 0);
}

static void refresh_modes(void)
{
    for (int i = 0; i < 3; i++) {
        bool current = passport_moonbit_view_mode_selected(s_state, i) != 0;
        bool focused = passport_moonbit_view_mode_focused(s_state, i) != 0;
        lv_obj_set_style_bg_color(
            s_mode_panels[i],
            lv_color_hex(current ? UI_YELLOW : UI_PAPER),
            0
        );
        lv_obj_set_style_border_color(
            s_mode_panels[i],
            lv_color_hex(focused ? UI_GRASS_DARK : UI_SKY_DARK),
            0
        );
        lv_obj_set_style_text_color(
            s_mode_labels[i], lv_color_hex(current ? UI_INK : UI_TEXT), 0
        );
    }
}

/*
 * 与已通过同一台 AI Passport 真机验证的 Codex Buddy 一致：优先显示 CW2017
 * 的 SOC；SOC 不可用时才按 3.3V～4.2V 线性换算并用“~”明确标为估算值。
 */
static void refresh_battery(lv_timer_t *timer)
{
    (void)timer;
    int reading = passport_moonbit_battery_resolve(
        bsp_battery_soc(), bsp_battery_mv()
    );

    if (passport_moonbit_battery_available(reading)) {
        int percent = passport_moonbit_battery_percent(reading);
        lv_label_set_text_fmt(
            s_battery_label,
            passport_moonbit_battery_estimated(reading) ? "~%d%%" : "%d%%",
            percent
        );
        lv_obj_set_style_text_color(
            s_battery_label,
            lv_color_hex(passport_moonbit_battery_low(reading) ? UI_RED : UI_LIME),
            0
        );
    } else {
        lv_label_set_text(s_battery_label, "--%");
        lv_obj_set_style_text_color(s_battery_label, lv_color_hex(UI_MUTED), 0);
    }
}

static void refresh_parameters(void)
{
    static const char *separator_text[] = {"-", ".", "_"};
    for (int i = 0; i < 5; i++) {
        lv_obj_add_flag(s_parameter_labels[i], LV_OBJ_FLAG_HIDDEN);
        set_focus_style(s_parameter_labels[i], false, false);
        int kind = passport_moonbit_view_parameter_kind(s_state, i);
        int value = passport_moonbit_view_parameter_value(s_state, i);
        if (kind == PARAMETER_HIDDEN) continue;
        configure_parameter_label(i);
        switch (kind) {
        case PARAMETER_RANDOM_LENGTH:
            lv_label_set_text_fmt(s_parameter_labels[i], "长度 %d", value);
            break;
        case PARAMETER_LOWERCASE:
            lv_label_set_text_fmt(s_parameter_labels[i], "小写 %s", value ? "ON" : "OFF");
            break;
        case PARAMETER_UPPERCASE:
            lv_label_set_text_fmt(s_parameter_labels[i], "大写 %s", value ? "ON" : "OFF");
            break;
        case PARAMETER_DIGITS:
            lv_label_set_text_fmt(s_parameter_labels[i], "数字 %s", value ? "ON" : "OFF");
            break;
        case PARAMETER_SYMBOLS:
            lv_label_set_text_fmt(s_parameter_labels[i], "符号 %s", value ? "ON" : "OFF");
            break;
        case PARAMETER_WORD_COUNT:
            lv_label_set_text_fmt(s_parameter_labels[i], "单词数 %d", value);
            break;
        case PARAMETER_CAPITALIZE:
            lv_label_set_text_fmt(s_parameter_labels[i], "首字母 %s", value ? "ON" : "OFF");
            break;
        case PARAMETER_COMPLETE_WORD:
            lv_label_set_text_fmt(s_parameter_labels[i], "完整单词 %s", value ? "ON" : "OFF");
            break;
        case PARAMETER_SEPARATOR:
            lv_label_set_text_fmt(s_parameter_labels[i], "分隔符 %s", separator_text[value]);
            break;
        case PARAMETER_PIN_LENGTH:
            lv_label_set_text_fmt(s_parameter_labels[i], "PIN 位数 %d", value);
            break;
        default:
            break;
        }
        set_focus_style(
            s_parameter_labels[i],
            passport_moonbit_view_parameter_selected(s_state, i) != 0,
            passport_moonbit_view_parameter_editing(s_state, i) != 0
        );
    }
}

static void refresh_result(void)
{
    int result = state_value(FIELD_RESULT);
    s_has_password = result == RESULT_SUCCESS;
    uint64_t elapsed_ms = s_password_reveal_started
        ? (uint64_t)lv_tick_elaps(s_password_reveal_started_at)
        : passport_moonbit_password_display_timeout_ms();
    s_password_display_mode = passport_moonbit_password_display_update(
        s_state, s_password_display_mode, elapsed_ms
    );
    int warning = passport_moonbit_security_warning(s_state);
    static const char *profile_names[] = {"兼容", "标准", "严格"};
    static const char *warning_names[] = {
        "", "长度过短", "缺少小写", "缺少大写", "缺少数字", "缺少符号", "配置无效"
    };
    if (result == RESULT_FAILURE) {
        lv_label_set_text(s_status_label, "生成失败 请重试");
        lv_obj_set_style_text_color(s_status_label, lv_color_hex(UI_RED), 0);
        lv_label_set_text(s_result_label, "");
        return;
    }
    if (state_value(FIELD_MODE) == 0) {
        int profile = state_value(FIELD_POLICY_PROFILE);
        const char *profile_name = profile >= 0 && profile < 3
            ? profile_names[profile] : profile_names[1];
        if (warning > 0 && warning < 7) {
            lv_label_set_text_fmt(s_status_label, "%s %s", profile_name, warning_names[warning]);
            lv_obj_set_style_text_color(s_status_label, lv_color_hex(UI_ORANGE), 0);
        } else {
            lv_label_set_text_fmt(s_status_label, "策略 %s", profile_name);
            lv_obj_set_style_text_color(s_status_label, lv_color_hex(UI_LIME), 0);
        }
    } else {
        lv_label_set_text(s_status_label, result == RESULT_SUCCESS ? "已生成" : "");
        lv_obj_set_style_text_color(s_status_label, lv_color_hex(UI_LIME), 0);
    }
    if (result == RESULT_SUCCESS) {
        if (passport_moonbit_password_display_is_visible(
                s_password_display_mode
            )) {
            lv_label_set_text(s_result_label, password_platform_output());
        } else {
            char mask[PASSWORD_MASK_CAPACITY];
            int width = passport_moonbit_password_display_mask_width();
            int character = passport_moonbit_password_display_mask_character();
            if (width < 1) width = 1;
            if (width >= PASSWORD_MASK_CAPACITY) width = PASSWORD_MASK_CAPACITY - 1;
            for (int i = 0; i < width; i++) mask[i] = (char)character;
            mask[width] = '\0';
            lv_label_set_text(s_result_label, mask);
        }
    } else {
        lv_label_set_text(s_result_label, "OK -> Generate");
    }
}

static void refresh_password_visibility(lv_timer_t *timer)
{
    (void)timer;
    if (!s_result_label || !s_password_reveal_started ||
        !passport_moonbit_password_display_is_visible(
            s_password_display_mode
        )) return;

    uint64_t elapsed_ms = (uint64_t)lv_tick_elaps(s_password_reveal_started_at);
    int next = passport_moonbit_password_display_update(
        s_state, s_password_display_mode, elapsed_ms
    );
    if (next != s_password_display_mode) refresh_result();
}

static void refresh_ble(lv_timer_t *timer)
{
    (void)timer;
    if (!s_ble_label || !s_send_panel || !s_send_label) return;

    int status = passport_moonbit_ble_keyboard_status(
        password_ble_keyboard_status()
    );
    switch (status) {
    case PASSWORD_BLE_STARTING:
        lv_label_set_text(s_ble_label, "BLE 启动中");
        break;
    case PASSWORD_BLE_ADVERTISING:
        lv_label_set_text(s_ble_label, "BLE: 配对 FoloPassKey");
        break;
    case PASSWORD_BLE_PAIRING:
        lv_label_set_text(s_ble_label, "BLE 连接中 无需配对码");
        break;
    case PASSWORD_BLE_CONNECTED:
        lv_label_set_text(s_ble_label, "BLE 已连接 可发送");
        break;
    case PASSWORD_BLE_SENDING:
        lv_label_set_text(s_ble_label, "正在输入密码...");
        break;
    case PASSWORD_BLE_SENT:
        lv_label_set_text(s_ble_label, "密码已发送");
        break;
    default:
        lv_label_set_text(s_ble_label, "BLE 不可用");
        break;
    }

    bool enabled = s_has_password &&
        (status == PASSWORD_BLE_CONNECTED || status == PASSWORD_BLE_SENT);
    lv_obj_set_style_bg_color(
        s_send_panel,
        lv_color_hex(s_send_focused ? UI_YELLOW : UI_PAPER),
        0
    );
    lv_obj_set_style_border_color(
        s_send_panel,
        lv_color_hex(s_send_focused ? UI_GRASS_DARK :
            (enabled ? UI_SKY_DARK : UI_MUTED)),
        0
    );
    lv_obj_set_style_text_color(
        s_send_label,
        lv_color_hex(s_send_focused ? UI_INK : (enabled ? UI_TEXT : UI_MUTED)),
        0
    );
    lv_obj_set_style_text_color(
        s_ble_label,
        lv_color_hex(status == PASSWORD_BLE_ERROR ? UI_RED :
            (enabled ? UI_LIME : UI_MUTED)),
        0
    );
}

static void refresh_ui(void)
{
    refresh_modes();
    refresh_parameters();
    refresh_result();

    int entropy_x10 = passport_moonbit_entropy_x10(s_state);
    lv_label_set_text_fmt(s_entropy_label, "H %d.%d bit", entropy_x10 / 10, entropy_x10 % 10);
    int strength = passport_moonbit_strength(s_state);
    lv_obj_set_style_text_color(
        s_entropy_label,
        lv_color_hex(strength <= 1 ? UI_RED : (strength == 2 ? UI_ORANGE : UI_SKY_DARK)),
        0
    );
    lv_label_set_text(
        s_generate_label,
        state_value(FIELD_RESULT) == RESULT_SUCCESS ? "重新生成" : "生成"
    );
    bool generate_focused = passport_moonbit_view_generate_focused(s_state) != 0;
    lv_obj_set_style_bg_color(
        s_generate_panel,
        lv_color_hex(generate_focused ? UI_YELLOW : UI_PAPER),
        0
    );
    lv_obj_set_style_border_color(
        s_generate_panel,
        lv_color_hex(generate_focused ? UI_GRASS_DARK : UI_SKY_DARK),
        0
    );
    lv_obj_set_style_text_color(
        s_generate_label,
        lv_color_hex(generate_focused ? UI_INK : UI_TEXT),
        0
    );
    s_send_focused = passport_moonbit_view_send_focused(s_state) != 0;
    refresh_ble(NULL);
}

static void password_app_teardown_ui(void)
{
    if (s_battery_timer) {
        lv_timer_delete(s_battery_timer);
        s_battery_timer = NULL;
    }
    if (s_ble_timer) {
        lv_timer_delete(s_ble_timer);
        s_ble_timer = NULL;
    }
    if (s_visibility_timer) {
        lv_timer_delete(s_visibility_timer);
        s_visibility_timer = NULL;
    }
    if (s_screen) {
        lv_obj_delete(s_screen);
        s_screen = NULL;
    }
    s_battery_label = NULL;
    s_status_label = NULL;
    s_entropy_label = NULL;
    s_result_label = NULL;
    s_generate_panel = NULL;
    s_generate_label = NULL;
    s_send_panel = NULL;
    s_send_label = NULL;
    s_ble_label = NULL;
    for (int i = 0; i < 3; i++) {
        s_mode_panels[i] = NULL;
        s_mode_labels[i] = NULL;
    }
    for (int i = 0; i < 5; i++) {
        s_parameter_labels[i] = NULL;
    }
}

static void password_app_build_ui(void)
{
    static const char *mode_names[] = {"随机", "易记", "PIN"};
    lv_obj_t *old_screen = s_screen;
    lv_timer_t *old_battery_timer = s_battery_timer;
    lv_timer_t *old_ble_timer = s_ble_timer;
    lv_timer_t *old_visibility_timer = s_visibility_timer;

    s_screen = NULL;
    s_battery_timer = NULL;
    s_ble_timer = NULL;
    s_visibility_timer = NULL;

    s_theme = (ui_pixel_theme_t)passport_moonbit_view_theme(s_state);
    ui_pixel_set_theme(s_theme);

    s_screen = ui_pixel_screen_create_with_font("强密码生成器", &passport_font_zh_16);
    s_battery_label = ui_pixel_label(
        s_screen, "--%", &lv_font_montserrat_14, UI_LIME
    );
    lv_obj_set_pos(s_battery_label, 164, 25);
    lv_obj_set_width(s_battery_label, 70);
    lv_obj_set_style_text_align(s_battery_label, LV_TEXT_ALIGN_RIGHT, 0);
    refresh_battery(NULL);
    s_battery_timer = lv_timer_create(
        refresh_battery,
        (uint32_t)passport_moonbit_battery_refresh_interval_ms(),
        NULL
    );

    for (int i = 0; i < 3; i++) {
        s_mode_panels[i] = ui_pixel_panel_create(s_screen, 7 + i * 75, 49, 68, 31, UI_PAPER);
        s_mode_labels[i] = ui_pixel_label(
            s_mode_panels[i], mode_names[i], &passport_font_zh_16, UI_TEXT
        );
        lv_obj_center(s_mode_labels[i]);
    }

    lv_obj_t *parameter_panel = ui_pixel_panel_create(s_screen, 7, 89, 226, 76, UI_PAPER);
    for (int i = 0; i < 5; i++) {
        s_parameter_labels[i] = ui_pixel_label(
            parameter_panel, "", &passport_font_zh_16, UI_TEXT
        );
    }

    lv_obj_t *result_panel = ui_pixel_panel_create(s_screen, 7, 174, 226, 73, UI_PAPER);
    s_status_label = ui_pixel_label(result_panel, "", &passport_font_zh_16, UI_LIME);
    lv_obj_set_pos(s_status_label, 0, -2);
    lv_obj_set_width(s_status_label, 108);
    s_entropy_label = ui_pixel_label(result_panel, "", &lv_font_montserrat_14, UI_SKY_DARK);
    lv_obj_set_pos(s_entropy_label, 111, -1);
    lv_obj_set_width(s_entropy_label, 99);
    lv_obj_set_style_text_align(s_entropy_label, LV_TEXT_ALIGN_RIGHT, 0);
    s_result_label = ui_pixel_label(result_panel, "", &lv_font_montserrat_14, UI_TEXT);
    lv_obj_set_pos(s_result_label, 0, 20);
    lv_obj_set_size(s_result_label, 210, 43);
    lv_label_set_long_mode(s_result_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(s_result_label, LV_TEXT_ALIGN_CENTER, 0);

    s_generate_panel = ui_pixel_panel_create(s_screen, 7, 255, 108, 27, UI_PAPER);
    lv_obj_set_style_pad_all(s_generate_panel, 2, 0);
    s_generate_label = ui_pixel_label(
        s_generate_panel, "生成", &passport_font_zh_16, UI_TEXT
    );
    lv_obj_center(s_generate_label);

    s_send_panel = ui_pixel_panel_create(s_screen, 125, 255, 108, 27, UI_PAPER);
    lv_obj_set_style_pad_all(s_send_panel, 2, 0);
    s_send_label = ui_pixel_label(
        s_send_panel, "发送", &passport_font_zh_16, UI_MUTED
    );
    lv_obj_center(s_send_label);

    s_ble_label = ui_pixel_label(
        s_screen, "BLE 启动中", &passport_font_zh_16, UI_MUTED
    );
    lv_obj_set_pos(s_ble_label, 7, 288);
    lv_obj_set_width(s_ble_label, 226);
    lv_obj_set_style_text_align(s_ble_label, LV_TEXT_ALIGN_CENTER, 0);
    s_ble_timer = lv_timer_create(refresh_ble, 200, NULL);
    s_visibility_timer = lv_timer_create(refresh_password_visibility, 200, NULL);

    refresh_ui();
    lv_screen_load(s_screen);
    if (old_battery_timer) lv_timer_delete(old_battery_timer);
    if (old_ble_timer) lv_timer_delete(old_ble_timer);
    if (old_visibility_timer) lv_timer_delete(old_visibility_timer);
    if (old_screen) lv_obj_delete(old_screen);
}

static bool on_settings_exit(void)
{
    if (!bsp_lvgl_lock(500)) return false;
    uint64_t previous = s_state;
    s_state = passport_moonbit_with_theme(s_state, settings_store_theme());
    s_state = passport_moonbit_with_policy_settings(
        s_state,
        settings_store_policy_profile(),
        settings_store_exclude_ambiguous() ? 1 : 0
    );
    if (passport_moonbit_configuration_changed(previous, s_state)) {
        password_platform_clear_output();
        password_ble_keyboard_reset_feedback();
        s_password_reveal_started = false;
        s_password_display_mode = passport_moonbit_password_display_begin(s_state);
    }
    password_app_build_ui();
    bsp_lvgl_unlock();
    s_in_settings = false;
    return true;
}

void password_app_enter(void)
{
    s_in_settings = false;
    s_state = passport_moonbit_initial_state();
    s_state = passport_moonbit_with_theme(s_state, settings_store_theme());
    s_state = passport_moonbit_with_policy_settings(
        s_state,
        settings_store_policy_profile(),
        settings_store_exclude_ambiguous() ? 1 : 0
    );
    password_platform_clear_output();
    s_password_reveal_started = false;
    s_password_display_mode = passport_moonbit_password_display_begin(s_state);
    password_app_build_ui();
}

void password_app_handle_button(bsp_btn_t button, bsp_btn_ev_t event)
{
    if (s_in_settings) {
        settings_screen_handle_button(button, event);
        return;
    }

    if (!passport_moonbit_ble_keyboard_input_allowed(
            password_ble_keyboard_status()
        )) return;
    password_ble_keyboard_reset_feedback();

    int button_code = button == BSP_BTN_UP ? BUTTON_UP
        : (button == BSP_BTN_DOWN ? BUTTON_DOWN
        : (button == BSP_BTN_OK ? BUTTON_OK : -1));
    int event_code = event == BSP_BTN_CLICK ? BUTTON_EVENT_CLICK
        : (event == BSP_BTN_LONG ? BUTTON_EVENT_LONG
        : (event == BSP_BTN_LONG_HOLD ? BUTTON_EVENT_LONG_HOLD : -1));
    int input = passport_moonbit_map_button_event(
        s_state, button_code, event_code
    );
    if (input < 0) return;

    uint64_t previous = s_state;
    s_state = passport_moonbit_handle_input(s_state, input);
    int previous_theme = passport_moonbit_view_theme(previous);
    int next_theme = passport_moonbit_view_theme(s_state);
    if (previous_theme != next_theme) {
        (void)settings_store_set_theme(next_theme);
    }
    if (passport_moonbit_configuration_changed(previous, s_state)) {
        password_platform_clear_output();
        password_ble_keyboard_reset_feedback();
        s_password_reveal_started = false;
        s_password_display_mode = passport_moonbit_password_display_begin(s_state);
    }

    int action = passport_moonbit_state_action(s_state);
    if (action == ACTION_OPEN_SETTINGS) {
        if (settings_screen_enter(s_state, on_settings_exit)) {
            s_in_settings = true;
            if (bsp_lvgl_lock(500)) {
                password_app_teardown_ui();
                bsp_lvgl_unlock();
            }
        }
        return;
    } else if (action == ACTION_GENERATE) {
        password_ble_keyboard_reset_feedback();
        int result = passport_moonbit_configuration_valid(s_state)
            ? passport_moonbit_generate(s_state) : -1;
        s_state = passport_moonbit_record_generation(s_state, result);
        if (result != 0) {
            password_platform_clear_output();
            s_password_reveal_started = false;
        } else {
            s_password_reveal_started_at = lv_tick_get();
            s_password_reveal_started = true;
            if (settings_store_sound_enabled()) password_sound_play_success();
        }
        s_password_display_mode = passport_moonbit_password_display_begin(s_state);
    } else if (action == ACTION_SEND &&
               passport_moonbit_ble_keyboard_send_allowed(
                   s_state, password_ble_keyboard_status()
               )) {
        (void)password_ble_keyboard_send(password_platform_output());
    }

    if (bsp_lvgl_lock(500)) {
        if ((ui_pixel_theme_t)passport_moonbit_view_theme(s_state) != s_theme) {
            password_app_build_ui();
        } else {
            refresh_ui();
        }
        bsp_lvgl_unlock();
    }
}

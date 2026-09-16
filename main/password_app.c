#include "password_app.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bsp_battery.h"
#include "bsp_display.h"
#include "lvgl.h"
#include "moonbit_password.h"
#include "password_platform.h"
#include "password_sound.h"
#include "ui_pixel.h"

LV_FONT_DECLARE(passport_font_zh_16);

enum {
    FIELD_RESULT = 11,
};

enum {
    ACTION_GENERATE = 1,
    RESULT_SUCCESS = 1,
    RESULT_FAILURE = 2,
    BUTTON_UP = 0,
    BUTTON_DOWN = 1,
    BUTTON_OK = 2,
    BUTTON_EVENT_CLICK = 0,
    BUTTON_EVENT_LONG = 1,
    BUTTON_EVENT_LONG_HOLD = 2,
};

enum {
    PARAMETER_HIDDEN = 0,
    PARAMETER_RANDOM_LENGTH = 1,
    PARAMETER_LETTERS = 2,
    PARAMETER_DIGITS = 3,
    PARAMETER_SYMBOLS = 4,
    PARAMETER_WORD_COUNT = 5,
    PARAMETER_CAPITALIZE = 6,
    PARAMETER_COMPLETE_WORD = 7,
    PARAMETER_SEPARATOR = 8,
    PARAMETER_PIN_LENGTH = 9,
};

static lv_obj_t *s_screen;
static lv_obj_t *s_mode_panels[3];
static lv_obj_t *s_mode_labels[3];
static lv_obj_t *s_parameter_labels[4];
static lv_obj_t *s_status_label;
static lv_obj_t *s_entropy_label;
static lv_obj_t *s_result_label;
static lv_obj_t *s_generate_panel;
static lv_obj_t *s_generate_label;
static lv_obj_t *s_battery_label;
static lv_timer_t *s_battery_timer;
static uint64_t s_state;

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
    for (int i = 0; i < 4; i++) {
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
        case PARAMETER_LETTERS:
            lv_label_set_text(s_parameter_labels[i], "字母 ON");
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
    if (result == RESULT_FAILURE) {
        lv_label_set_text(s_status_label, "生成失败 请重试");
        lv_label_set_text(s_result_label, "");
        return;
    }
    if (result == RESULT_SUCCESS) {
        lv_label_set_text(s_status_label, "已生成");
        lv_label_set_text(s_result_label, password_platform_output());
    } else {
        lv_label_set_text(s_status_label, "");
        lv_label_set_text(s_result_label, "OK -> Generate");
    }
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
}

void password_app_enter(void)
{
    static const char *mode_names[] = {"随机", "易记", "PIN"};
    s_state = passport_moonbit_initial_state();
    password_platform_clear_output();

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
    for (int i = 0; i < 4; i++) {
        s_parameter_labels[i] = ui_pixel_label(
            parameter_panel, "", &passport_font_zh_16, UI_TEXT
        );
    }

    lv_obj_t *result_panel = ui_pixel_panel_create(s_screen, 7, 174, 226, 73, UI_PAPER);
    s_status_label = ui_pixel_label(result_panel, "", &passport_font_zh_16, UI_LIME);
    lv_obj_set_pos(s_status_label, 0, -2);
    s_entropy_label = ui_pixel_label(result_panel, "", &lv_font_montserrat_14, UI_SKY_DARK);
    lv_obj_set_pos(s_entropy_label, 111, -1);
    lv_obj_set_width(s_entropy_label, 99);
    lv_obj_set_style_text_align(s_entropy_label, LV_TEXT_ALIGN_RIGHT, 0);
    s_result_label = ui_pixel_label(result_panel, "", &lv_font_montserrat_14, UI_TEXT);
    lv_obj_set_pos(s_result_label, 0, 20);
    lv_obj_set_size(s_result_label, 210, 43);
    lv_label_set_long_mode(s_result_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(s_result_label, LV_TEXT_ALIGN_CENTER, 0);

    s_generate_panel = ui_pixel_panel_create(s_screen, 34, 255, 172, 27, UI_PAPER);
    lv_obj_set_style_pad_all(s_generate_panel, 2, 0);
    s_generate_label = ui_pixel_label(
        s_generate_panel, "生成", &passport_font_zh_16, UI_TEXT
    );
    lv_obj_center(s_generate_label);

    refresh_ui();
    lv_screen_load(s_screen);
}

void password_app_handle_button(bsp_btn_t button, bsp_btn_ev_t event)
{
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
    if (passport_moonbit_configuration_changed(previous, s_state)) {
        password_platform_clear_output();
    }

    if (passport_moonbit_state_action(s_state) == ACTION_GENERATE) {
        int result = passport_moonbit_configuration_valid(s_state)
            ? passport_moonbit_generate(s_state) : -1;
        s_state = passport_moonbit_record_generation(s_state, result);
        if (result != 0) password_platform_clear_output();
        else password_sound_play_success();
    }

    if (bsp_lvgl_lock(500)) {
        refresh_ui();
        bsp_lvgl_unlock();
    }
}

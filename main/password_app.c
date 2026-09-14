#include "password_app.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bsp_battery.h"
#include "bsp_display.h"
#include "lvgl.h"
#include "moonbit_password.h"
#include "password_platform.h"
#include "ui_pixel.h"

LV_FONT_DECLARE(passport_font_zh_16);

enum {
    FIELD_MODE = 0,
    FIELD_FOCUS = 1,
    FIELD_EDITING = 2,
    FIELD_RANDOM_LENGTH = 3,
    FIELD_DIGITS = 4,
    FIELD_SYMBOLS = 5,
    FIELD_WORD_COUNT = 6,
    FIELD_CAPITALIZE = 7,
    FIELD_COMPLETE_WORD = 8,
    FIELD_SEPARATOR = 9,
    FIELD_PIN_LENGTH = 10,
};

enum {
    INPUT_UP = 0,
    INPUT_DOWN = 1,
    INPUT_OK = 2,
    INPUT_OK_LONG = 3,
    ACTION_GENERATE = 1,
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
static uint64_t s_state;
static bool s_has_result;
static bool s_last_generation_failed;

static int state_value(int field)
{
    return passport_moonbit_state_get(s_state, field);
}

static int maximum_focus(int mode)
{
    static const int maximum[] = {4, 5, 2};
    return maximum[mode];
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
    lv_obj_set_style_border_color(object, lv_color_hex(UI_INK), 0);
    lv_obj_set_style_text_color(object, lv_color_hex(UI_INK), 0);
}

static void configure_parameter_label(int index, int x, int y)
{
    lv_obj_t *label = s_parameter_labels[index];
    lv_obj_remove_flag(label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_size(label, 100, 27);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_top(label, 3, 0);
}

static void refresh_modes(int mode, int focus)
{
    for (int i = 0; i < 3; i++) {
        bool current = i == mode;
        lv_obj_set_style_bg_color(
            s_mode_panels[i],
            lv_color_hex(current ? UI_YELLOW : UI_PAPER),
            0
        );
        lv_obj_set_style_border_color(
            s_mode_panels[i],
            lv_color_hex(current && focus == 0 ? 0xFFFFFF : UI_INK),
            0
        );
    }
}

static void refresh_parameters(int mode, int focus, bool editing)
{
    static const char *separator_text[] = {"-", ".", "_"};
    int focus_map[4] = {0, 0, 0, 0};
    for (int i = 0; i < 4; i++) {
        lv_obj_add_flag(s_parameter_labels[i], LV_OBJ_FLAG_HIDDEN);
        set_focus_style(s_parameter_labels[i], false, false);
    }

    if (mode == 0) {
        configure_parameter_label(0, 2, 2);
        configure_parameter_label(1, 106, 2);
        configure_parameter_label(2, 2, 34);
        configure_parameter_label(3, 106, 34);
        lv_label_set_text_fmt(s_parameter_labels[0], "长度 %d", state_value(FIELD_RANDOM_LENGTH));
        lv_label_set_text(s_parameter_labels[1], "字母 ON");
        lv_label_set_text_fmt(s_parameter_labels[2], "数字 %s", state_value(FIELD_DIGITS) ? "ON" : "OFF");
        lv_label_set_text_fmt(s_parameter_labels[3], "符号 %s", state_value(FIELD_SYMBOLS) ? "ON" : "OFF");
        focus_map[0] = 1;
        focus_map[2] = 2;
        focus_map[3] = 3;
    } else if (mode == 1) {
        configure_parameter_label(0, 2, 2);
        configure_parameter_label(1, 106, 2);
        configure_parameter_label(2, 2, 34);
        configure_parameter_label(3, 106, 34);
        lv_label_set_text_fmt(s_parameter_labels[0], "单词数 %d", state_value(FIELD_WORD_COUNT));
        lv_label_set_text_fmt(s_parameter_labels[1], "首字母 %s", state_value(FIELD_CAPITALIZE) ? "ON" : "OFF");
        lv_label_set_text_fmt(s_parameter_labels[2], "完整单词 %s", state_value(FIELD_COMPLETE_WORD) ? "ON" : "OFF");
        lv_label_set_text_fmt(s_parameter_labels[3], "分隔符 %s", separator_text[state_value(FIELD_SEPARATOR)]);
        focus_map[0] = 1;
        focus_map[1] = 2;
        focus_map[2] = 3;
        focus_map[3] = 4;
    } else {
        configure_parameter_label(0, 54, 18);
        lv_label_set_text_fmt(s_parameter_labels[0], "PIN 位数 %d", state_value(FIELD_PIN_LENGTH));
        focus_map[0] = 1;
    }

    for (int i = 0; i < 4; i++) {
        bool selected = focus_map[i] != 0 && focus == focus_map[i];
        set_focus_style(s_parameter_labels[i], selected, selected && editing);
    }
}

static void refresh_result(void)
{
    if (s_last_generation_failed) {
        lv_label_set_text(s_status_label, "生成失败 请重试");
        lv_label_set_text(s_result_label, "");
        return;
    }
    if (s_has_result) {
        lv_label_set_text(s_status_label, "已生成");
        lv_label_set_text(s_result_label, password_platform_output());
    } else {
        lv_label_set_text(s_status_label, "");
        lv_label_set_text(s_result_label, "OK -> Generate");
    }
}

static void refresh_ui(void)
{
    int mode = state_value(FIELD_MODE);
    int focus = state_value(FIELD_FOCUS);
    bool editing = state_value(FIELD_EDITING) != 0;
    refresh_modes(mode, focus);
    refresh_parameters(mode, focus, editing);
    refresh_result();

    int entropy_x10 = passport_moonbit_entropy_x10(s_state);
    lv_label_set_text_fmt(s_entropy_label, "H %d.%d bit", entropy_x10 / 10, entropy_x10 % 10);
    lv_label_set_text(s_generate_label, s_has_result ? "重新生成" : "生成");
    bool generate_focused = focus == maximum_focus(mode);
    lv_obj_set_style_bg_color(
        s_generate_panel,
        lv_color_hex(generate_focused ? UI_YELLOW : UI_PAPER),
        0
    );
    lv_obj_set_style_border_color(
        s_generate_panel,
        lv_color_hex(generate_focused ? 0xFFFFFF : UI_INK),
        0
    );
}

static bool configuration_changed(uint64_t before, uint64_t after)
{
    static const int fields[] = {
        FIELD_MODE, FIELD_RANDOM_LENGTH, FIELD_DIGITS, FIELD_SYMBOLS,
        FIELD_WORD_COUNT, FIELD_CAPITALIZE, FIELD_COMPLETE_WORD,
        FIELD_SEPARATOR, FIELD_PIN_LENGTH,
    };
    for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); i++) {
        if (passport_moonbit_state_get(before, fields[i]) !=
            passport_moonbit_state_get(after, fields[i])) return true;
    }
    return false;
}

void password_app_enter(void)
{
    static const char *mode_names[] = {"随机", "易记", "PIN"};
    s_state = passport_moonbit_initial_state();
    s_has_result = false;
    s_last_generation_failed = false;
    password_platform_clear_output();

    s_screen = ui_pixel_screen_create_with_font("强密码生成器", &passport_font_zh_16);
    int battery = bsp_battery_soc();
    lv_obj_t *battery_label = ui_pixel_label(s_screen, "", &lv_font_montserrat_14, 0xFFFFFF);
    lv_obj_set_pos(battery_label, 164, 27);
    if (battery >= 0) {
        lv_label_set_text_fmt(battery_label, "%d%%", battery);
    } else {
        lv_label_set_text(battery_label, "--%");
    }

    for (int i = 0; i < 3; i++) {
        s_mode_panels[i] = ui_pixel_panel_create(s_screen, 7 + i * 75, 49, 68, 31, UI_PAPER);
        s_mode_labels[i] = ui_pixel_label(
            s_mode_panels[i], mode_names[i], &passport_font_zh_16, UI_INK
        );
        lv_obj_center(s_mode_labels[i]);
    }

    lv_obj_t *parameter_panel = ui_pixel_panel_create(s_screen, 7, 89, 226, 76, UI_PAPER);
    for (int i = 0; i < 4; i++) {
        s_parameter_labels[i] = ui_pixel_label(
            parameter_panel, "", &passport_font_zh_16, UI_INK
        );
    }

    lv_obj_t *result_panel = ui_pixel_panel_create(s_screen, 7, 174, 226, 73, UI_PAPER);
    s_status_label = ui_pixel_label(result_panel, "", &passport_font_zh_16, UI_GRASS_DARK);
    lv_obj_set_pos(s_status_label, 0, -2);
    s_entropy_label = ui_pixel_label(result_panel, "", &lv_font_montserrat_14, UI_SKY_DARK);
    lv_obj_set_pos(s_entropy_label, 111, -1);
    lv_obj_set_width(s_entropy_label, 99);
    lv_obj_set_style_text_align(s_entropy_label, LV_TEXT_ALIGN_RIGHT, 0);
    s_result_label = ui_pixel_label(result_panel, "", &lv_font_montserrat_14, UI_INK);
    lv_obj_set_pos(s_result_label, 0, 20);
    lv_obj_set_size(s_result_label, 210, 43);
    lv_label_set_long_mode(s_result_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(s_result_label, LV_TEXT_ALIGN_CENTER, 0);

    s_generate_panel = ui_pixel_panel_create(s_screen, 34, 255, 172, 27, UI_PAPER);
    lv_obj_set_style_pad_all(s_generate_panel, 2, 0);
    s_generate_label = ui_pixel_label(
        s_generate_panel, "生成", &passport_font_zh_16, UI_INK
    );
    lv_obj_center(s_generate_label);

    refresh_ui();
    lv_screen_load(s_screen);
}

void password_app_handle_button(bsp_btn_t button, bsp_btn_ev_t event)
{
    int input = -1;
    if (button == BSP_BTN_OK && event == BSP_BTN_LONG) input = INPUT_OK_LONG;
    if (event == BSP_BTN_CLICK) {
        if (button == BSP_BTN_UP) input = INPUT_UP;
        if (button == BSP_BTN_DOWN) input = INPUT_DOWN;
        if (button == BSP_BTN_OK) input = INPUT_OK;
    }
    if (input < 0) return;

    uint64_t previous = s_state;
    s_state = passport_moonbit_handle_input(s_state, input);
    if (configuration_changed(previous, s_state)) {
        password_platform_clear_output();
        s_has_result = false;
        s_last_generation_failed = false;
    }

    if (passport_moonbit_state_action(s_state) == ACTION_GENERATE) {
        int result = passport_moonbit_generate(s_state);
        s_has_result = result == 0;
        s_last_generation_failed = result != 0;
        if (result != 0) password_platform_clear_output();
    }

    if (bsp_lvgl_lock(500)) {
        refresh_ui();
        bsp_lvgl_unlock();
    }
}

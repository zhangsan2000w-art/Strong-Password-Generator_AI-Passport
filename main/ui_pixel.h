#pragma once

#include <stdint.h>

#include "lvgl.h"

/*
 * 双主题 UI 基元。主题选择策略由 MoonBit 状态机持有（view_theme），
 * 本层只负责根据当前主题渲染赛博朋克风或蓝天白云风。颜色宏解析到
 * 当前活动调色板，业务页面无需复制布局原语。
 */
typedef enum {
    UI_PIXEL_THEME_CYBER = 0,
    UI_PIXEL_THEME_SKY = 1,
} ui_pixel_theme_t;

typedef struct {
    uint32_t sky;
    uint32_t sky_dark;
    uint32_t ink;
    uint32_t paper;
    uint32_t grass;
    uint32_t grass_dark;
    uint32_t yellow;
    uint32_t orange;
    uint32_t red;
    uint32_t muted;
    uint32_t text;
    uint32_t lime;
    uint32_t violet;
} ui_pixel_palette_t;

const ui_pixel_palette_t *ui_pixel_palette(void);
void ui_pixel_set_theme(ui_pixel_theme_t theme);
ui_pixel_theme_t ui_pixel_get_theme(void);

#define UI_SKY        (ui_pixel_palette()->sky)
#define UI_SKY_DARK   (ui_pixel_palette()->sky_dark)
#define UI_INK        (ui_pixel_palette()->ink)
#define UI_PAPER      (ui_pixel_palette()->paper)
#define UI_GRASS      (ui_pixel_palette()->grass)
#define UI_GRASS_DARK (ui_pixel_palette()->grass_dark)
#define UI_YELLOW     (ui_pixel_palette()->yellow)
#define UI_ORANGE     (ui_pixel_palette()->orange)
#define UI_RED        (ui_pixel_palette()->red)
#define UI_MUTED      (ui_pixel_palette()->muted)
#define UI_TEXT       (ui_pixel_palette()->text)
#define UI_LIME       (ui_pixel_palette()->lime)
#define UI_VIOLET     (ui_pixel_palette()->violet)

lv_obj_t *ui_pixel_screen_create(const char *title);
lv_obj_t *ui_pixel_screen_create_with_font(const char *title, const lv_font_t *font);
lv_obj_t *ui_pixel_panel_create(lv_obj_t *parent, int x, int y, int w, int h,
                                uint32_t color);
lv_obj_t *ui_pixel_label(lv_obj_t *parent, const char *text,
                         const lv_font_t *font, uint32_t color);
lv_obj_t *ui_pixel_mascot_create(lv_obj_t *parent, int x, int y);
void ui_pixel_mascot_jump(lv_obj_t *mascot);
void ui_pixel_set_selected(lv_obj_t *panel, bool selected, bool enabled);
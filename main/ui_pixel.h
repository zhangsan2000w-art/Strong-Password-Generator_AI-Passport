#pragma once

#include "lvgl.h"

/* 深色赛博朋克调色板。保留 ui_pixel API，避免业务页面复制布局原语。 */
#define UI_SKY        0x050816
#define UI_SKY_DARK   0x00D9FF
#define UI_INK        0x02040C
#define UI_PAPER      0x0C1730
#define UI_GRASS      0x17103B
#define UI_GRASS_DARK 0xFF2BD6
#define UI_YELLOW     0x00F5FF
#define UI_ORANGE     0xFF2BD6
#define UI_RED        0xFF3B78
#define UI_MUTED      0x536787
#define UI_TEXT       0xEAFBFF
#define UI_LIME       0xB6FF2E
#define UI_VIOLET     0x7A2CFF

lv_obj_t *ui_pixel_screen_create(const char *title);
lv_obj_t *ui_pixel_screen_create_with_font(const char *title, const lv_font_t *font);
lv_obj_t *ui_pixel_panel_create(lv_obj_t *parent, int x, int y, int w, int h,
                                uint32_t color);
lv_obj_t *ui_pixel_label(lv_obj_t *parent, const char *text,
                         const lv_font_t *font, uint32_t color);
lv_obj_t *ui_pixel_mascot_create(lv_obj_t *parent, int x, int y);
void ui_pixel_mascot_jump(lv_obj_t *mascot);
void ui_pixel_set_selected(lv_obj_t *panel, bool selected, bool enabled);

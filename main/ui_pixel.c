#include "ui_pixel.h"

static void start_blink(lv_obj_t *eye);

static const ui_pixel_palette_t s_cyber_palette = {
    .sky = 0x050816,
    .sky_dark = 0x00D9FF,
    .ink = 0x02040C,
    .paper = 0x0C1730,
    .grass = 0x17103B,
    .grass_dark = 0xFF2BD6,
    .yellow = 0x00F5FF,
    .orange = 0xFF2BD6,
    .red = 0xFF3B78,
    .muted = 0x536787,
    .text = 0xEAFBFF,
    .lime = 0xB6FF2E,
    .violet = 0x7A2CFF,
};

static const ui_pixel_palette_t s_sky_palette = {
    .sky = 0x1689E8,
    .sky_dark = 0x0872C9,
    .ink = 0x17202A,
    .paper = 0xF4F4EA,
    .grass = 0x82BE2D,
    .grass_dark = 0x55951D,
    .yellow = 0xFFD928,
    .orange = 0xFFB23E,
    .red = 0xE43B2F,
    .muted = 0xD9E7EC,
    .text = 0x17202A,
    .lime = 0x82BE2D,
    .violet = 0x0872C9,
};

static const ui_pixel_palette_t *s_palette = &s_cyber_palette;

const ui_pixel_palette_t *ui_pixel_palette(void)
{
    return s_palette;
}

ui_pixel_theme_t ui_pixel_get_theme(void)
{
    return s_palette == &s_sky_palette ? UI_PIXEL_THEME_SKY
                                       : UI_PIXEL_THEME_CYBER;
}

void ui_pixel_set_theme(ui_pixel_theme_t theme)
{
    s_palette = theme == UI_PIXEL_THEME_SKY ? &s_sky_palette : &s_cyber_palette;
}

static lv_obj_t *block(lv_obj_t *parent, int x, int y, int w, int h, uint32_t color)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
    return obj;
}

lv_obj_t *ui_pixel_label(lv_obj_t *parent, const char *text,
                         const lv_font_t *font, uint32_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    return label;
}

/* 蓝天白云风：白云与草地。 */
static void add_cloud(lv_obj_t *parent, int x, int y)
{
    block(parent, x + 1, y + 7, 43, 10, UI_INK);
    block(parent, x + 5, y + 4, 35, 10, 0xFFFFFF);
    block(parent, x + 12, y, 10, 9, 0xFFFFFF);
    block(parent, x + 27, y + 1, 9, 8, 0xFFFFFF);
}

static void add_sky_decor(lv_obj_t *parent)
{
    add_cloud(parent, 188, 8);
    block(parent, 0, 286, 240, 34, UI_GRASS);
    block(parent, 0, 286, 240, 4, 0xA7D93E);
    for (int x = 0; x < 240; x += 30) {
        block(parent, x, 312, 18, 8, UI_GRASS_DARK);
        block(parent, x + 18, 316, 12, 4, 0x75452E);
    }
}

/* 赛博朋克风：顶部信号条与底部透视网格，保持纯矩形、低 RAM。 */
static void add_cyber_decor(lv_obj_t *parent)
{
    block(parent, 190, 9, 9, 3, UI_VIOLET);
    block(parent, 202, 9, 14, 3, UI_SKY_DARK);
    block(parent, 219, 9, 12, 3, UI_GRASS_DARK);

    block(parent, 0, 286, 240, 34, 0x080D22);
    block(parent, 0, 286, 240, 2, UI_GRASS_DARK);
    block(parent, 0, 304, 240, 1, UI_SKY_DARK);
    for (int x = 0; x < 240; x += 30) {
        block(parent, x, 288, 1, 32, (x % 60) == 0 ? UI_VIOLET : 0x173458);
    }
}

static void add_theme_decor(lv_obj_t *parent)
{
    if (ui_pixel_get_theme() == UI_PIXEL_THEME_SKY) {
        add_sky_decor(parent);
    } else {
        add_cyber_decor(parent);
    }
}

lv_obj_t *ui_pixel_screen_create_with_font(const char *title, const lv_font_t *font)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(scr, lv_color_hex(UI_SKY), 0);
    lv_obj_set_style_border_width(scr, 0, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);

    add_theme_decor(scr);

    lv_obj_t *plate;
    lv_obj_t *heading;
    if (ui_pixel_get_theme() == UI_PIXEL_THEME_SKY) {
        block(scr, 9, 12, 151, 33, UI_INK);
        plate = block(scr, 5, 8, 151, 33, UI_PAPER);
        lv_obj_set_style_border_color(plate, lv_color_hex(UI_INK), 0);
        lv_obj_set_style_border_width(plate, 3, 0);
        heading = ui_pixel_label(plate, title, font, UI_TEXT);
    } else {
        block(scr, 9, 12, 151, 33, UI_GRASS_DARK);
        plate = block(scr, 5, 8, 151, 33, UI_PAPER);
        lv_obj_set_style_border_color(plate, lv_color_hex(UI_SKY_DARK), 0);
        lv_obj_set_style_border_width(plate, 2, 0);
        heading = ui_pixel_label(plate, title, font, UI_TEXT);
    }
    lv_obj_center(heading);
    return scr;
}

lv_obj_t *ui_pixel_screen_create(const char *title)
{
    return ui_pixel_screen_create_with_font(title, &lv_font_montserrat_20);
}

lv_obj_t *ui_pixel_panel_create(lv_obj_t *parent, int x, int y, int w, int h,
                                uint32_t color)
{
    lv_obj_t *panel;
    if (ui_pixel_get_theme() == UI_PIXEL_THEME_SKY) {
        block(parent, x + 5, y + 6, w, h, UI_INK);
        panel = block(parent, x, y, w, h, color);
        lv_obj_set_style_border_color(panel, lv_color_hex(UI_INK), 0);
        lv_obj_set_style_border_width(panel, 4, 0);
    } else {
        block(parent, x + 4, y + 5, w, h, UI_GRASS_DARK);
        panel = block(parent, x, y, w, h, color);
        lv_obj_set_style_border_color(panel, lv_color_hex(UI_SKY_DARK), 0);
        lv_obj_set_style_border_width(panel, 2, 0);
    }
    lv_obj_set_style_pad_all(panel, 7, 0);
    return panel;
}

lv_obj_t *ui_pixel_mascot_create(lv_obj_t *parent, int x, int y)
{
    lv_obj_t *m = lv_obj_create(parent);
    lv_obj_remove_flag(m, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(m, x, y);
    lv_obj_set_size(m, 38, 48);
    lv_obj_set_style_bg_opa(m, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(m, 0, 0);
    lv_obj_set_style_pad_all(m, 0, 0);

    /* 原创“小电视机器人”：天线、发光屏幕脸、橙色围巾与履带脚。 */
    block(m, 18, 0, 3, 6, UI_INK);
    block(m, 16, 0, 7, 3, UI_ORANGE);
    block(m, 3, 6, 32, 24, UI_INK);
    block(m, 0, 12, 5, 10, 0x7557D9);
    block(m, 33, 12, 5, 10, 0x7557D9);
    block(m, 7, 10, 24, 16, 0xB9F3FF);
    lv_obj_t *left_eye = block(m, 11, 14, 4, 6, 0x294B7A);
    lv_obj_t *right_eye = block(m, 23, 14, 4, 6, 0x294B7A);
    block(m, 16, 22, 7, 2, 0x7557D9);
    block(m, 10, 29, 18, 4, UI_ORANGE);
    block(m, 8, 33, 22, 11, 0x7557D9);
    block(m, 3, 35, 5, 7, 0xB9F3FF);
    block(m, 30, 35, 5, 7, 0xB9F3FF);
    block(m, 8, 44, 9, 4, UI_INK);
    block(m, 21, 44, 9, 4, UI_INK);
    start_blink(left_eye);
    start_blink(right_eye);
    return m;
}

static void jump_y(void *obj, int32_t value)
{
    lv_obj_set_y((lv_obj_t *)obj, value);
}

static void blink_eye(void *obj, int32_t value)
{
    lv_obj_set_style_opa((lv_obj_t *)obj, (lv_opa_t)value, 0);
}

static void start_blink(lv_obj_t *eye)
{
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, eye);
    lv_anim_set_exec_cb(&anim, blink_eye);
    lv_anim_set_values(&anim, LV_OPA_COVER, LV_OPA_20);
    lv_anim_set_duration(&anim, 70);
    lv_anim_set_playback_duration(&anim, 70);
    lv_anim_set_repeat_delay(&anim, 1700);
    lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&anim, lv_anim_path_step);
    lv_anim_start(&anim);
}

void ui_pixel_mascot_jump(lv_obj_t *mascot)
{
    if (!mascot) return;
    int y = lv_obj_get_y(mascot);
    lv_anim_delete(mascot, jump_y);
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, mascot);
    lv_anim_set_exec_cb(&anim, jump_y);
    lv_anim_set_values(&anim, y, y - 5);
    lv_anim_set_duration(&anim, 110);
    lv_anim_set_playback_duration(&anim, 140);
    lv_anim_set_path_cb(&anim, lv_anim_path_step);
    lv_anim_start(&anim);
}

void ui_pixel_set_selected(lv_obj_t *panel, bool selected, bool enabled)
{
    uint32_t color = !enabled ? UI_MUTED : (selected ? UI_YELLOW : UI_PAPER);
    lv_obj_set_style_bg_color(panel, lv_color_hex(color), 0);
    lv_obj_set_style_border_color(panel,
        lv_color_hex(selected ? UI_GRASS_DARK : UI_SKY_DARK), 0);
}
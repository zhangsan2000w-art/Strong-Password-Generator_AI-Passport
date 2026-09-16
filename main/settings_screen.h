#pragma once

#include <stdbool.h>

#include "bsp_button.h"

typedef bool (*settings_screen_on_exit_t)(void);

/* 持 LVGL 锁创建并载入设置页；失败时保持原页面。 */
bool settings_screen_enter(settings_screen_on_exit_t on_exit);

/* 按键由 input 任务转发；函数自行缩短 LVGL 锁范围。 */
void settings_screen_handle_button(bsp_btn_t button, bsp_btn_ev_t event);

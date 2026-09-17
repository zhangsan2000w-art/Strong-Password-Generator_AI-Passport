#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "bsp_button.h"

typedef bool (*settings_screen_on_exit_t)(void);

/* 持 LVGL 锁创建并载入设置页；policy_state 是自检使用的只读快照。 */
bool settings_screen_enter(
    uint64_t policy_state,
    settings_screen_on_exit_t on_exit
);

/* 按键由 input 任务转发；函数自行缩短 LVGL 锁范围。 */
void settings_screen_handle_button(bsp_btn_t button, bsp_btn_ev_t event);

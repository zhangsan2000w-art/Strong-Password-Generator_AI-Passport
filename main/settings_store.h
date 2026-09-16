#pragma once

#include <stdbool.h>

#include "esp_err.h"

/*
 * 持久化用户设置（NVS）。主题取值为 0/1，与 ui_pixel_theme_t 及
 * MoonBit 的 theme_cyber/theme_sky 保持一致，便于直接转换。
 */
enum {
    SETTINGS_THEME_CYBER = 0,
    SETTINGS_THEME_SKY = 1,
};

/*
 * 初始化 NVS、载入设置并启动持久化任务。可重复调用；不会为了恢复
 * NVS 错误擦除整分区。
 */
esp_err_t settings_store_init(void);

int settings_store_theme(void);
bool settings_store_sound_enabled(void);

/*
 * 立即更新内存值并把最新快照交给持久化任务。NVS 不可用时当前会话
 * 仍生效，但返回非 ESP_OK，且重启后可能恢复旧值。
 */
esp_err_t settings_store_set_theme(int theme);
esp_err_t settings_store_set_sound(bool enabled);

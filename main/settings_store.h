#pragma once

#include <stdbool.h>

#include "esp_err.h"

/*
 * 持久化用户设置（NVS）。主题与策略枚举值和 MoonBit 模型保持一致，
 * C 层只负责存取，不复制产品规则。
 */
enum {
    SETTINGS_THEME_CYBER = 0,
    SETTINGS_THEME_SKY = 1,
};

enum {
    SETTINGS_POLICY_COMPATIBLE = 0,
    SETTINGS_POLICY_STANDARD = 1,
    SETTINGS_POLICY_STRICT = 2,
};

/*
 * 初始化 NVS、载入设置并启动持久化任务。可重复调用；不会为了恢复
 * NVS 错误擦除整分区。
 */
esp_err_t settings_store_init(void);

int settings_store_theme(void);
bool settings_store_sound_enabled(void);
int settings_store_policy_profile(void);
bool settings_store_exclude_ambiguous(void);

/*
 * 立即更新内存值并把最新快照交给持久化任务。NVS 不可用时当前会话
 * 仍生效，但返回非 ESP_OK，且重启后可能恢复旧值。
 */
esp_err_t settings_store_set_theme(int theme);
esp_err_t settings_store_set_sound(bool enabled);
esp_err_t settings_store_set_policy_profile(int profile);
esp_err_t settings_store_set_exclude_ambiguous(bool enabled);

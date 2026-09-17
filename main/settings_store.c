#include "settings_store.h"

#include <stdint.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"

#define SETTINGS_NAMESPACE "passport"
#define SETTINGS_QUEUE_DEPTH 1
#define SETTINGS_TASK_STACK 3072

static const char *TAG = "settings_store";

typedef struct {
    uint8_t theme;
    uint8_t sound;
    uint8_t policy_profile;
    uint8_t exclude_ambiguous;
} settings_snapshot_t;

static nvs_handle_t s_handle;
static QueueHandle_t s_queue;
static TaskHandle_t s_task;
static bool s_ready;
static int s_theme = SETTINGS_THEME_CYBER;
static bool s_sound_enabled = true;
static int s_policy_profile = SETTINGS_POLICY_STANDARD;
static bool s_exclude_ambiguous = true;

static void persist_task(void *argument)
{
    (void)argument;
    settings_snapshot_t snapshot;
    for (;;) {
        if (xQueueReceive(s_queue, &snapshot, portMAX_DELAY) != pdTRUE) continue;

        esp_err_t err = nvs_set_u8(s_handle, "theme", snapshot.theme);
        if (err == ESP_OK) err = nvs_set_u8(s_handle, "sound", snapshot.sound);
        if (err == ESP_OK) err = nvs_set_u8(s_handle, "policy", snapshot.policy_profile);
        if (err == ESP_OK) err = nvs_set_u8(s_handle, "clear", snapshot.exclude_ambiguous);
        if (err == ESP_OK) err = nvs_commit(s_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to persist settings: %s", esp_err_to_name(err));
        }
    }
}

esp_err_t settings_store_init(void)
{
    if (s_ready) return ESP_OK;

    esp_err_t err = nvs_flash_init();
    if (err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "NVS init failed: %s; partition was not erased",
            esp_err_to_name(err)
        );
        return err;
    }

    err = nvs_open(SETTINGS_NAMESPACE, NVS_READWRITE, &s_handle);
    if (err != ESP_OK) return err;

    uint8_t theme = (uint8_t)s_theme;
    if (nvs_get_u8(s_handle, "theme", &theme) == ESP_OK) {
        s_theme = theme == SETTINGS_THEME_SKY ? SETTINGS_THEME_SKY : SETTINGS_THEME_CYBER;
    }

    uint8_t sound = s_sound_enabled ? 1 : 0;
    if (nvs_get_u8(s_handle, "sound", &sound) == ESP_OK) {
        s_sound_enabled = sound != 0;
    }

    uint8_t policy_profile = (uint8_t)s_policy_profile;
    if (nvs_get_u8(s_handle, "policy", &policy_profile) == ESP_OK) {
        s_policy_profile = policy_profile <= SETTINGS_POLICY_STRICT
            ? policy_profile : SETTINGS_POLICY_STANDARD;
    }

    uint8_t exclude_ambiguous = s_exclude_ambiguous ? 1 : 0;
    if (nvs_get_u8(s_handle, "clear", &exclude_ambiguous) == ESP_OK) {
        s_exclude_ambiguous = exclude_ambiguous != 0;
    }

    s_queue = xQueueCreate(SETTINGS_QUEUE_DEPTH, sizeof(settings_snapshot_t));
    if (!s_queue) {
        nvs_close(s_handle);
        s_handle = 0;
        return ESP_ERR_NO_MEM;
    }
    if (xTaskCreate(
            persist_task,
            "settings_store",
            SETTINGS_TASK_STACK,
            NULL,
            4,
            &s_task
        ) != pdPASS) {
        vQueueDelete(s_queue);
        s_queue = NULL;
        nvs_close(s_handle);
        s_handle = 0;
        return ESP_ERR_NO_MEM;
    }

    s_ready = true;
    ESP_LOGI(
        TAG,
        "Loaded theme=%d sound=%d policy=%d clear=%d",
        s_theme,
        (int)s_sound_enabled,
        s_policy_profile,
        (int)s_exclude_ambiguous
    );
    return ESP_OK;
}

int settings_store_theme(void)
{
    return s_theme;
}

bool settings_store_sound_enabled(void)
{
    return s_sound_enabled;
}

int settings_store_policy_profile(void)
{
    return s_policy_profile;
}

bool settings_store_exclude_ambiguous(void)
{
    return s_exclude_ambiguous;
}

static esp_err_t queue_current_snapshot(void)
{
    if (!s_ready || !s_queue) return ESP_ERR_INVALID_STATE;
    settings_snapshot_t snapshot = {
        .theme = (uint8_t)s_theme,
        .sound = (uint8_t)(s_sound_enabled ? 1 : 0),
        .policy_profile = (uint8_t)s_policy_profile,
        .exclude_ambiguous = (uint8_t)(s_exclude_ambiguous ? 1 : 0),
    };
    return xQueueOverwrite(s_queue, &snapshot) == pdPASS
        ? ESP_OK : ESP_FAIL;
}

esp_err_t settings_store_set_theme(int theme)
{
    int normalized = theme == SETTINGS_THEME_SKY ? SETTINGS_THEME_SKY : SETTINGS_THEME_CYBER;
    s_theme = normalized;
    return queue_current_snapshot();
}

esp_err_t settings_store_set_sound(bool enabled)
{
    s_sound_enabled = enabled;
    return queue_current_snapshot();
}

esp_err_t settings_store_set_policy_profile(int profile)
{
    s_policy_profile = profile >= SETTINGS_POLICY_COMPATIBLE &&
        profile <= SETTINGS_POLICY_STRICT ? profile : SETTINGS_POLICY_STANDARD;
    return queue_current_snapshot();
}

esp_err_t settings_store_set_exclude_ambiguous(bool enabled)
{
    s_exclude_ambiguous = enabled;
    return queue_current_snapshot();
}

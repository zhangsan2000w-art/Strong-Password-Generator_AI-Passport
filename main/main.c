#include <stdbool.h>

#include "bsp_battery.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "bsp_i2c.h"
#include "bsp_pins.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "moonbit_password.h"
#include "password_app.h"
#include "password_platform.h"
#include "password_sound.h"
#include "settings_store.h"

#define INPUT_QUEUE_DEPTH 8

static const char *TAG = "password_main";

typedef struct {
    bsp_btn_t button;
    bsp_btn_ev_t event;
} input_event_t;

static QueueHandle_t s_input_queue;
static TaskHandle_t s_input_task;
static volatile bool s_input_ready;

static void input_task(void *argument)
{
    (void)argument;
    input_event_t input;
    for (;;) {
        if (xQueueReceive(s_input_queue, &input, portMAX_DELAY) == pdTRUE) {
            password_app_handle_button(input.button, input.event);
        }
    }
}

static esp_err_t input_dispatch_init(void)
{
    s_input_queue = xQueueCreate(INPUT_QUEUE_DEPTH, sizeof(input_event_t));
    if (!s_input_queue) return ESP_ERR_NO_MEM;
    if (xTaskCreate(input_task, "password_input", 4096, NULL, 5, &s_input_task) != pdPASS) {
        vQueueDelete(s_input_queue);
        s_input_queue = NULL;
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

/* BSP button callbacks run on the shared esp_timer task. Queue only. */
static void on_key(bsp_btn_t button, bsp_btn_ev_t event, void *user)
{
    (void)user;
    if (!s_input_ready || !s_input_queue) return;
    const input_event_t input = {.button = button, .event = event};
    (void)xQueueSend(s_input_queue, &input, 0);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting Strong Password Generator_AI Passport");

    /* Seed the DRBG before bsp_button_init() claims ADC1. */
    esp_err_t random_error = password_platform_init();
    if (random_error != ESP_OK) {
        ESP_LOGE(TAG, "Secure random initialization failed: %s", esp_err_to_name(random_error));
    }

    if (bsp_i2c_init() != ESP_OK) {
        ESP_LOGW(TAG, "I2C initialization failed; battery status may be unavailable");
    }
    esp_err_t battery_error = bsp_battery_init();
    if (battery_error != ESP_OK) {
        ESP_LOGW(TAG, "Battery gauge unavailable: %s", esp_err_to_name(battery_error));
    }

    esp_err_t settings_error = settings_store_init();
    if (settings_error != ESP_OK) {
        ESP_LOGW(
            TAG,
            "Settings persistence unavailable: %s",
            esp_err_to_name(settings_error)
        );
    }

    if (bsp_display_init() != ESP_OK || !bsp_lvgl_init()) {
        ESP_LOGE(
            TAG,
            "Display initialization failed (MOSI=%d SCLK=%d CS=%d DC=%d BL=%d)",
            BSP_LCD_MOSI, BSP_LCD_SCLK, BSP_LCD_CS, BSP_LCD_DC, BSP_LCD_BL
        );
        return;
    }
    bsp_display_backlight(100);

    moonbit_init();
    esp_err_t input_error = input_dispatch_init();
    esp_err_t button_error = input_error == ESP_OK
        ? bsp_button_init(on_key, NULL)
        : ESP_ERR_INVALID_STATE;
    if (input_error != ESP_OK || button_error != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Button input initialization failed: queue=%s button=%s",
            esp_err_to_name(input_error), esp_err_to_name(button_error)
        );
    }

    if (bsp_lvgl_lock(1000)) {
        password_app_enter();
        bsp_lvgl_unlock();
        s_input_ready = input_error == ESP_OK && button_error == ESP_OK;
    }

    esp_err_t sound_error = password_sound_init();
    if (sound_error != ESP_OK) {
        ESP_LOGW(TAG, "Success sound unavailable: %s", esp_err_to_name(sound_error));
    }

    ESP_LOGI(
        TAG,
        "Ready: secure_random=%d buttons=%d",
        password_platform_ready(), s_input_ready
    );
}

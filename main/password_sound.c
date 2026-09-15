#include "password_sound.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bsp_audio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "moonbit_password.h"

enum {
    SOUND_CHUNK_SAMPLES = 160,
    SOUND_TASK_STACK = 4096,
};

static TaskHandle_t s_sound_task;

static bool play_note(int note)
{
    int16_t samples[SOUND_CHUNK_SAMPLES];
    int total = passport_moonbit_sound_note_samples(note);
    int cursor = 0;
    if (total <= 0) return false;

    while (cursor < total) {
        int count = total - cursor < SOUND_CHUNK_SAMPLES
            ? total - cursor : SOUND_CHUNK_SAMPLES;
        for (int i = 0; i < count; i++) {
            samples[i] = (int16_t)passport_moonbit_sound_sample(note, cursor + i);
        }
        if (bsp_audio_write(samples, (size_t)count * sizeof(samples[0])) != ESP_OK) return false;
        cursor += count;
    }
    return true;
}

static void sound_task(void *argument)
{
    (void)argument;
    bool ready = false;
    for (;;) {
        (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if (!ready) {
            int sample_rate = passport_moonbit_sound_sample_rate();
            ready = bsp_audio_init() == ESP_OK &&
                    bsp_audio_set_format((uint32_t)sample_rate, 16, 1) == ESP_OK;
            if (ready) bsp_audio_set_volume(passport_moonbit_sound_volume());
        }
        for (int note = 0; ready && note < passport_moonbit_sound_note_count(); note++) {
            if (!play_note(note)) ready = false;
        }
    }
}

esp_err_t password_sound_init(void)
{
    if (s_sound_task != NULL) return ESP_OK;
    return xTaskCreate(sound_task, "password_sound", SOUND_TASK_STACK, NULL, 3,
                       &s_sound_task) == pdPASS ? ESP_OK : ESP_ERR_NO_MEM;
}

void password_sound_play_success(void)
{
    if (s_sound_task != NULL) xTaskNotifyGive(s_sound_task);
}

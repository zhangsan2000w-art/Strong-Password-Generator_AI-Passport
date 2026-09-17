#include "password_platform.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include "bootloader_random.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/platform_util.h"

#define PASSWORD_OUTPUT_CAPACITY 128

static mbedtls_ctr_drbg_context s_drbg;
static SemaphoreHandle_t s_drbg_mutex;
static bool s_ready;
static bool s_generation_failed;
static char s_output[PASSWORD_OUTPUT_CAPACITY];
static size_t s_output_length;
static char s_diagnostic_output[PASSWORD_OUTPUT_CAPACITY];
static size_t s_diagnostic_output_length;
static bool s_diagnostic_mode;

static void clear_buffer(char *output, size_t *length)
{
    mbedtls_platform_zeroize(output, PASSWORD_OUTPUT_CAPACITY);
    *length = 0;
}

static char *active_output(void)
{
    return s_diagnostic_mode ? s_diagnostic_output : s_output;
}

static size_t *active_output_length(void)
{
    return s_diagnostic_mode ? &s_diagnostic_output_length : &s_output_length;
}

static int hardware_entropy(void *context, unsigned char *output, size_t length)
{
    (void)context;
    esp_fill_random(output, length);
    return 0;
}

esp_err_t password_platform_init(void)
{
    static const unsigned char personalization[] = "Strong Password Generator_AI Passport";
    if (s_ready) return ESP_OK;

    s_drbg_mutex = xSemaphoreCreateMutex();
    if (!s_drbg_mutex) return ESP_ERR_NO_MEM;

    mbedtls_ctr_drbg_init(&s_drbg);
    bootloader_random_enable();
    int result = mbedtls_ctr_drbg_seed(
        &s_drbg,
        hardware_entropy,
        NULL,
        personalization,
        sizeof(personalization) - 1
    );
    bootloader_random_disable();
    if (result != 0) {
        mbedtls_ctr_drbg_free(&s_drbg);
        vSemaphoreDelete(s_drbg_mutex);
        s_drbg_mutex = NULL;
        return ESP_FAIL;
    }

    /* Automatic reseeding would call the ADC-backed entropy callback after the
       button driver owns ADC1. A seeded CTR-DRBG safely supports this device's
       lifetime request volume without doing that. */
    mbedtls_ctr_drbg_set_reseed_interval(&s_drbg, INT_MAX);
    s_ready = true;
    return ESP_OK;
}

bool password_platform_ready(void)
{
    return s_ready;
}

const char *password_platform_output(void)
{
    return s_output;
}

void password_platform_clear_output(void)
{
    clear_buffer(s_output, &s_output_length);
}

void password_platform_begin_diagnostics(void)
{
    clear_buffer(s_diagnostic_output, &s_diagnostic_output_length);
    s_diagnostic_mode = true;
}

const char *password_platform_diagnostic_output(void)
{
    return s_diagnostic_output;
}

void password_platform_end_diagnostics(void)
{
    clear_buffer(s_diagnostic_output, &s_diagnostic_output_length);
    s_diagnostic_mode = false;
}

uint32_t passport_random_u32(void)
{
    uint32_t value = 0;
    if (!s_ready || !s_drbg_mutex ||
        xSemaphoreTake(s_drbg_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        s_generation_failed = true;
        return 0;
    }
    if (mbedtls_ctr_drbg_random(&s_drbg, (unsigned char *)&value, sizeof(value)) != 0) {
        s_generation_failed = true;
        value = 0;
    }
    xSemaphoreGive(s_drbg_mutex);
    return value;
}

void passport_output_reset(void)
{
    clear_buffer(active_output(), active_output_length());
    s_generation_failed = !s_ready;
}

int32_t passport_output_push(int32_t character)
{
    char *output = active_output();
    size_t *length = active_output_length();
    if (s_generation_failed || character < 0x20 || character > 0x7e ||
        *length + 1 >= PASSWORD_OUTPUT_CAPACITY) {
        s_generation_failed = true;
        clear_buffer(output, length);
        return 0;
    }
    output[(*length)++] = (char)character;
    output[*length] = '\0';
    return 1;
}

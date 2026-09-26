// components/bsp/src/bsp_battery.c
// CW2017 电量计驱动。
#include "bsp_battery.h"
#include "bsp_i2c.h"
#include "bsp_pins.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdbool.h>

static const char *TAG = "bsp_batt";

#define CW_REG_VERSION 0x00 // 版本号，上电应答即代表芯片在位
#define CW_REG_VCELL_H 0x02 // 14bit 电压，V(uV) = raw * 312.5
#define CW_REG_SOC_H   0x04 // 高字节 = 整数百分比；低字节 = 1/256 %
#define CW_REG_CONFIG    0x08 // 0xF0=睡眠 / 0x30=复位态 / 0x00=正常
#define CW_REG_SOC_ALERT 0x0B // bit7=profile UPDATE_FLAG
#define CW_REG_PROFILE   0x10 // 80 字节电池 profile 起始地址

#define CW_CONFIG_ACTIVE  0x00
#define CW_CONFIG_RESTART 0x30
#define CW_CONFIG_SLEEP   0xF0
#define CW_UPDATE_FLAG    0x80
#define CW_PROFILE_SIZE   80

/* 厂家为本机 520mAh 电芯生成的 CW2017 profile。 */
static const uint8_t s_battery_profile[CW_PROFILE_SIZE] = {
    0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xB7, 0xC5, 0xC6, 0xC8, 0xBF, 0xB3, 0xAD, 0x81,
    0x69, 0xAE, 0x94, 0x72, 0x5C, 0x49, 0x41, 0x32,
    0x2A, 0x24, 0x1D, 0x4E, 0x20, 0xDE, 0x37, 0xBA,
    0xBB, 0xC3, 0xCA, 0xCF, 0xD1, 0xD2, 0xCF, 0xCE,
    0xCF, 0xD5, 0xC6, 0xB3, 0xA4, 0x9A, 0x93, 0x8E,
    0x8E, 0x91, 0x93, 0x9E, 0xA6, 0x86, 0x80, 0xF4,
    0x00, 0x00, 0xAB, 0x02, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEA,
};

_Static_assert(
    sizeof(s_battery_profile) == CW_PROFILE_SIZE,
    "CW2017 battery profile must contain exactly 80 bytes"
);

static i2c_master_dev_handle_t s_dev;

static void cw_remove_device(void)
{
    if (s_dev != NULL) {
        (void)i2c_master_bus_rm_device(s_dev);
        s_dev = NULL;
    }
}

static int cw_read(uint8_t reg, uint8_t *buf, size_t n)
{
    if (!s_dev) return -1;
    return i2c_master_transmit_receive(s_dev, &reg, 1, buf, n, 100) == ESP_OK ? 0 : -1;
}

static int cw_write(uint8_t reg, uint8_t val)
{
    if (!s_dev) return -1;
    uint8_t bytes[2] = {reg, val};
    return i2c_master_transmit(s_dev, bytes, 2, 100) == ESP_OK ? 0 : -1;
}

static int cw_restart(uint8_t final_mode)
{
    if (cw_write(CW_REG_CONFIG, CW_CONFIG_RESTART) != 0) return -1;
    vTaskDelay(pdMS_TO_TICKS(20));
    if (cw_write(CW_REG_CONFIG, final_mode) != 0) return -1;
    vTaskDelay(pdMS_TO_TICKS(10));
    return 0;
}

static int cw_profile_matches(bool *matches)
{
    uint8_t value = 0;
    *matches = false;
    if (cw_read(CW_REG_SOC_ALERT, &value, 1) != 0) return -1;
    if ((value & CW_UPDATE_FLAG) == 0) return 0;

    for (size_t index = 0; index < CW_PROFILE_SIZE; index++) {
        if (cw_read((uint8_t)(CW_REG_PROFILE + index), &value, 1) != 0) {
            return -1;
        }
        if (value != s_battery_profile[index]) return 0;
    }
    *matches = true;
    return 0;
}

static int cw_install_profile(void)
{
    uint8_t value = 0;
    if (cw_restart(CW_CONFIG_SLEEP) != 0) return -1;

    for (size_t index = 0; index < CW_PROFILE_SIZE; index++) {
        if (cw_write(
                (uint8_t)(CW_REG_PROFILE + index),
                s_battery_profile[index]
            ) != 0) {
            ESP_LOGE(TAG, "写入电池 profile 失败: index=%u", (unsigned)index);
            return -1;
        }
    }

    for (size_t index = 0; index < CW_PROFILE_SIZE; index++) {
        if (cw_read(
                (uint8_t)(CW_REG_PROFILE + index), &value, 1
            ) != 0 || value != s_battery_profile[index]) {
            ESP_LOGE(TAG, "校验电池 profile 失败: index=%u", (unsigned)index);
            return -1;
        }
    }

    if (cw_read(CW_REG_SOC_ALERT, &value, 1) != 0) return -1;
    if (cw_write(CW_REG_SOC_ALERT, value | CW_UPDATE_FLAG) != 0) return -1;
    return cw_restart(CW_CONFIG_ACTIVE);
}

static int cw_wait_soc_ready(void)
{
    for (int retry = 0; retry < 50; retry++) {
        uint8_t soc = 0;
        vTaskDelay(pdMS_TO_TICKS(100));
        if (cw_read(CW_REG_SOC_H, &soc, 1) == 0 && soc <= 100) return 0;
    }
    return -1;
}

esp_err_t bsp_battery_init(void)
{
    if (s_dev) {
        uint8_t version = 0;
        if (cw_read(CW_REG_VERSION, &version, 1) == 0) return ESP_OK;
        ESP_LOGW(TAG, "CW2017 已失去响应，重新挂载 I2C 设备");
        cw_remove_device();
    }

    esp_err_t error = bsp_i2c_init();
    if (error != ESP_OK) return error;

    i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = BSP_I2C_CW2017_ADDR,
        .scl_speed_hz = 100000,
    };
    error = i2c_master_bus_add_device(bsp_i2c_bus(), &device_config, &s_dev);
    if (error != ESP_OK) {
        ESP_LOGE(TAG, "添加 I2C 设备失败: %s", esp_err_to_name(error));
        return error;
    }

    uint8_t version = 0;
    if (cw_read(CW_REG_VERSION, &version, 1) != 0) {
        ESP_LOGW(TAG, "CW2017 未应答；请确认 I2C 地址 0x%02X", BSP_I2C_CW2017_ADDR);
        cw_remove_device();
        return ESP_ERR_NOT_FOUND;
    }
    ESP_LOGI(TAG, "检测到 CW2017 VERSION=0x%02X", version);

    bool profile_matches = false;
    if (cw_profile_matches(&profile_matches) != 0) {
        ESP_LOGE(TAG, "读取电池 profile 失败");
        error = ESP_FAIL;
        goto fail;
    }

    if (!profile_matches) {
        ESP_LOGI(TAG, "安装 520mAh 电芯 profile");
        if (cw_install_profile() != 0) {
            error = ESP_FAIL;
            goto fail;
        }
    } else {
        uint8_t config = 0;
        if (cw_read(CW_REG_CONFIG, &config, 1) != 0) {
            error = ESP_FAIL;
            goto fail;
        }
        if (config != CW_CONFIG_ACTIVE &&
            cw_restart(CW_CONFIG_ACTIVE) != 0) {
            error = ESP_FAIL;
            goto fail;
        }
        ESP_LOGI(TAG, "520mAh 电芯 profile 已匹配");
    }

    if (cw_wait_soc_ready() != 0) {
        ESP_LOGE(TAG, "等待 CW2017 SOC 就绪超时");
        error = ESP_ERR_TIMEOUT;
        goto fail;
    }
    return ESP_OK;

fail:
    cw_remove_device();
    return error;
}

int bsp_battery_soc(void)
{
    uint8_t bytes[2] = {0};
    if (cw_read(CW_REG_SOC_H, bytes, 2) != 0) return -1;
    int soc = bytes[0];
    return soc <= 100 ? soc : -1;
}

int bsp_battery_mv(void)
{
    uint8_t bytes[2] = {0};
    if (cw_read(CW_REG_VCELL_H, bytes, 2) != 0) return -1;
    uint32_t raw = ((uint32_t)bytes[0] << 8 | bytes[1]) & 0x3FFF;
    return (int)((raw * 3125) / 10000);
}

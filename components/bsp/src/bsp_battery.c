// components/bsp/src/bsp_battery.c
// CW2017 电量计驱动。
// 本项目沿用已在同一台 AI Passport 真机验证过的芯片内置 Li-Poly profile。
#include "bsp_battery.h"
#include "bsp_i2c.h"
#include "bsp_pins.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "bsp_batt";

#define CW_REG_VERSION 0x00 // 版本号，上电应答即代表芯片在位
#define CW_REG_VCELL_H 0x02 // 14bit 电压，V(uV) = raw * 312.5
#define CW_REG_SOC_H   0x04 // 高字节 = 整数百分比；低字节 = 1/256 %
#define CW_REG_CONFIG  0x08 // 0xF0=睡眠 / 0x30=复位态 / 0x00=正常

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

    /*
     * 用户的同一台 AI Passport 已验证芯片内置 Li-Poly profile 可正确显示电量。
     * 只退出睡眠态，不在每次 MCU 启动时写 0x30 复位电量计；CW2017 的电量跟踪
     * 必须跨应用重启连续保留。反复复位会造成冷启动 0% 和随后跳到固定值。
     */
    if (cw_write(CW_REG_CONFIG, 0x00) != 0) {
        cw_remove_device();
        return ESP_FAIL;
    }
    vTaskDelay(pdMS_TO_TICKS(100));
    return ESP_OK;
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

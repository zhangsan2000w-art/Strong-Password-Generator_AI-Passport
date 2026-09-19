#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

typedef enum {
    PASSWORD_BLE_STARTING = 0,
    PASSWORD_BLE_ADVERTISING = 1,
    PASSWORD_BLE_PAIRING = 2,
    PASSWORD_BLE_CONNECTED = 3,
    PASSWORD_BLE_SENDING = 4,
    PASSWORD_BLE_SENT = 5,
    PASSWORD_BLE_ERROR = 6,
} password_ble_status_t;

esp_err_t password_ble_keyboard_init(void);
esp_err_t password_ble_keyboard_send(const char *password);
password_ble_status_t password_ble_keyboard_status(void);
uint32_t password_ble_keyboard_passkey(void);
void password_ble_keyboard_reset_feedback(void);

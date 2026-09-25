#include "password_ble_keyboard.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "esp_hid_common.h"
#include "esp_hidd.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "host/ble_gap.h"
#include "host/ble_hs.h"
#include "host/ble_store.h"
#include "host/util/util.h"
#include "mbedtls/platform_util.h"
#include "moonbit_password.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "nvs_flash.h"

#define PASSWORD_BLE_DEVICE_NAME "FoloPassKey"
#define PASSWORD_BLE_OUTPUT_CAPACITY 128
#define PASSWORD_BLE_REPORT_ID 1
#define PASSWORD_BLE_KEY_DOWN_MS 12
#define PASSWORD_BLE_KEY_UP_MS 8

static const char *TAG = "password_ble";

typedef struct {
    char value[PASSWORD_BLE_OUTPUT_CAPACITY];
} password_ble_request_t;

/* Standard boot-style keyboard report with Report ID 1. */
static const uint8_t s_keyboard_report_map[] = {
    0x05, 0x01, 0x09, 0x06, 0xa1, 0x01, 0x85, PASSWORD_BLE_REPORT_ID,
    0x05, 0x07, 0x19, 0xe0, 0x29, 0xe7, 0x15, 0x00, 0x25, 0x01,
    0x75, 0x01, 0x95, 0x08, 0x81, 0x02, 0x95, 0x01, 0x75, 0x08,
    0x81, 0x03, 0x95, 0x05, 0x75, 0x01, 0x05, 0x08, 0x19, 0x01,
    0x29, 0x05, 0x91, 0x02, 0x95, 0x01, 0x75, 0x03, 0x91, 0x03,
    0x95, 0x06, 0x75, 0x08, 0x15, 0x00, 0x25, 0x65, 0x05, 0x07,
    0x19, 0x00, 0x29, 0x65, 0x81, 0x00, 0xc0,
};

static esp_hid_raw_report_map_t s_report_maps[] = {
    {.data = s_keyboard_report_map, .len = sizeof(s_keyboard_report_map)},
};

static const esp_hid_device_config_t s_hid_config = {
    .vendor_id = 0x303a,
    .product_id = 0x4001,
    .version = 0x0100,
    .device_name = PASSWORD_BLE_DEVICE_NAME,
    .manufacturer_name = "FoloToy",
    .serial_number = "AI-Passport",
    .report_maps = s_report_maps,
    .report_maps_len = 1,
};

static ble_uuid16_t s_hid_service_uuid = BLE_UUID16_INIT(0x1812);
static struct ble_gap_event_listener s_gap_listener;
static esp_hidd_dev_t *s_hid_device;
static QueueHandle_t s_send_queue;
static TaskHandle_t s_send_task;
static portMUX_TYPE s_state_lock = portMUX_INITIALIZER_UNLOCKED;
static volatile password_ble_status_t s_status = PASSWORD_BLE_STARTING;
static volatile bool s_encrypted;
static volatile bool s_subscribed;
static uint8_t s_address_type;

void ble_store_config_init(void);

static void set_status(password_ble_status_t status)
{
    portENTER_CRITICAL(&s_state_lock);
    s_status = status;
    portEXIT_CRITICAL(&s_state_lock);
}

password_ble_status_t password_ble_keyboard_status(void)
{
    password_ble_status_t status;
    portENTER_CRITICAL(&s_state_lock);
    status = s_status;
    portEXIT_CRITICAL(&s_state_lock);
    return status;
}

static bool transport_ready(void)
{
    bool ready;
    portENTER_CRITICAL(&s_state_lock);
    ready = s_encrypted && s_subscribed;
    portEXIT_CRITICAL(&s_state_lock);
    return ready && s_hid_device && esp_hidd_dev_connected(s_hid_device);
}

static void update_connection_status(void)
{
    if (transport_ready()) set_status(PASSWORD_BLE_CONNECTED);
}

static int start_advertising(void)
{
    struct ble_hs_adv_fields fields = {0};
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.appearance = ESP_HID_APPEARANCE_KEYBOARD;
    fields.appearance_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    fields.tx_pwr_lvl_is_present = 1;
    fields.name = (const uint8_t *)PASSWORD_BLE_DEVICE_NAME;
    fields.name_len = strlen(PASSWORD_BLE_DEVICE_NAME);
    fields.name_is_complete = 1;
    fields.uuids16 = &s_hid_service_uuid;
    fields.num_uuids16 = 1;
    fields.uuids16_is_complete = 1;

    int rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) return rc;

    struct ble_gap_adv_params params = {0};
    params.conn_mode = BLE_GAP_CONN_MODE_UND;
    params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    params.itvl_min = BLE_GAP_ADV_ITVL_MS(30);
    params.itvl_max = BLE_GAP_ADV_ITVL_MS(50);
    rc = ble_gap_adv_start(
        s_address_type, NULL, BLE_HS_FOREVER, &params, NULL, NULL
    );
    if (rc == 0) set_status(PASSWORD_BLE_ADVERTISING);
    return rc;
}

static int gap_event(struct ble_gap_event *event, void *argument)
{
    (void)argument;
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            portENTER_CRITICAL(&s_state_lock);
            s_encrypted = false;
            s_subscribed = false;
            portEXIT_CRITICAL(&s_state_lock);
            set_status(PASSWORD_BLE_PAIRING);
        }
        break;
    case BLE_GAP_EVENT_DISCONNECT:
        portENTER_CRITICAL(&s_state_lock);
        s_encrypted = false;
        s_subscribed = false;
        portEXIT_CRITICAL(&s_state_lock);
        set_status(PASSWORD_BLE_ADVERTISING);
        break;
    case BLE_GAP_EVENT_SUBSCRIBE:
        portENTER_CRITICAL(&s_state_lock);
        s_subscribed = event->subscribe.cur_notify != 0;
        portEXIT_CRITICAL(&s_state_lock);
        if (event->subscribe.cur_notify != 0) {
            update_connection_status();
        } else {
            set_status(PASSWORD_BLE_PAIRING);
        }
        break;
    case BLE_GAP_EVENT_ENC_CHANGE:
        portENTER_CRITICAL(&s_state_lock);
        s_encrypted = event->enc_change.status == 0;
        portEXIT_CRITICAL(&s_state_lock);
        if (event->enc_change.status == 0) {
            update_connection_status();
        } else {
            set_status(PASSWORD_BLE_PAIRING);
        }
        break;
    case BLE_GAP_EVENT_REPEAT_PAIRING: {
        struct ble_gap_conn_desc description;
        if (ble_gap_conn_find(
                event->repeat_pairing.conn_handle, &description
            ) == 0) {
            ble_store_util_delete_peer(&description.peer_id_addr);
            return BLE_GAP_REPEAT_PAIRING_RETRY;
        }
        break;
    }
    default:
        break;
    }
    return 0;
}

static void hid_event(
    void *handler_argument,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data
)
{
    (void)handler_argument;
    (void)event_base;
    (void)event_data;
    switch ((esp_hidd_event_t)event_id) {
    case ESP_HIDD_START_EVENT:
        if (ble_hs_util_ensure_addr(0) != 0 ||
            ble_hs_id_infer_auto(0, &s_address_type) != 0 ||
            start_advertising() != 0) {
            set_status(PASSWORD_BLE_ERROR);
        }
        break;
    case ESP_HIDD_CONNECT_EVENT:
        if (password_ble_keyboard_status() != PASSWORD_BLE_CONNECTED) {
            set_status(PASSWORD_BLE_PAIRING);
        }
        break;
    case ESP_HIDD_DISCONNECT_EVENT:
        if (start_advertising() != 0) set_status(PASSWORD_BLE_ERROR);
        break;
    default:
        break;
    }
}

static void host_task(void *argument)
{
    (void)argument;
    nimble_port_run();
    nimble_port_freertos_deinit();
}

static esp_err_t send_report(const uint8_t report[8])
{
    return esp_hidd_dev_input_set(
        s_hid_device, 0, PASSWORD_BLE_REPORT_ID, (uint8_t *)report, 8
    );
}

static esp_err_t type_password(const char *password)
{
    uint8_t report[8] = {0};
    esp_err_t result = ESP_OK;
    for (size_t index = 0; password[index] != '\0'; index++) {
        if (!transport_ready()) {
            result = ESP_ERR_INVALID_STATE;
            break;
        }
        int32_t mapped = passport_moonbit_ble_keyboard_report(
            (uint8_t)password[index]
        );
        if (mapped < 0) {
            result = ESP_ERR_INVALID_ARG;
            break;
        }
        report[0] = (uint8_t)((mapped >> 8) & 0xff);
        report[2] = (uint8_t)(mapped & 0xff);
        result = send_report(report);
        if (result != ESP_OK) break;
        vTaskDelay(pdMS_TO_TICKS(PASSWORD_BLE_KEY_DOWN_MS));
        mbedtls_platform_zeroize(report, sizeof(report));
        result = send_report(report);
        if (result != ESP_OK) break;
        vTaskDelay(pdMS_TO_TICKS(PASSWORD_BLE_KEY_UP_MS));
    }
    mbedtls_platform_zeroize(report, sizeof(report));
    return result;
}

static void send_task(void *argument)
{
    (void)argument;
    password_ble_request_t request;
    for (;;) {
        if (xQueueReceive(s_send_queue, &request, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        esp_err_t result = type_password(request.value);
        mbedtls_platform_zeroize(&request, sizeof(request));
        if (result == ESP_OK && transport_ready()) {
            set_status(PASSWORD_BLE_SENT);
        } else if (transport_ready()) {
            set_status(PASSWORD_BLE_ERROR);
        } else if (s_hid_device && esp_hidd_dev_connected(s_hid_device)) {
            set_status(PASSWORD_BLE_PAIRING);
        }
    }
}

esp_err_t password_ble_keyboard_init(void)
{
    if (s_send_queue || s_hid_device) return ESP_ERR_INVALID_STATE;

    esp_err_t error = nvs_flash_init();
    if (error != ESP_OK) return error;

    s_send_queue = xQueueCreate(1, sizeof(password_ble_request_t));
    if (!s_send_queue) return ESP_ERR_NO_MEM;
    if (xTaskCreate(
            send_task, "password_ble_send", 4096, NULL, 5, &s_send_task
        ) != pdPASS) {
        vQueueDelete(s_send_queue);
        s_send_queue = NULL;
        return ESP_ERR_NO_MEM;
    }

    error = nimble_port_init();
    if (error != ESP_OK) goto fail;

    ble_hs_cfg.sm_io_cap = BLE_HS_IO_NO_INPUT_OUTPUT;
    ble_hs_cfg.sm_bonding = 1;
    ble_hs_cfg.sm_mitm = 0;
    ble_hs_cfg.sm_sc = 1;
    ble_hs_cfg.sm_our_key_dist = BLE_SM_PAIR_KEY_DIST_ID |
        BLE_SM_PAIR_KEY_DIST_ENC;
    ble_hs_cfg.sm_their_key_dist = BLE_SM_PAIR_KEY_DIST_ID |
        BLE_SM_PAIR_KEY_DIST_ENC;
    ble_store_config_init();
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    error = esp_hidd_dev_init(
        &s_hid_config, ESP_HID_TRANSPORT_BLE, hid_event, &s_hid_device
    );
    if (error != ESP_OK) goto fail_nimble;

    if (ble_gap_event_listener_register(&s_gap_listener, gap_event, NULL) != 0) {
        error = ESP_FAIL;
        goto fail_hid;
    }

    set_status(PASSWORD_BLE_STARTING);
    nimble_port_freertos_init(host_task);
    return ESP_OK;

fail_hid:
    esp_hidd_dev_deinit(s_hid_device);
    s_hid_device = NULL;
fail_nimble:
    nimble_port_deinit();
fail:
    if (s_send_task) {
        vTaskDelete(s_send_task);
        s_send_task = NULL;
    }
    if (s_send_queue) {
        vQueueDelete(s_send_queue);
        s_send_queue = NULL;
    }
    set_status(PASSWORD_BLE_ERROR);
    ESP_LOGE(TAG, "BLE keyboard initialization failed: %s", esp_err_to_name(error));
    return error;
}

esp_err_t password_ble_keyboard_send(const char *password)
{
    if (!password || !s_send_queue || !transport_ready()) {
        return ESP_ERR_INVALID_STATE;
    }
    size_t length = strnlen(password, PASSWORD_BLE_OUTPUT_CAPACITY);
    if (length == 0 || length >= PASSWORD_BLE_OUTPUT_CAPACITY) {
        return ESP_ERR_INVALID_SIZE;
    }

    password_ble_request_t request = {0};
    memcpy(request.value, password, length);
    BaseType_t queued = xQueueSend(s_send_queue, &request, 0);
    mbedtls_platform_zeroize(&request, sizeof(request));
    if (queued != pdTRUE) return ESP_ERR_INVALID_STATE;
    set_status(PASSWORD_BLE_SENDING);
    return ESP_OK;
}

void password_ble_keyboard_reset_feedback(void)
{
    password_ble_status_t status = password_ble_keyboard_status();
    if ((status == PASSWORD_BLE_SENT || status == PASSWORD_BLE_ERROR) &&
        transport_ready()) {
        set_status(PASSWORD_BLE_CONNECTED);
    }
}

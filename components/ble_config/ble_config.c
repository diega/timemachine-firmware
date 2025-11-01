/**
 * @file ble_config.c
 * @brief BLE configuration component implementation
 */

#include "ble_config.h"
#include "timemachine_events.h"
#include "network.h"
#include "clock_panel.h"
#include "ntp_sync.h"
#include "i18n.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_common_api.h"
#include "esp_bt_device.h"
#include <string.h>

static const char *TAG = "ble_config";

#define GATTS_SERVICE_UUID_NETWORK     0x00FF
#define GATTS_CHAR_UUID_WIFI_SSID      0xFF01
#define GATTS_CHAR_UUID_WIFI_PASSWORD  0xFF02
#define GATTS_CHAR_UUID_WIFI_AUTHMODE  0xFF03

#define GATTS_SERVICE_UUID_CLOCK       0x01FF
#define GATTS_CHAR_UUID_TIME_FORMAT    0xFF11
#define GATTS_CHAR_UUID_SHOW_SECONDS   0xFF12

#define GATTS_SERVICE_UUID_NTP         0x02FF
#define GATTS_CHAR_UUID_TIMEZONE       0xFF21
#define GATTS_CHAR_UUID_NTP_SERVER1    0xFF22
#define GATTS_CHAR_UUID_NTP_SERVER2    0xFF23
#define GATTS_CHAR_UUID_SYNC_INTERVAL  0xFF24

#define GATTS_SERVICE_UUID_LANGUAGE    0x03FF
#define GATTS_CHAR_UUID_LANGUAGE       0xFF31

#define GATTS_NUM_HANDLE_NETWORK       8
#define GATTS_NUM_HANDLE_CLOCK         6
#define GATTS_NUM_HANDLE_NTP           10
#define GATTS_NUM_HANDLE_LANGUAGE      4

#define DEVICE_NAME                    "TimeMachine"
#define GATTS_TAG                      "GATTS_CONFIG"

// Service handles
enum {
    IDX_SVC_NETWORK,
    IDX_CHAR_WIFI_SSID,
    IDX_CHAR_VAL_WIFI_SSID,
    IDX_CHAR_WIFI_PASSWORD,
    IDX_CHAR_VAL_WIFI_PASSWORD,
    IDX_CHAR_WIFI_AUTHMODE,
    IDX_CHAR_VAL_WIFI_AUTHMODE,
    HRS_NETWORK_IDX_NB,
};

enum {
    IDX_SVC_CLOCK,
    IDX_CHAR_TIME_FORMAT,
    IDX_CHAR_VAL_TIME_FORMAT,
    IDX_CHAR_SHOW_SECONDS,
    IDX_CHAR_VAL_SHOW_SECONDS,
    HRS_CLOCK_IDX_NB,
};

enum {
    IDX_SVC_NTP,
    IDX_CHAR_TIMEZONE,
    IDX_CHAR_VAL_TIMEZONE,
    IDX_CHAR_NTP_SERVER1,
    IDX_CHAR_VAL_NTP_SERVER1,
    IDX_CHAR_NTP_SERVER2,
    IDX_CHAR_VAL_NTP_SERVER2,
    IDX_CHAR_SYNC_INTERVAL,
    IDX_CHAR_VAL_SYNC_INTERVAL,
    HRS_NTP_IDX_NB,
};

enum {
    IDX_SVC_LANGUAGE,
    IDX_CHAR_LANGUAGE,
    IDX_CHAR_VAL_LANGUAGE,
    HRS_LANGUAGE_IDX_NB,
};

static bool s_connected = false;
static bool s_initialized = false;
static uint16_t s_gatts_if = ESP_GATT_IF_NONE;
static uint16_t s_conn_id = 0;

// Service handle tables
static uint16_t s_network_handle_table[HRS_NETWORK_IDX_NB];
static uint16_t s_clock_handle_table[HRS_CLOCK_IDX_NB];
static uint16_t s_ntp_handle_table[HRS_NTP_IDX_NB];
static uint16_t s_language_handle_table[HRS_LANGUAGE_IDX_NB];

// Configuration buffers
static char s_wifi_ssid[32] = {0};
static char s_wifi_password[64] = {0};
static uint8_t s_wifi_authmode = 0;
static uint8_t s_time_format = 0;
static uint8_t s_show_seconds = 0;
static char s_timezone[64] = {0};
static char s_ntp_server1[64] = {0};
static char s_ntp_server2[64] = {0};
static uint32_t s_sync_interval = 0;
static uint8_t s_language = 0;

// Forward declarations
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

// Advertising data
static uint8_t s_adv_service_uuid128[16] = {
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
    0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};

static esp_ble_adv_data_t s_adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x0006,
    .max_interval = 0x0010,
    .appearance = 0x00,
    .manufacturer_len = 0,
    .p_manufacturer_data = NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(s_adv_service_uuid128),
    .p_service_uuid = s_adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_params_t s_adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

// GATT profile
static struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
} s_profile = {
    .gatts_cb = gatts_profile_event_handler,
    .gatts_if = ESP_GATT_IF_NONE,
};

// ============================================================================
// Public API
// ============================================================================

esp_err_t ble_config_init(void)
{
    if (s_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing BLE configuration...");

    // Initialize NVS (required for BLE)
    // Already initialized in main, but just in case

    // Release classic BT memory
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    // Initialize BT controller
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(TAG, "Failed to initialize BT controller: %s", esp_err_to_name(ret));
        return ret;
    }

    // Enable BT controller in BLE mode
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(TAG, "Failed to enable BT controller: %s", esp_err_to_name(ret));
        return ret;
    }

    // Initialize Bluedroid
    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(TAG, "Failed to initialize Bluedroid: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(TAG, "Failed to enable Bluedroid: %s", esp_err_to_name(ret));
        return ret;
    }

    // Register callbacks
    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret) {
        ESP_LOGE(TAG, "Failed to register GATTS callback: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret) {
        ESP_LOGE(TAG, "Failed to register GAP callback: %s", esp_err_to_name(ret));
        return ret;
    }

    // Register application profile
    ret = esp_ble_gatts_app_register(0);
    if (ret) {
        ESP_LOGE(TAG, "Failed to register app: %s", esp_err_to_name(ret));
        return ret;
    }

    // Set MTU
    ret = esp_ble_gatt_set_local_mtu(500);
    if (ret) {
        ESP_LOGE(TAG, "Failed to set MTU: %s", esp_err_to_name(ret));
    }

    s_initialized = true;
    ESP_LOGI(TAG, "BLE configuration initialized");

    return ESP_OK;
}

void ble_config_deinit(void)
{
    if (!s_initialized) {
        return;
    }

    ESP_LOGI(TAG, "Deinitializing BLE configuration...");

    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();

    s_initialized = false;
    s_connected = false;

    ESP_LOGI(TAG, "BLE configuration deinitialized");
}

bool ble_config_is_connected(void)
{
    return s_connected;
}

// ============================================================================
// Private - GAP Event Handler
// ============================================================================

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        esp_ble_gap_start_advertising(&s_adv_params);
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "Advertising start failed");
        } else {
            ESP_LOGI(TAG, "Advertising started");
        }
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "Advertising stop failed");
        } else {
            ESP_LOGI(TAG, "Advertising stopped");
        }
        break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
        ESP_LOGI(TAG, "Connection params updated");
        break;
    default:
        break;
    }
}

// ============================================================================
// Private - GATTS Event Handlers
// ============================================================================

static void handle_write_event(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    uint16_t handle = param->write.handle;

    // Network service characteristics
    if (handle == s_network_handle_table[IDX_CHAR_VAL_WIFI_SSID]) {
        memset(s_wifi_ssid, 0, sizeof(s_wifi_ssid));
        memcpy(s_wifi_ssid, param->write.value,
               param->write.len < sizeof(s_wifi_ssid) ? param->write.len : sizeof(s_wifi_ssid) - 1);
        ESP_LOGI(TAG, "WiFi SSID updated: %s", s_wifi_ssid);

    } else if (handle == s_network_handle_table[IDX_CHAR_VAL_WIFI_PASSWORD]) {
        memset(s_wifi_password, 0, sizeof(s_wifi_password));
        memcpy(s_wifi_password, param->write.value,
               param->write.len < sizeof(s_wifi_password) ? param->write.len : sizeof(s_wifi_password) - 1);
        ESP_LOGI(TAG, "WiFi password updated");

    } else if (handle == s_network_handle_table[IDX_CHAR_VAL_WIFI_AUTHMODE]) {
        s_wifi_authmode = param->write.value[0];
        ESP_LOGI(TAG, "WiFi authmode updated: %d", s_wifi_authmode);

        // Emit NETWORK_CONFIG_CHANGED event
        network_config_t config = {
            .wifi_ssid = s_wifi_ssid,
            .wifi_password = s_wifi_password,
            .wifi_authmode = s_wifi_authmode,
            .max_retries = 5
        };
        esp_event_post(TIMEMACHINE_EVENT, NETWORK_CONFIG_CHANGED,
                      &config, sizeof(config), portMAX_DELAY);
    }

    // Clock service characteristics
    else if (handle == s_clock_handle_table[IDX_CHAR_VAL_TIME_FORMAT]) {
        s_time_format = param->write.value[0];
        ESP_LOGI(TAG, "Time format updated: %d", s_time_format);

    } else if (handle == s_clock_handle_table[IDX_CHAR_VAL_SHOW_SECONDS]) {
        s_show_seconds = param->write.value[0];
        ESP_LOGI(TAG, "Show seconds updated: %d", s_show_seconds);

        // Emit CLOCK_CONFIG_CHANGED event
        clock_config_t config = {
            .format = (time_format_t)s_time_format,
            .show_seconds = s_show_seconds != 0
        };
        esp_event_post(TIMEMACHINE_EVENT, CLOCK_CONFIG_CHANGED,
                      &config, sizeof(config), portMAX_DELAY);
        ESP_LOGI(TAG, "Clock config changed event posted");
    }

    // NTP service characteristics
    else if (handle == s_ntp_handle_table[IDX_CHAR_VAL_TIMEZONE]) {
        memset(s_timezone, 0, sizeof(s_timezone));
        memcpy(s_timezone, param->write.value,
               param->write.len < sizeof(s_timezone) ? param->write.len : sizeof(s_timezone) - 1);
        ESP_LOGI(TAG, "Timezone updated: %s", s_timezone);

    } else if (handle == s_ntp_handle_table[IDX_CHAR_VAL_NTP_SERVER1]) {
        memset(s_ntp_server1, 0, sizeof(s_ntp_server1));
        memcpy(s_ntp_server1, param->write.value,
               param->write.len < sizeof(s_ntp_server1) ? param->write.len : sizeof(s_ntp_server1) - 1);
        ESP_LOGI(TAG, "NTP server1 updated: %s", s_ntp_server1);

    } else if (handle == s_ntp_handle_table[IDX_CHAR_VAL_NTP_SERVER2]) {
        memset(s_ntp_server2, 0, sizeof(s_ntp_server2));
        memcpy(s_ntp_server2, param->write.value,
               param->write.len < sizeof(s_ntp_server2) ? param->write.len : sizeof(s_ntp_server2) - 1);
        ESP_LOGI(TAG, "NTP server2 updated: %s", s_ntp_server2);

    } else if (handle == s_ntp_handle_table[IDX_CHAR_VAL_SYNC_INTERVAL]) {
        memcpy(&s_sync_interval, param->write.value, sizeof(uint32_t));
        ESP_LOGI(TAG, "Sync interval updated: %lu", s_sync_interval);

        // Emit NTP_CONFIG_CHANGED event
        ntp_sync_config_t config = {
            .timezone = s_timezone,
            .server1 = s_ntp_server1,
            .server2 = s_ntp_server2,
            .sync_interval_ms = s_sync_interval
        };
        esp_event_post(TIMEMACHINE_EVENT, NTP_CONFIG_CHANGED,
                      &config, sizeof(config), portMAX_DELAY);
        ESP_LOGI(TAG, "NTP config changed event posted");
    }

    // Language service characteristic
    else if (handle == s_language_handle_table[IDX_CHAR_VAL_LANGUAGE]) {
        s_language = param->write.value[0];
        ESP_LOGI(TAG, "Language updated: %d", s_language);

        // Emit LANGUAGE_CHANGED event
        language_t lang = (language_t)s_language;
        esp_event_post(TIMEMACHINE_EVENT, LANGUAGE_CHANGED,
                      &lang, sizeof(lang), portMAX_DELAY);
        ESP_LOGI(TAG, "Language changed event posted");
    }

    // Send response if needed
    if (param->write.need_rsp) {
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id,
                                   param->write.trans_id, ESP_GATT_OK, NULL);
    }
}

static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(TAG, "GATT server registered, app_id=%d", param->reg.app_id);

        s_gatts_if = gatts_if;

        // Set device name
        esp_ble_gap_set_device_name(DEVICE_NAME);

        // Configure advertising
        esp_ble_gap_config_adv_data(&s_adv_data);

        // Create services
        esp_ble_gatts_create_service(gatts_if,
            &(esp_gatt_srvc_id_t){
                .is_primary = true,
                .id = {
                    .uuid = {.len = ESP_UUID_LEN_16, .uuid = {.uuid16 = GATTS_SERVICE_UUID_NETWORK}},
                    .inst_id = 0,
                }
            },
            GATTS_NUM_HANDLE_NETWORK);
        break;

    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(TAG, "Service created: handle=%d", param->create.service_handle);

        // Store handles and start services based on UUID
        if (param->create.service_id.id.uuid.uuid.uuid16 == GATTS_SERVICE_UUID_NETWORK) {
            s_network_handle_table[IDX_SVC_NETWORK] = param->create.service_handle;
            esp_ble_gatts_start_service(s_network_handle_table[IDX_SVC_NETWORK]);

            // Add WIFI_SSID characteristic
            esp_ble_gatts_add_char(s_network_handle_table[IDX_SVC_NETWORK],
                &(esp_bt_uuid_t){.len = ESP_UUID_LEN_16, .uuid = {.uuid16 = GATTS_CHAR_UUID_WIFI_SSID}},
                ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE,
                NULL, NULL);

        } else if (param->create.service_id.id.uuid.uuid.uuid16 == GATTS_SERVICE_UUID_CLOCK) {
            s_clock_handle_table[IDX_SVC_CLOCK] = param->create.service_handle;
            esp_ble_gatts_start_service(s_clock_handle_table[IDX_SVC_CLOCK]);

            esp_ble_gatts_add_char(s_clock_handle_table[IDX_SVC_CLOCK],
                &(esp_bt_uuid_t){.len = ESP_UUID_LEN_16, .uuid = {.uuid16 = GATTS_CHAR_UUID_TIME_FORMAT}},
                ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE,
                NULL, NULL);

        } else if (param->create.service_id.id.uuid.uuid.uuid16 == GATTS_SERVICE_UUID_NTP) {
            s_ntp_handle_table[IDX_SVC_NTP] = param->create.service_handle;
            esp_ble_gatts_start_service(s_ntp_handle_table[IDX_SVC_NTP]);

            esp_ble_gatts_add_char(s_ntp_handle_table[IDX_SVC_NTP],
                &(esp_bt_uuid_t){.len = ESP_UUID_LEN_16, .uuid = {.uuid16 = GATTS_CHAR_UUID_TIMEZONE}},
                ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE,
                NULL, NULL);

        } else if (param->create.service_id.id.uuid.uuid.uuid16 == GATTS_SERVICE_UUID_LANGUAGE) {
            s_language_handle_table[IDX_SVC_LANGUAGE] = param->create.service_handle;
            esp_ble_gatts_start_service(s_language_handle_table[IDX_SVC_LANGUAGE]);

            esp_ble_gatts_add_char(s_language_handle_table[IDX_SVC_LANGUAGE],
                &(esp_bt_uuid_t){.len = ESP_UUID_LEN_16, .uuid = {.uuid16 = GATTS_CHAR_UUID_LANGUAGE}},
                ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE,
                NULL, NULL);
        }
        break;

    case ESP_GATTS_ADD_CHAR_EVT: {
        ESP_LOGI(TAG, "Characteristic added: handle=%d, uuid=0x%x",
                 param->add_char.attr_handle, param->add_char.char_uuid.uuid.uuid16);

        // Map characteristic handles
        uint16_t char_uuid = param->add_char.char_uuid.uuid.uuid16;

        // Network characteristics
        if (char_uuid == GATTS_CHAR_UUID_WIFI_SSID) {
            s_network_handle_table[IDX_CHAR_WIFI_SSID] = param->add_char.attr_handle;
            s_network_handle_table[IDX_CHAR_VAL_WIFI_SSID] = param->add_char.attr_handle;

            // Add next characteristic
            esp_ble_gatts_add_char(s_network_handle_table[IDX_SVC_NETWORK],
                &(esp_bt_uuid_t){.len = ESP_UUID_LEN_16, .uuid = {.uuid16 = GATTS_CHAR_UUID_WIFI_PASSWORD}},
                ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE,
                NULL, NULL);

        } else if (char_uuid == GATTS_CHAR_UUID_WIFI_PASSWORD) {
            s_network_handle_table[IDX_CHAR_WIFI_PASSWORD] = param->add_char.attr_handle;
            s_network_handle_table[IDX_CHAR_VAL_WIFI_PASSWORD] = param->add_char.attr_handle;

            esp_ble_gatts_add_char(s_network_handle_table[IDX_SVC_NETWORK],
                &(esp_bt_uuid_t){.len = ESP_UUID_LEN_16, .uuid = {.uuid16 = GATTS_CHAR_UUID_WIFI_AUTHMODE}},
                ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE,
                NULL, NULL);

        } else if (char_uuid == GATTS_CHAR_UUID_WIFI_AUTHMODE) {
            s_network_handle_table[IDX_CHAR_WIFI_AUTHMODE] = param->add_char.attr_handle;
            s_network_handle_table[IDX_CHAR_VAL_WIFI_AUTHMODE] = param->add_char.attr_handle;

            // Network service complete, create clock service
            esp_ble_gatts_create_service(gatts_if,
                &(esp_gatt_srvc_id_t){
                    .is_primary = true,
                    .id = {
                        .uuid = {.len = ESP_UUID_LEN_16, .uuid = {.uuid16 = GATTS_SERVICE_UUID_CLOCK}},
                        .inst_id = 0,
                    }
                },
                GATTS_NUM_HANDLE_CLOCK);
        }

        // Clock characteristics
        else if (char_uuid == GATTS_CHAR_UUID_TIME_FORMAT) {
            s_clock_handle_table[IDX_CHAR_TIME_FORMAT] = param->add_char.attr_handle;
            s_clock_handle_table[IDX_CHAR_VAL_TIME_FORMAT] = param->add_char.attr_handle;

            esp_ble_gatts_add_char(s_clock_handle_table[IDX_SVC_CLOCK],
                &(esp_bt_uuid_t){.len = ESP_UUID_LEN_16, .uuid = {.uuid16 = GATTS_CHAR_UUID_SHOW_SECONDS}},
                ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE,
                NULL, NULL);

        } else if (char_uuid == GATTS_CHAR_UUID_SHOW_SECONDS) {
            s_clock_handle_table[IDX_CHAR_SHOW_SECONDS] = param->add_char.attr_handle;
            s_clock_handle_table[IDX_CHAR_VAL_SHOW_SECONDS] = param->add_char.attr_handle;

            // Clock service complete, create NTP service
            esp_ble_gatts_create_service(gatts_if,
                &(esp_gatt_srvc_id_t){
                    .is_primary = true,
                    .id = {
                        .uuid = {.len = ESP_UUID_LEN_16, .uuid = {.uuid16 = GATTS_SERVICE_UUID_NTP}},
                        .inst_id = 0,
                    }
                },
                GATTS_NUM_HANDLE_NTP);
        }

        // NTP characteristics
        else if (char_uuid == GATTS_CHAR_UUID_TIMEZONE) {
            s_ntp_handle_table[IDX_CHAR_TIMEZONE] = param->add_char.attr_handle;
            s_ntp_handle_table[IDX_CHAR_VAL_TIMEZONE] = param->add_char.attr_handle;

            esp_ble_gatts_add_char(s_ntp_handle_table[IDX_SVC_NTP],
                &(esp_bt_uuid_t){.len = ESP_UUID_LEN_16, .uuid = {.uuid16 = GATTS_CHAR_UUID_NTP_SERVER1}},
                ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE,
                NULL, NULL);

        } else if (char_uuid == GATTS_CHAR_UUID_NTP_SERVER1) {
            s_ntp_handle_table[IDX_CHAR_NTP_SERVER1] = param->add_char.attr_handle;
            s_ntp_handle_table[IDX_CHAR_VAL_NTP_SERVER1] = param->add_char.attr_handle;

            esp_ble_gatts_add_char(s_ntp_handle_table[IDX_SVC_NTP],
                &(esp_bt_uuid_t){.len = ESP_UUID_LEN_16, .uuid = {.uuid16 = GATTS_CHAR_UUID_NTP_SERVER2}},
                ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE,
                NULL, NULL);

        } else if (char_uuid == GATTS_CHAR_UUID_NTP_SERVER2) {
            s_ntp_handle_table[IDX_CHAR_NTP_SERVER2] = param->add_char.attr_handle;
            s_ntp_handle_table[IDX_CHAR_VAL_NTP_SERVER2] = param->add_char.attr_handle;

            esp_ble_gatts_add_char(s_ntp_handle_table[IDX_SVC_NTP],
                &(esp_bt_uuid_t){.len = ESP_UUID_LEN_16, .uuid = {.uuid16 = GATTS_CHAR_UUID_SYNC_INTERVAL}},
                ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE,
                NULL, NULL);

        } else if (char_uuid == GATTS_CHAR_UUID_SYNC_INTERVAL) {
            s_ntp_handle_table[IDX_CHAR_SYNC_INTERVAL] = param->add_char.attr_handle;
            s_ntp_handle_table[IDX_CHAR_VAL_SYNC_INTERVAL] = param->add_char.attr_handle;

            // NTP service complete, create language service
            esp_ble_gatts_create_service(gatts_if,
                &(esp_gatt_srvc_id_t){
                    .is_primary = true,
                    .id = {
                        .uuid = {.len = ESP_UUID_LEN_16, .uuid = {.uuid16 = GATTS_SERVICE_UUID_LANGUAGE}},
                        .inst_id = 0,
                    }
                },
                GATTS_NUM_HANDLE_LANGUAGE);
        }

        // Language characteristic
        else if (char_uuid == GATTS_CHAR_UUID_LANGUAGE) {
            s_language_handle_table[IDX_CHAR_LANGUAGE] = param->add_char.attr_handle;
            s_language_handle_table[IDX_CHAR_VAL_LANGUAGE] = param->add_char.attr_handle;

            ESP_LOGI(TAG, "All services and characteristics created");
        }
        break;
    }

    case ESP_GATTS_CONNECT_EVT: {
        ESP_LOGI(TAG, "Client connected: conn_id=%d", param->connect.conn_id);
        s_connected = true;
        s_conn_id = param->connect.conn_id;
        s_gatts_if = gatts_if;

        // Update connection parameters for more stable connection
        esp_ble_conn_update_params_t conn_params = {0};
        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        conn_params.min_int = 0x10;    // 20ms (in 1.25ms units)
        conn_params.max_int = 0x20;    // 40ms (in 1.25ms units)
        conn_params.latency = 0;       // No slave latency
        conn_params.timeout = 400;     // 4000ms supervision timeout (in 10ms units)
        esp_ble_gap_update_conn_params(&conn_params);
        break;
    }

    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(TAG, "Client disconnected");
        s_connected = false;
        esp_ble_gap_start_advertising(&s_adv_params);
        break;

    case ESP_GATTS_WRITE_EVT:
        handle_write_event(gatts_if, param);
        break;

    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(TAG, "MTU exchanged: %d", param->mtu.mtu);
        break;

    default:
        break;
    }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            s_profile.gatts_if = gatts_if;
        } else {
            ESP_LOGE(TAG, "Registration failed: app_id=%d, status=%d",
                     param->reg.app_id, param->reg.status);
            return;
        }
    }

    if (gatts_if == ESP_GATT_IF_NONE || gatts_if == s_profile.gatts_if) {
        if (s_profile.gatts_cb) {
            s_profile.gatts_cb(event, gatts_if, param);
        }
    }
}

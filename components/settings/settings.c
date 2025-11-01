/**
 * @file settings.c
 * @brief Settings component implementation
 */

#include "settings.h"
#include "timemachine_events.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "sdkconfig.h"
#include <string.h>

static const char *TAG = "settings";

#define NVS_NAMESPACE "settings"

// NVS keys
#define KEY_WIFI_SSID      "wifi_ssid"
#define KEY_WIFI_PASS      "wifi_pass"
#define KEY_WIFI_AUTH      "wifi_auth"
#define KEY_WIFI_RETRIES   "wifi_retries"
#define KEY_TIME_FORMAT    "time_format"
#define KEY_SHOW_SECONDS   "show_seconds"
#define KEY_TIMEZONE       "timezone"
#define KEY_NTP_SERVER1    "ntp_server1"
#define KEY_NTP_SERVER2    "ntp_server2"
#define KEY_NTP_INTERVAL   "ntp_interval"
#define KEY_LANGUAGE       "language"
#define KEY_BRIGHTNESS     "brightness"

#define DEFAULT_BRIGHTNESS 8  // Medium brightness

static bool s_initialized = false;
static nvs_handle_t s_nvs_handle;

// Event handler instances
static esp_event_handler_instance_t s_network_config_handler = NULL;
static esp_event_handler_instance_t s_clock_config_handler = NULL;
static esp_event_handler_instance_t s_ntp_config_handler = NULL;
static esp_event_handler_instance_t s_language_handler = NULL;
static esp_event_handler_instance_t s_brightness_handler = NULL;

// Forward declarations
static void on_network_config_changed(void* arg, esp_event_base_t base,
                                      int32_t event_id, void* event_data);
static void on_clock_config_changed(void* arg, esp_event_base_t base,
                                    int32_t event_id, void* event_data);
static void on_ntp_config_changed(void* arg, esp_event_base_t base,
                                  int32_t event_id, void* event_data);
static void on_language_changed(void* arg, esp_event_base_t base,
                               int32_t event_id, void* event_data);
static void on_brightness_changed(void* arg, esp_event_base_t base,
                                  int32_t event_id, void* event_data);

// ============================================================================
// Public API
// ============================================================================

esp_err_t settings_init(void)
{
    if (s_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing settings...");

    // Open NVS namespace
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &s_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace: %s", esp_err_to_name(err));
        return err;
    }

    // Register event handlers for configuration changes
    err = esp_event_handler_instance_register(
        TIMEMACHINE_EVENT,
        NETWORK_CONFIG_CHANGED,
        on_network_config_changed,
        NULL,
        &s_network_config_handler
    );
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register NETWORK_CONFIG_CHANGED handler");
        nvs_close(s_nvs_handle);
        return err;
    }

    err = esp_event_handler_instance_register(
        TIMEMACHINE_EVENT,
        CLOCK_CONFIG_CHANGED,
        on_clock_config_changed,
        NULL,
        &s_clock_config_handler
    );
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register CLOCK_CONFIG_CHANGED handler");
        settings_deinit();
        return err;
    }

    err = esp_event_handler_instance_register(
        TIMEMACHINE_EVENT,
        NTP_CONFIG_CHANGED,
        on_ntp_config_changed,
        NULL,
        &s_ntp_config_handler
    );
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register NTP_CONFIG_CHANGED handler");
        settings_deinit();
        return err;
    }

    err = esp_event_handler_instance_register(
        TIMEMACHINE_EVENT,
        LANGUAGE_CHANGED,
        on_language_changed,
        NULL,
        &s_language_handler
    );
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register LANGUAGE_CHANGED handler");
        settings_deinit();
        return err;
    }

    err = esp_event_handler_instance_register(
        TIMEMACHINE_EVENT,
        BRIGHTNESS_CHANGED,
        on_brightness_changed,
        NULL,
        &s_brightness_handler
    );
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register BRIGHTNESS_CHANGED handler");
        settings_deinit();
        return err;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "Settings initialized");

    return ESP_OK;
}

network_config_t settings_get_network(void)
{
    static char wifi_ssid[32] = {0};
    static char wifi_password[64] = {0};
    network_config_t config;

    size_t ssid_len = sizeof(wifi_ssid);
    size_t pass_len = sizeof(wifi_password);
    uint8_t wifi_auth = 0;
    uint8_t wifi_retries = 0;

    esp_err_t err = nvs_get_str(s_nvs_handle, KEY_WIFI_SSID, wifi_ssid, &ssid_len);
    if (err == ESP_OK) {
        nvs_get_str(s_nvs_handle, KEY_WIFI_PASS, wifi_password, &pass_len);
        nvs_get_u8(s_nvs_handle, KEY_WIFI_AUTH, &wifi_auth);
        nvs_get_u8(s_nvs_handle, KEY_WIFI_RETRIES, &wifi_retries);

        ESP_LOGI(TAG, "Loaded network config from NVS: SSID=%s", wifi_ssid);
    } else {
        // Use Kconfig defaults
        strlcpy(wifi_ssid, CONFIG_TIMEMACHINE_WIFI_SSID, sizeof(wifi_ssid));
        strlcpy(wifi_password, CONFIG_TIMEMACHINE_WIFI_PASSWORD, sizeof(wifi_password));
        wifi_auth = CONFIG_TIMEMACHINE_WIFI_AUTHMODE;
        wifi_retries = 5;

        ESP_LOGI(TAG, "Using default network config: SSID=%s", wifi_ssid);
    }

    config.wifi_ssid = wifi_ssid;
    config.wifi_password = wifi_password;
    config.wifi_authmode = wifi_auth;
    config.max_retries = wifi_retries;

    return config;
}

clock_config_t settings_get_clock(void)
{
    clock_config_t config;
    uint8_t time_format = 0;
    uint8_t show_seconds = 0;

    esp_err_t err = nvs_get_u8(s_nvs_handle, KEY_TIME_FORMAT, &time_format);
    if (err == ESP_OK) {
        nvs_get_u8(s_nvs_handle, KEY_SHOW_SECONDS, &show_seconds);

        ESP_LOGI(TAG, "Loaded clock config from NVS: format=%s",
                 time_format == TIME_FORMAT_24H ? "24h" : "12h");
    } else {
        // Use Kconfig defaults
        time_format = CONFIG_TIMEMACHINE_TIME_FORMAT;
        show_seconds = CONFIG_TIMEMACHINE_SHOW_SECONDS ? 1 : 0;

        ESP_LOGI(TAG, "Using default clock config: format=%s",
                 time_format == TIME_FORMAT_24H ? "24h" : "12h");
    }

    config.format = (time_format_t)time_format;
    config.show_seconds = show_seconds != 0;

    return config;
}

ntp_sync_config_t settings_get_ntp(void)
{
    static char timezone[64] = {0};
    static char ntp_server1[64] = {0};
    static char ntp_server2[64] = {0};
    ntp_sync_config_t config;

    size_t tz_len = sizeof(timezone);
    size_t srv1_len = sizeof(ntp_server1);
    size_t srv2_len = sizeof(ntp_server2);
    uint32_t ntp_interval = 0;

    esp_err_t err = nvs_get_str(s_nvs_handle, KEY_TIMEZONE, timezone, &tz_len);
    if (err == ESP_OK) {
        nvs_get_str(s_nvs_handle, KEY_NTP_SERVER1, ntp_server1, &srv1_len);
        nvs_get_str(s_nvs_handle, KEY_NTP_SERVER2, ntp_server2, &srv2_len);
        nvs_get_u32(s_nvs_handle, KEY_NTP_INTERVAL, &ntp_interval);

        ESP_LOGI(TAG, "Loaded NTP config from NVS: TZ=%s", timezone);
    } else {
        // Use Kconfig defaults
        strlcpy(timezone, CONFIG_TIMEMACHINE_TIMEZONE, sizeof(timezone));
        strlcpy(ntp_server1, CONFIG_TIMEMACHINE_NTP_SERVER1, sizeof(ntp_server1));
        strlcpy(ntp_server2, CONFIG_TIMEMACHINE_NTP_SERVER2, sizeof(ntp_server2));
        ntp_interval = 3600000;

        ESP_LOGI(TAG, "Using default NTP config: TZ=%s", timezone);
    }

    config.server1 = ntp_server1;
    config.server2 = ntp_server2;
    config.timezone = timezone;
    config.sync_interval_ms = ntp_interval;

    return config;
}

language_t settings_get_language(void)
{
    uint8_t language = 0;

    esp_err_t err = nvs_get_u8(s_nvs_handle, KEY_LANGUAGE, &language);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Loaded language from NVS: %d", language);
    } else {
        // Use default (English)
        language = LANG_EN;
        ESP_LOGI(TAG, "Using default language: EN");
    }

    return (language_t)language;
}

uint8_t settings_get_brightness(void)
{
    uint8_t brightness = 0;

    esp_err_t err = nvs_get_u8(s_nvs_handle, KEY_BRIGHTNESS, &brightness);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Loaded brightness from NVS: %d", brightness);
    } else {
        // Use default
        brightness = DEFAULT_BRIGHTNESS;
        ESP_LOGI(TAG, "Using default brightness: %d", DEFAULT_BRIGHTNESS);
    }

    return brightness;
}

void settings_deinit(void)
{
    if (!s_initialized) {
        return;
    }

    ESP_LOGI(TAG, "Deinitializing settings...");

    // Unregister event handlers
    if (s_brightness_handler != NULL) {
        esp_event_handler_instance_unregister(
            TIMEMACHINE_EVENT,
            BRIGHTNESS_CHANGED,
            s_brightness_handler
        );
        s_brightness_handler = NULL;
    }

    if (s_language_handler != NULL) {
        esp_event_handler_instance_unregister(
            TIMEMACHINE_EVENT,
            LANGUAGE_CHANGED,
            s_language_handler
        );
        s_language_handler = NULL;
    }

    if (s_ntp_config_handler != NULL) {
        esp_event_handler_instance_unregister(
            TIMEMACHINE_EVENT,
            NTP_CONFIG_CHANGED,
            s_ntp_config_handler
        );
        s_ntp_config_handler = NULL;
    }

    if (s_clock_config_handler != NULL) {
        esp_event_handler_instance_unregister(
            TIMEMACHINE_EVENT,
            CLOCK_CONFIG_CHANGED,
            s_clock_config_handler
        );
        s_clock_config_handler = NULL;
    }

    if (s_network_config_handler != NULL) {
        esp_event_handler_instance_unregister(
            TIMEMACHINE_EVENT,
            NETWORK_CONFIG_CHANGED,
            s_network_config_handler
        );
        s_network_config_handler = NULL;
    }

    // Close NVS
    nvs_close(s_nvs_handle);

    s_initialized = false;
    ESP_LOGI(TAG, "Settings deinitialized");
}

// ============================================================================
// Private - Event Handlers (persist changes to NVS)
// ============================================================================

static void on_network_config_changed(void* arg, esp_event_base_t base,
                                      int32_t event_id, void* event_data)
{
    network_config_t *config = (network_config_t*)event_data;

    ESP_LOGI(TAG, "Saving network config to NVS...");

    nvs_set_str(s_nvs_handle, KEY_WIFI_SSID, config->wifi_ssid);
    nvs_set_str(s_nvs_handle, KEY_WIFI_PASS, config->wifi_password);
    nvs_set_u8(s_nvs_handle, KEY_WIFI_AUTH, config->wifi_authmode);
    nvs_set_u8(s_nvs_handle, KEY_WIFI_RETRIES, config->max_retries);

    nvs_commit(s_nvs_handle);
    ESP_LOGI(TAG, "Network config saved");
}

static void on_clock_config_changed(void* arg, esp_event_base_t base,
                                    int32_t event_id, void* event_data)
{
    clock_config_t *config = (clock_config_t*)event_data;

    ESP_LOGI(TAG, "Saving clock config to NVS...");

    nvs_set_u8(s_nvs_handle, KEY_TIME_FORMAT, (uint8_t)config->format);
    nvs_set_u8(s_nvs_handle, KEY_SHOW_SECONDS, config->show_seconds ? 1 : 0);

    nvs_commit(s_nvs_handle);
    ESP_LOGI(TAG, "Clock config saved");
}

static void on_ntp_config_changed(void* arg, esp_event_base_t base,
                                  int32_t event_id, void* event_data)
{
    ntp_sync_config_t *config = (ntp_sync_config_t*)event_data;

    ESP_LOGI(TAG, "Saving NTP config to NVS...");

    nvs_set_str(s_nvs_handle, KEY_TIMEZONE, config->timezone);
    nvs_set_str(s_nvs_handle, KEY_NTP_SERVER1, config->server1);
    nvs_set_str(s_nvs_handle, KEY_NTP_SERVER2, config->server2);
    nvs_set_u32(s_nvs_handle, KEY_NTP_INTERVAL, config->sync_interval_ms);

    nvs_commit(s_nvs_handle);
    ESP_LOGI(TAG, "NTP config saved");
}

static void on_language_changed(void* arg, esp_event_base_t base,
                               int32_t event_id, void* event_data)
{
    language_t *lang = (language_t*)event_data;

    ESP_LOGI(TAG, "Saving language to NVS...");

    nvs_set_u8(s_nvs_handle, KEY_LANGUAGE, (uint8_t)*lang);

    nvs_commit(s_nvs_handle);
    ESP_LOGI(TAG, "Language saved");
}

static void on_brightness_changed(void* arg, esp_event_base_t base,
                                  int32_t event_id, void* event_data)
{
    uint8_t *brightness = (uint8_t*)event_data;

    ESP_LOGI(TAG, "Saving brightness to NVS: %d", *brightness);

    nvs_set_u8(s_nvs_handle, KEY_BRIGHTNESS, *brightness);

    nvs_commit(s_nvs_handle);
    ESP_LOGI(TAG, "Brightness saved");
}

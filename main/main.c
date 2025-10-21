#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "timemachine_events.h"
#include "network.h"
#include "ntp_sync.h"
#include "display.h"
#include "panel_manager.h"
#include "touch_sensor.h"
#include "clock.h"
#include "tick_task.h"

static const char *TAG = "timemachine";

// Forward declarations
static void on_network_connected(void* arg, esp_event_base_t event_base,
                                  int32_t event_id, void* event_data);
static void on_network_failed(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data);
static void on_ntp_synced(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data);

// ============================================================================
// Main Entry Point
// ============================================================================

void app_main(void)
{
    ESP_LOGI(TAG, "Time Machine starting...");

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS initialized");

    // Create default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        TIMEMACHINE_EVENT,
        NETWORK_CONNECTED,
        &on_network_connected,
        NULL,
        NULL
    ));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        TIMEMACHINE_EVENT,
        NETWORK_FAILED,
        &on_network_failed,
        NULL,
        NULL
    ));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        TIMEMACHINE_EVENT,
        NTP_SYNCED,
        &on_ntp_synced,
        NULL,
        NULL
    ));

    // Initialize display
    ESP_ERROR_CHECK(display_init());

    // Initialize panel manager
    panel_manager_config_t panel_config = {
        .default_panel = PANEL_CLOCK,
        .inactivity_timeout_s = CONFIG_TIMEMACHINE_PANEL_TIMEOUT_S
    };
    ESP_ERROR_CHECK(panel_manager_init(&panel_config));

    // Initialize touch sensor
    touch_sensor_config_t touch_config = {
        .gpio = CONFIG_TIMEMACHINE_TOUCH_GPIO,
        .active_high = true,
        .debounce_ms = CONFIG_TIMEMACHINE_TOUCH_DEBOUNCE_MS
    };
    ESP_ERROR_CHECK(touch_sensor_init(&touch_config));

    // Initialize network (async, emits events when ready)
    network_config_t network_config = {
        .wifi_ssid = CONFIG_TIMEMACHINE_WIFI_SSID,
        .wifi_password = CONFIG_TIMEMACHINE_WIFI_PASSWORD,
        .wifi_authmode = CONFIG_TIMEMACHINE_WIFI_AUTHMODE,
        .max_retries = 5
    };
    ESP_ERROR_CHECK(network_init(&network_config));

    ESP_LOGI(TAG, "Initialization complete, system is event-driven");
}

// ============================================================================
// Event Handlers
// ============================================================================

static void on_network_connected(void* arg, esp_event_base_t event_base,
                                  int32_t event_id, void* event_data)
{
    ESP_LOGI(TAG, "Network connected, starting NTP sync...");

    // Initialize NTP sync (async)
    ntp_sync_config_t ntp_config = {
        .server1 = CONFIG_TIMEMACHINE_NTP_SERVER1,
        .server2 = CONFIG_TIMEMACHINE_NTP_SERVER2,
        .timezone = CONFIG_TIMEMACHINE_TIMEZONE,
        .sync_interval_ms = 3600000  // 1 hour
    };

    esp_err_t ret = ntp_sync_init(&ntp_config, 30);  // 30 second timeout
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NTP sync");
    }
}

static void on_network_failed(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    ESP_LOGE(TAG, "Network connection failed");
}

static void on_ntp_synced(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    ESP_LOGI(TAG, "NTP synced, starting clock...");

    // Initialize clock
    clock_config_t clock_config = {
        .format = CONFIG_TIMEMACHINE_TIME_FORMAT,
        .show_seconds = CONFIG_TIMEMACHINE_SHOW_SECONDS
    };
    ESP_ERROR_CHECK(clock_init(&clock_config));

    // Register clock panel with panel manager
    panel_info_t clock_panel = {
        .id = PANEL_CLOCK,
        .name = "clock"
    };
    ESP_ERROR_CHECK(panel_manager_register_panel(&clock_panel));

    // Start tick task
    tick_task_config_t tick_config = {
        .interval_ms = 1000
    };
    ESP_ERROR_CHECK(tick_task_start(&tick_config));

    ESP_LOGI(TAG, "Time Machine ready!");
}

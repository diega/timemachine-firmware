/*
 * Time Machine - WiFi-connected clock for ESP32-C3
 * Copyright (C) 2025 Diego López León
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Inspired by ESPTimeCast (LGPL-2.1) by mfactory-osaka
 * https://github.com/mfactory-osaka/ESPTimeCast
 */

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
#include "settings.h"
#include "ble_config.h"
#include "brightness_control.h"
#include "network.h"
#include "ntp_sync.h"
#include "display.h"
#include "panel_manager.h"
#include "touch_sensor.h"
#include "clock_panel.h"
#include "date_panel.h"
#include "i18n.h"
#include "wifi_animation.h"

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

    // Initialize settings component (must be before other components)
    ESP_ERROR_CHECK(settings_init());

    // Get settings for initialization
    language_t language = settings_get_language();
    network_config_t network_config = settings_get_network();

    // Initialize i18n with configured language
    ESP_ERROR_CHECK(i18n_init(language));

    // Initialize BLE configuration service
    ESP_ERROR_CHECK(ble_config_init());

    // Initialize brightness control
    uint8_t brightness = settings_get_brightness();
    brightness_control_config_t brightness_config = {
        .initial_brightness = brightness,
        .cycle_interval_ms = 300  // 300ms between brightness changes
    };
    ESP_ERROR_CHECK(brightness_control_init(&brightness_config));

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

    // Initialize WiFi animation (must be before network init)
    ESP_ERROR_CHECK(wifi_animation_init());

    // Initialize touch sensor
    touch_sensor_config_t touch_config = {
        .gpio = CONFIG_TIMEMACHINE_TOUCH_GPIO,
        .active_high = true,
        .debounce_ms = CONFIG_TIMEMACHINE_TOUCH_DEBOUNCE_MS
    };
    ESP_ERROR_CHECK(touch_sensor_init(&touch_config));

    // Initialize network with settings (async, emits events when ready)
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

    // Get NTP config from settings
    ntp_sync_config_t ntp_config = settings_get_ntp();

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

    // Get clock config from settings
    clock_config_t clock_config = settings_get_clock();
    ESP_ERROR_CHECK(clock_panel_init(&clock_config));

    // Register clock panel with panel manager
    panel_info_t clock_panel = {
        .id = PANEL_CLOCK,
        .name = "clock"
    };
    ESP_ERROR_CHECK(panel_manager_register_panel(&clock_panel));

    // Initialize date panel
    ESP_ERROR_CHECK(date_panel_init());

    ESP_LOGI(TAG, "Time Machine ready!");
}

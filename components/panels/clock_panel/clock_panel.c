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
 */

#include "clock_panel.h"
#include "timemachine_events.h"
#include "fonts/font.h"
#include "i18n.h"
#include "esp_log.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

static const char *TAG = "clock_panel";

static clock_config_t s_config = {
    .format = TIME_FORMAT_24H,
    .show_seconds = true
};

static bool s_initialized = false;
static bool s_active = false;
static TimerHandle_t s_update_timer = NULL;
static esp_event_handler_instance_t s_panel_activated_handler = NULL;
static esp_event_handler_instance_t s_panel_deactivated_handler = NULL;
static esp_event_handler_instance_t s_config_changed_handler = NULL;

// Forward declarations
static void update_timer_callback(TimerHandle_t xTimer);
static void render_time(void);
static void panel_activated_handler(void* arg, esp_event_base_t base,
                                    int32_t event_id, void* event_data);
static void panel_deactivated_handler(void* arg, esp_event_base_t base,
                                      int32_t event_id, void* event_data);
static void on_clock_config_changed(void* arg, esp_event_base_t event_base,
                                     int32_t event_id, void* event_data);

// ============================================================================
// Public API - Lifecycle
// ============================================================================

esp_err_t clock_panel_init(const clock_config_t *config)
{
    if (config == NULL) {
        ESP_LOGE(TAG, "Invalid config");
        return ESP_ERR_INVALID_ARG;
    }

    if (s_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    s_config = *config;
    s_active = false;

    // Create update timer (will be started when panel is activated)
    s_update_timer = xTimerCreate(
        "clock_update",
        pdMS_TO_TICKS(1000),  // 1 second period
        pdTRUE,               // Auto-reload
        NULL,
        update_timer_callback
    );
    if (s_update_timer == NULL) {
        ESP_LOGE(TAG, "Failed to create update timer");
        return ESP_ERR_NO_MEM;
    }

    // Register PANEL_ACTIVATED handler
    esp_err_t err = esp_event_handler_instance_register(
        TIMEMACHINE_EVENT,
        PANEL_ACTIVATED,
        panel_activated_handler,
        NULL,
        &s_panel_activated_handler
    );
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register PANEL_ACTIVATED handler");
        clock_panel_deinit();
        return err;
    }

    // Register PANEL_DEACTIVATED handler
    err = esp_event_handler_instance_register(
        TIMEMACHINE_EVENT,
        PANEL_DEACTIVATED,
        panel_deactivated_handler,
        NULL,
        &s_panel_deactivated_handler
    );
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register PANEL_DEACTIVATED handler");
        clock_panel_deinit();
        return err;
    }

    // Register CLOCK_CONFIG_CHANGED handler
    err = esp_event_handler_instance_register(
        TIMEMACHINE_EVENT,
        CLOCK_CONFIG_CHANGED,
        on_clock_config_changed,
        NULL,
        &s_config_changed_handler
    );
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register CLOCK_CONFIG_CHANGED handler");
        clock_panel_deinit();
        return err;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "Clock initialized");

    return ESP_OK;
}

void clock_panel_deinit(void)
{
    if (!s_initialized) {
        return;
    }

    // Stop and delete timer
    if (s_update_timer != NULL) {
        xTimerStop(s_update_timer, 0);
        xTimerDelete(s_update_timer, 0);
        s_update_timer = NULL;
    }

    // Unregister all event handlers
    if (s_config_changed_handler != NULL) {
        esp_event_handler_instance_unregister(
            TIMEMACHINE_EVENT,
            CLOCK_CONFIG_CHANGED,
            s_config_changed_handler
        );
        s_config_changed_handler = NULL;
    }

    if (s_panel_deactivated_handler != NULL) {
        esp_event_handler_instance_unregister(
            TIMEMACHINE_EVENT,
            PANEL_DEACTIVATED,
            s_panel_deactivated_handler
        );
        s_panel_deactivated_handler = NULL;
    }

    if (s_panel_activated_handler != NULL) {
        esp_event_handler_instance_unregister(
            TIMEMACHINE_EVENT,
            PANEL_ACTIVATED,
            s_panel_activated_handler
        );
        s_panel_activated_handler = NULL;
    }

    s_initialized = false;
    ESP_LOGI(TAG, "Clock deinitialized");
}

// ============================================================================
// Private - Event Handlers
// ============================================================================

static void panel_activated_handler(void* arg, esp_event_base_t base,
                                   int32_t event_id, void* event_data)
{
    panel_id_t *panel_id = (panel_id_t *)event_data;

    if (*panel_id == PANEL_CLOCK) {
        s_active = true;
        ESP_LOGI(TAG, "Clock panel activated");

        // Start update timer
        if (s_update_timer != NULL) {
            render_time();  // Render immediately
            xTimerStart(s_update_timer, 0);
        }
    }
}

static void panel_deactivated_handler(void* arg, esp_event_base_t base,
                                     int32_t event_id, void* event_data)
{
    panel_id_t *panel_id = (panel_id_t *)event_data;

    if (*panel_id == PANEL_CLOCK) {
        s_active = false;
        ESP_LOGI(TAG, "Clock panel deactivated");

        // Stop update timer
        if (s_update_timer != NULL) {
            xTimerStop(s_update_timer, 0);
        }
    }
}

static void update_timer_callback(TimerHandle_t xTimer)
{
    render_time();
}

static void render_time(void)
{
    // Get current time from system
    time_t now = time(NULL);
    if (now <= 0) {
        return;  // Invalid time (not synced yet)
    }

    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    struct tm *time = &timeinfo;

    // Get day of week (localized)
    static char dow_str[4];  // "DDD\0" = max 4 chars
    const char *day = i18n_get_day_name(time->tm_wday);
    snprintf(dow_str, sizeof(dow_str), "%s", day);

    // Format time string based on configuration
    static char time_str[16];  // Static to persist after function returns
    int hour = time->tm_hour;
    int min = time->tm_min;
    int sec = time->tm_sec;

    // Convert to 12h if needed
    if (s_config.format == TIME_FORMAT_12H) {
        hour = hour % 12;
        if (hour == 0) {
            hour = 12;
        }
    }

    // Blink colon: show on even seconds, hide on odd seconds
    char separator = (sec % 2) == 0 ? ':' : ' ';

    // Format time as "HH:MM" or "H:MM" (or with space instead of colon)
    if (hour < 10) {
        snprintf(time_str, sizeof(time_str), "%d%c%02d", hour, separator, min);
    } else {
        snprintf(time_str, sizeof(time_str), "%02d%c%02d", hour, separator, min);
    }

    // Build scene with two text elements: day of week (dotmatrix) + time (default)
    static scene_element_t time_elements[2];

    // Day of week with small dotmatrix font (for the techito)
    time_elements[0].type = SCENE_ELEMENT_TEXT;
    time_elements[0].data.text.str = dow_str;
    time_elements[0].data.text.font = &font_dotmatrix_small;

    // Time with default font
    time_elements[1].type = SCENE_ELEMENT_TEXT;
    time_elements[1].data.text.str = time_str;
    time_elements[1].data.text.font = &font_default;

    static display_scene_t time_scene;
    time_scene.element_count = 2;
    time_scene.elements = time_elements;

    // Fallback text for simple displays
    static char fallback_str[20];
    snprintf(fallback_str, sizeof(fallback_str), "%s %s", dow_str, time_str);
    time_scene.fallback_text = fallback_str;

    // Emit RENDER_SCENE
    esp_err_t err = esp_event_post(
        DISPLAY_EVENT,
        RENDER_SCENE,
        &time_scene,
        sizeof(display_scene_t),
        portMAX_DELAY
    );

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to post display event: %s", esp_err_to_name(err));
    }
}

// ============================================================================
// Public API - Configuration
// ============================================================================

esp_err_t clock_panel_set_format(time_format_t format)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    s_config.format = format;
    ESP_LOGI(TAG, "Format changed to %s",
             format == TIME_FORMAT_24H ? "24h" : "12h");

    return ESP_OK;
}

time_format_t clock_panel_get_format(void)
{
    return s_config.format;
}

static void on_clock_config_changed(void* arg, esp_event_base_t event_base,
                                     int32_t event_id, void* event_data)
{
    clock_config_t *new_config = (clock_config_t*)event_data;

    ESP_LOGI(TAG, "Clock configuration changed: format=%s, show_seconds=%d",
             new_config->format == TIME_FORMAT_24H ? "24h" : "12h",
             new_config->show_seconds);

    // Update stored configuration
    s_config = *new_config;

    // Display will update on next timer tick with new format
}

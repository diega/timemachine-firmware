#include "clock.h"
#include "timemachine_events.h"
#include "esp_log.h"
#include "esp_event.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "clock";

static clock_config_t s_config = {
    .format = TIME_FORMAT_24H,
    .show_seconds = true
};

static bool s_initialized = false;
static bool s_active = false;
static esp_event_handler_instance_t s_time_tick_handler = NULL;
static esp_event_handler_instance_t s_panel_activated_handler = NULL;
static esp_event_handler_instance_t s_panel_deactivated_handler = NULL;

// Forward declarations
static void time_tick_event_handler(void* arg, esp_event_base_t base,
                                    int32_t event_id, void* event_data);
static void panel_activated_handler(void* arg, esp_event_base_t base,
                                    int32_t event_id, void* event_data);
static void panel_deactivated_handler(void* arg, esp_event_base_t base,
                                      int32_t event_id, void* event_data);

// ============================================================================
// Public API - Lifecycle
// ============================================================================

esp_err_t clock_init(const clock_config_t *config)
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

    // Register TIME_TICK handler
    esp_err_t err = esp_event_handler_instance_register(
        TIMEMACHINE_EVENT,
        TIME_TICK,
        time_tick_event_handler,
        NULL,
        &s_time_tick_handler
    );
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register TIME_TICK handler");
        clock_deinit();
        return err;
    }

    // Register PANEL_ACTIVATED handler
    err = esp_event_handler_instance_register(
        TIMEMACHINE_EVENT,
        PANEL_ACTIVATED,
        panel_activated_handler,
        NULL,
        &s_panel_activated_handler
    );
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register PANEL_ACTIVATED handler");
        clock_deinit();
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
        clock_deinit();
        return err;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "Clock initialized");

    return ESP_OK;
}

void clock_deinit(void)
{
    if (!s_initialized) {
        return;
    }

    // Unregister all event handlers
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

    if (s_time_tick_handler != NULL) {
        esp_event_handler_instance_unregister(
            TIMEMACHINE_EVENT,
            TIME_TICK,
            s_time_tick_handler
        );
        s_time_tick_handler = NULL;
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
    }
}

static void panel_deactivated_handler(void* arg, esp_event_base_t base,
                                     int32_t event_id, void* event_data)
{
    panel_id_t *panel_id = (panel_id_t *)event_data;

    if (*panel_id == PANEL_CLOCK) {
        s_active = false;
        ESP_LOGI(TAG, "Clock panel deactivated");
    }
}

static void time_tick_event_handler(void* arg, esp_event_base_t base,
                                    int32_t event_id, void* event_data)
{
    if (event_id != TIME_TICK) {
        return;
    }

    // Only render if this panel is active
    if (!s_active) {
        return;
    }

    timemachine_time_tick_t *tick_data = (timemachine_time_tick_t *)event_data;
    struct tm *time = &tick_data->timeinfo;

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

    // Build scene with single text element
    static scene_element_t time_element;
    time_element.type = SCENE_ELEMENT_TEXT;
    time_element.data.text.str = time_str;

    static display_scene_t time_scene;
    time_scene.element_count = 1;
    time_scene.elements = &time_element;
    time_scene.fallback_text = time_str;

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

esp_err_t clock_set_format(time_format_t format)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    s_config.format = format;
    ESP_LOGI(TAG, "Format changed to %s",
             format == TIME_FORMAT_24H ? "24h" : "12h");

    return ESP_OK;
}

time_format_t clock_get_format(void)
{
    return s_config.format;
}

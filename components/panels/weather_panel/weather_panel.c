/**
 * @file weather_panel.c
 * @brief Weather panel implementation
 */

#include "weather_panel.h"
#include "weather.h"
#include "timemachine_events.h"
#include "panel_manager.h"
#include "fonts/font.h"
#include "esp_log.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "weather_panel";

static bool s_initialized = false;
static bool s_active = false;
static TimerHandle_t s_update_timer = NULL;
static esp_event_handler_instance_t s_panel_activated_handler = NULL;
static esp_event_handler_instance_t s_panel_deactivated_handler = NULL;

// Forward declarations
static void update_timer_callback(TimerHandle_t xTimer);
static void render_weather(void);
static void panel_activated_handler(void* arg, esp_event_base_t base,
                                    int32_t event_id, void* event_data);
static void panel_deactivated_handler(void* arg, esp_event_base_t base,
                                      int32_t event_id, void* event_data);

// ============================================================================
// Public API - Lifecycle
// ============================================================================

esp_err_t weather_panel_init(void)
{
    if (s_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    s_active = false;

    // Create update timer (10 second refresh)
    s_update_timer = xTimerCreate(
        "weather_update",
        pdMS_TO_TICKS(10000),  // 10 second period
        pdTRUE,                // Auto-reload
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
        weather_panel_deinit();
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
        weather_panel_deinit();
        return err;
    }

    // Register panel with manager
    panel_info_t panel_info = {
        .id = PANEL_WEATHER,
        .name = "Weather"
    };
    err = panel_manager_register_panel(&panel_info);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register panel");
        weather_panel_deinit();
        return err;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "Weather panel initialized");

    return ESP_OK;
}

void weather_panel_deinit(void)
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
    ESP_LOGI(TAG, "Weather panel deinitialized");
}

// ============================================================================
// Private - Event Handlers
// ============================================================================

static void panel_activated_handler(void* arg, esp_event_base_t base,
                                   int32_t event_id, void* event_data)
{
    panel_id_t *panel_id = (panel_id_t *)event_data;

    if (*panel_id == PANEL_WEATHER) {
        s_active = true;
        ESP_LOGI(TAG, "Weather panel activated");

        // Start update timer
        if (s_update_timer != NULL) {
            render_weather();  // Render immediately
            xTimerStart(s_update_timer, 0);
        }
    }
}

static void panel_deactivated_handler(void* arg, esp_event_base_t base,
                                     int32_t event_id, void* event_data)
{
    panel_id_t *panel_id = (panel_id_t *)event_data;

    if (*panel_id == PANEL_WEATHER) {
        s_active = false;
        ESP_LOGI(TAG, "Weather panel deactivated");

        // Stop update timer
        if (s_update_timer != NULL) {
            xTimerStop(s_update_timer, 0);
        }
    }
}

static void update_timer_callback(TimerHandle_t xTimer)
{
    render_weather();
}

static void render_weather(void)
{
    // Get current weather data
    weather_data_t weather_data;
    esp_err_t err = weather_get_data(&weather_data);

    if (err != ESP_OK || !weather_data.valid) {
        ESP_LOGW(TAG, "No weather data available, requesting panel skip");
        // Request to skip this panel
        esp_event_post(
            TIMEMACHINE_EVENT,
            PANEL_SKIP_REQUESTED,
            NULL,
            0,
            0
        );
        return;
    }

    // Format temperature string (e.g., "23°C")
    // Convert temperature to integer to avoid float formatting (which uses malloc)
    int temp_int = (int)(weather_data.temperature + 0.5f);  // Round to nearest
    static char temp_str[16];
    int len = snprintf(temp_str, sizeof(temp_str), "%d", temp_int);
    // Manually append degree symbol (0xB0) and 'C'
    if (len > 0 && len < sizeof(temp_str) - 2) {
        temp_str[len] = 0xB0;      // Degree symbol
        temp_str[len + 1] = 'C';   // C
        temp_str[len + 2] = '\0';  // Null terminator
    }

    // Build scene with just temperature text
    static scene_element_t weather_elements[1];

    // Temperature with dotmatrix font
    weather_elements[0].type = SCENE_ELEMENT_TEXT;
    weather_elements[0].data.text.str = temp_str;
    weather_elements[0].data.text.font = &font_dotmatrix;

    static display_scene_t weather_scene;
    weather_scene.element_count = 1;
    weather_scene.elements = weather_elements;

    // Fallback text
    weather_scene.fallback_text = temp_str;

    // Emit RENDER_SCENE
    err = esp_event_post(
        DISPLAY_EVENT,
        RENDER_SCENE,
        &weather_scene,
        sizeof(display_scene_t),
        portMAX_DELAY
    );

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to post display event: %s", esp_err_to_name(err));
    }
}

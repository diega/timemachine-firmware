#include "tick_task.h"
#include "timemachine_events.h"
#include <time.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

static const char *TAG = "tick_task";

static TimerHandle_t s_tick_timer = NULL;
static uint32_t s_interval_ms = 1000;

// Forward declaration
static void tick_timer_callback(TimerHandle_t xTimer);

// ============================================================================
// Public API
// ============================================================================

esp_err_t tick_task_start(const tick_task_config_t *config)
{
    if (s_tick_timer != NULL) {
        ESP_LOGW(TAG, "Tick timer already running");
        return ESP_OK;
    }

    if (config != NULL && config->interval_ms > 0) {
        s_interval_ms = config->interval_ms;
    }

    ESP_LOGI(TAG, "Starting tick timer (interval: %lu ms)", s_interval_ms);

    // Create timer (auto-reload for periodic ticks)
    s_tick_timer = xTimerCreate(
        "tick_timer",                           // Timer name
        pdMS_TO_TICKS(s_interval_ms),          // Period
        pdTRUE,                                 // Auto-reload
        NULL,                                   // Timer ID (not used)
        tick_timer_callback                     // Callback function
    );

    if (s_tick_timer == NULL) {
        ESP_LOGE(TAG, "Failed to create tick timer");
        return ESP_FAIL;
    }

    // Start timer
    if (xTimerStart(s_tick_timer, 0) != pdPASS) {
        ESP_LOGE(TAG, "Failed to start tick timer");
        xTimerDelete(s_tick_timer, 0);
        s_tick_timer = NULL;
        return ESP_FAIL;
    }

    return ESP_OK;
}

// ============================================================================
// Private - Timer Callback
// ============================================================================

static void tick_timer_callback(TimerHandle_t xTimer)
{
    // Get current time from system
    time_t now = time(NULL);

    if (now > 0) {  // Valid time (not 1970)
        timemachine_time_tick_t tick_data;
        tick_data.timestamp = now;
        localtime_r(&now, &tick_data.timeinfo);

        // Publish time tick event
        esp_err_t err = esp_event_post(
            TIMEMACHINE_EVENT,
            TIME_TICK,
            &tick_data,
            sizeof(tick_data),
            0  // No wait (callback must not block)
        );

        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Failed to post time tick event: %s", esp_err_to_name(err));
        }
    }
}

// ============================================================================
// Public API - Cleanup
// ============================================================================

esp_err_t tick_task_stop(void)
{
    if (s_tick_timer == NULL) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Stopping tick timer");

    xTimerStop(s_tick_timer, portMAX_DELAY);
    xTimerDelete(s_tick_timer, portMAX_DELAY);
    s_tick_timer = NULL;

    return ESP_OK;
}

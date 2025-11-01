/**
 * @file brightness_control.c
 * @brief Display brightness control implementation
 */

#include "brightness_control.h"
#include "timemachine_events.h"
#include "esp_log.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

static const char *TAG = "brightness_control";

// Brightness cycle levels (5 steps between min and max)
static const uint8_t BRIGHTNESS_LEVELS[] = {2, 5, 9, 12, 15};
#define BRIGHTNESS_LEVEL_COUNT (sizeof(BRIGHTNESS_LEVELS) / sizeof(BRIGHTNESS_LEVELS[0]))

static struct {
    bool initialized;
    brightness_control_config_t config;
    uint8_t current_brightness;
    uint8_t current_level_index;
    bool cycling;
    bool going_up;
    bool at_limit;  // Flag to pause at min/max before reversing
    panel_id_t active_panel;
    TimerHandle_t cycle_timer;
    esp_event_handler_instance_t press_handler;
    esp_event_handler_instance_t release_handler;
    esp_event_handler_instance_t panel_activated_handler;
} s_state = {0};

// Forward declarations
static void input_long_press_handler(void* arg, esp_event_base_t base,
                                     int32_t event_id, void* event_data);
static void input_release_handler(void* arg, esp_event_base_t base,
                                  int32_t event_id, void* event_data);
static void panel_activated_handler(void* arg, esp_event_base_t base,
                                    int32_t event_id, void* event_data);
static void cycle_timer_callback(TimerHandle_t timer);
static void emit_brightness_changed(void);

// ============================================================================
// Private - Brightness Cycling
// ============================================================================

static void cycle_brightness(void)
{
    // If at limit, pause for one cycle before reversing direction
    if (s_state.at_limit) {
        s_state.at_limit = false;
        ESP_LOGI(TAG, "Limit pause complete, reversing direction");
        return;
    }

    // Move to next level
    if (s_state.going_up) {
        s_state.current_level_index++;
        if (s_state.current_level_index >= BRIGHTNESS_LEVEL_COUNT) {
            // Reached max, pause before going down
            s_state.current_level_index = BRIGHTNESS_LEVEL_COUNT - 1;
            s_state.at_limit = true;
            s_state.going_up = false;
            ESP_LOGI(TAG, "Reached maximum brightness, pausing");
        }
    } else {
        if (s_state.current_level_index == 0) {
            // Reached min, pause before going up
            s_state.at_limit = true;
            s_state.going_up = true;
            ESP_LOGI(TAG, "Reached minimum brightness, pausing");
        } else {
            s_state.current_level_index--;
        }
    }

    s_state.current_brightness = BRIGHTNESS_LEVELS[s_state.current_level_index];

    ESP_LOGI(TAG, "Brightness: %d (index %d, %s)",
             s_state.current_brightness,
             s_state.current_level_index,
             s_state.going_up ? "up" : "down");

    emit_brightness_changed();
}

static void cycle_timer_callback(TimerHandle_t timer)
{
    if (s_state.cycling) {
        cycle_brightness();
    }
}

static void emit_brightness_changed(void)
{
    uint8_t brightness = s_state.current_brightness;
    esp_err_t err = esp_event_post(
        TIMEMACHINE_EVENT,
        BRIGHTNESS_CHANGED,
        &brightness,
        sizeof(brightness),
        0  // Don't block
    );

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to post BRIGHTNESS_CHANGED event: %s", esp_err_to_name(err));
    }
}

// ============================================================================
// Private - Event Handlers
// ============================================================================

static void input_long_press_handler(void* arg, esp_event_base_t base,
                                     int32_t event_id, void* event_data)
{
    // Only respond if clock panel is active
    if (s_state.active_panel != PANEL_CLOCK) {
        return;
    }

    ESP_LOGI(TAG, "Long press detected - starting brightness cycle");

    s_state.cycling = true;
    s_state.going_up = true;
    s_state.at_limit = false;

    // Start cycling timer
    xTimerStart(s_state.cycle_timer, 0);
}

static void input_release_handler(void* arg, esp_event_base_t base,
                                  int32_t event_id, void* event_data)
{
    if (!s_state.cycling) {
        return;
    }

    ESP_LOGI(TAG, "Release detected - stopping brightness cycle at level %d",
             s_state.current_brightness);

    s_state.cycling = false;

    // Stop cycling timer
    xTimerStop(s_state.cycle_timer, 0);
}

static void panel_activated_handler(void* arg, esp_event_base_t base,
                                    int32_t event_id, void* event_data)
{
    if (event_data == NULL) {
        return;
    }

    panel_id_t *panel_id = (panel_id_t *)event_data;
    s_state.active_panel = *panel_id;

    ESP_LOGD(TAG, "Active panel changed to %d", s_state.active_panel);
}

// ============================================================================
// Public API
// ============================================================================

esp_err_t brightness_control_init(const brightness_control_config_t *config)
{
    if (config == NULL) {
        ESP_LOGE(TAG, "Invalid config");
        return ESP_ERR_INVALID_ARG;
    }

    if (s_state.initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    // Validate initial brightness
    if (config->initial_brightness < BRIGHTNESS_MIN ||
        config->initial_brightness > BRIGHTNESS_MAX) {
        ESP_LOGE(TAG, "Initial brightness out of range (%d-%d)",
                 BRIGHTNESS_MIN, BRIGHTNESS_MAX);
        return ESP_ERR_INVALID_ARG;
    }

    s_state.config = *config;
    s_state.current_brightness = config->initial_brightness;
    s_state.cycling = false;
    s_state.going_up = true;
    s_state.active_panel = PANEL_CLOCK;  // Default to clock panel

    // Find closest brightness level index
    s_state.current_level_index = 0;
    for (uint8_t i = 0; i < BRIGHTNESS_LEVEL_COUNT; i++) {
        if (BRIGHTNESS_LEVELS[i] >= s_state.current_brightness) {
            s_state.current_level_index = i;
            break;
        }
    }

    // Create cycle timer
    s_state.cycle_timer = xTimerCreate(
        "brightness_cycle",
        pdMS_TO_TICKS(config->cycle_interval_ms),
        pdTRUE,  // Auto-reload timer
        NULL,
        cycle_timer_callback
    );

    if (s_state.cycle_timer == NULL) {
        ESP_LOGE(TAG, "Failed to create cycle timer");
        return ESP_ERR_NO_MEM;
    }

    // Register INPUT_LONG_PRESS handler
    esp_err_t err = esp_event_handler_instance_register(
        TIMEMACHINE_EVENT,
        INPUT_LONG_PRESS,
        input_long_press_handler,
        NULL,
        &s_state.press_handler
    );
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register INPUT_LONG_PRESS handler");
        xTimerDelete(s_state.cycle_timer, 0);
        return err;
    }

    // Register INPUT_RELEASE handler
    err = esp_event_handler_instance_register(
        TIMEMACHINE_EVENT,
        INPUT_RELEASE,
        input_release_handler,
        NULL,
        &s_state.release_handler
    );
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register INPUT_RELEASE handler");
        esp_event_handler_instance_unregister(TIMEMACHINE_EVENT, INPUT_LONG_PRESS,
                                              s_state.press_handler);
        xTimerDelete(s_state.cycle_timer, 0);
        return err;
    }

    // Register PANEL_ACTIVATED handler to track active panel
    err = esp_event_handler_instance_register(
        TIMEMACHINE_EVENT,
        PANEL_ACTIVATED,
        panel_activated_handler,
        NULL,
        &s_state.panel_activated_handler
    );
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register PANEL_ACTIVATED handler");
        esp_event_handler_instance_unregister(TIMEMACHINE_EVENT, INPUT_LONG_PRESS,
                                              s_state.press_handler);
        esp_event_handler_instance_unregister(TIMEMACHINE_EVENT, INPUT_RELEASE,
                                              s_state.release_handler);
        xTimerDelete(s_state.cycle_timer, 0);
        return err;
    }

    s_state.initialized = true;
    ESP_LOGI(TAG, "Brightness control initialized (initial: %d, cycle: %lums)",
             config->initial_brightness, config->cycle_interval_ms);

    return ESP_OK;
}

void brightness_control_deinit(void)
{
    if (!s_state.initialized) {
        return;
    }

    // Stop and delete timer
    if (s_state.cycle_timer != NULL) {
        xTimerStop(s_state.cycle_timer, 0);
        xTimerDelete(s_state.cycle_timer, 0);
        s_state.cycle_timer = NULL;
    }

    // Unregister event handlers
    if (s_state.press_handler != NULL) {
        esp_event_handler_instance_unregister(TIMEMACHINE_EVENT, INPUT_LONG_PRESS,
                                              s_state.press_handler);
        s_state.press_handler = NULL;
    }

    if (s_state.release_handler != NULL) {
        esp_event_handler_instance_unregister(TIMEMACHINE_EVENT, INPUT_RELEASE,
                                              s_state.release_handler);
        s_state.release_handler = NULL;
    }

    if (s_state.panel_activated_handler != NULL) {
        esp_event_handler_instance_unregister(TIMEMACHINE_EVENT, PANEL_ACTIVATED,
                                              s_state.panel_activated_handler);
        s_state.panel_activated_handler = NULL;
    }

    s_state.initialized = false;
    ESP_LOGI(TAG, "Brightness control deinitialized");
}

uint8_t brightness_control_get_level(void)
{
    return s_state.current_brightness;
}

esp_err_t brightness_control_set_level(uint8_t level)
{
    if (level < BRIGHTNESS_MIN || level > BRIGHTNESS_MAX) {
        ESP_LOGE(TAG, "Brightness level out of range (%d-%d)",
                 BRIGHTNESS_MIN, BRIGHTNESS_MAX);
        return ESP_ERR_INVALID_ARG;
    }

    s_state.current_brightness = level;

    // Find closest brightness level index
    s_state.current_level_index = 0;
    for (uint8_t i = 0; i < BRIGHTNESS_LEVEL_COUNT; i++) {
        if (BRIGHTNESS_LEVELS[i] >= level) {
            s_state.current_level_index = i;
            s_state.current_brightness = BRIGHTNESS_LEVELS[i];
            break;
        }
    }

    emit_brightness_changed();

    return ESP_OK;
}

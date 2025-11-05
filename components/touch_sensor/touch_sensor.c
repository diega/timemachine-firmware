#include "touch_sensor.h"
#include "timemachine_events.h"
#include "esp_log.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

static const char *TAG = "touch_sensor";

#define LONG_PRESS_MS 200  // Time to wait before emitting INPUT_LONG_PRESS

static struct {
    bool initialized;
    touch_sensor_config_t config;
    TickType_t last_touch_time;
    TimerHandle_t release_timer;
    bool is_pressed;
    bool long_press_detected;
    // Debug counters
    uint32_t press_count;
    uint32_t release_count;
    uint32_t debounce_skip_count;
} s_state = {0};

// Forward declarations
static void gpio_isr_handler(void* arg);
static void release_timer_callback(TimerHandle_t timer);

// ============================================================================
// Private - Timer Callback
// ============================================================================

static void release_timer_callback(TimerHandle_t timer)
{
    // Check if button is still pressed
    int level = gpio_get_level(s_state.config.gpio);
    bool is_touch_active = (level == 1) == s_state.config.active_high;

    if (!is_touch_active) {
        // Button was released
        if (s_state.long_press_detected) {
            // Long press was active, now released
            ESP_LOGI(TAG, "Long press released (polling detected)");
            s_state.is_pressed = false;
            s_state.long_press_detected = false;

            // Reset timer to original long press period for next press
            xTimerChangePeriod(timer, pdMS_TO_TICKS(LONG_PRESS_MS), 0);

            // Post INPUT_RELEASE event
            esp_err_t err = esp_event_post(
                TIMEMACHINE_EVENT,
                INPUT_RELEASE,
                NULL,
                0,
                portMAX_DELAY
            );

            if (err != ESP_OK) {
                ESP_LOGW(TAG, "Failed to post INPUT_RELEASE event: %s", esp_err_to_name(err));
            }
        } else if (s_state.is_pressed) {
            // Timer expired and button already released - this was a tap
            ESP_LOGI(TAG, "Tap detected (button released before long press)");
            s_state.is_pressed = false;

            // Post INPUT_TAP event
            esp_err_t err = esp_event_post(
                TIMEMACHINE_EVENT,
                INPUT_TAP,
                NULL,
                0,
                portMAX_DELAY
            );

            if (err != ESP_OK) {
                ESP_LOGW(TAG, "Failed to post INPUT_TAP event: %s", esp_err_to_name(err));
            }
        }
        return;
    }

    if (!s_state.long_press_detected) {
        // First time timer expired - button still pressed, this is a long press
        s_state.long_press_detected = true;
        ESP_LOGI(TAG, "Long press detected (level=%d), starting polling", level);

        // Post INPUT_LONG_PRESS event
        esp_err_t err = esp_event_post(
            TIMEMACHINE_EVENT,
            INPUT_LONG_PRESS,
            NULL,
            0,
            portMAX_DELAY
        );

        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Failed to post INPUT_LONG_PRESS event: %s", esp_err_to_name(err));
        }

        // Start polling for release - restart timer with shorter period
        xTimerChangePeriod(timer, pdMS_TO_TICKS(50), 0);  // Poll every 50ms
        xTimerStart(timer, 0);  // Restart the one-shot timer
    } else {
        // Already in long press mode, keep polling by restarting timer
        xTimerStart(timer, 0);  // Restart the one-shot timer for next poll
    }
}

// ============================================================================
// Public API
// ============================================================================

esp_err_t touch_sensor_init(const touch_sensor_config_t *config)
{
    if (config == NULL) {
        ESP_LOGE(TAG, "Invalid config");
        return ESP_ERR_INVALID_ARG;
    }

    if (s_state.initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    s_state.config = *config;
    s_state.last_touch_time = 0;
    s_state.is_pressed = false;
    s_state.long_press_detected = false;

    // Create timer for detecting long press
    s_state.release_timer = xTimerCreate(
        "touch_release",
        pdMS_TO_TICKS(LONG_PRESS_MS),
        pdFALSE,  // One-shot timer
        NULL,
        release_timer_callback
    );

    if (s_state.release_timer == NULL) {
        ESP_LOGE(TAG, "Failed to create release timer");
        return ESP_ERR_NO_MEM;
    }

    // Configure GPIO with interrupts on both edges
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << config->gpio),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = config->active_high ? GPIO_PULLDOWN_ONLY : GPIO_PULLUP_ONLY,
        .pull_down_en = config->active_high ? GPIO_PULLUP_ONLY : GPIO_PULLDOWN_ONLY,
        .intr_type = GPIO_INTR_ANYEDGE  // Detect both press and release
    };

    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO");
        xTimerDelete(s_state.release_timer, 0);
        return err;
    }

    // Install GPIO ISR service
    err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        // ESP_ERR_INVALID_STATE means service already installed, which is OK
        ESP_LOGE(TAG, "Failed to install ISR service");
        xTimerDelete(s_state.release_timer, 0);
        return err;
    }

    // Add ISR handler for this GPIO
    err = gpio_isr_handler_add(config->gpio, gpio_isr_handler, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add ISR handler");
        xTimerDelete(s_state.release_timer, 0);
        return err;
    }

    s_state.initialized = true;
    ESP_LOGI(TAG, "Touch sensor initialized (GPIO %d, active_%s, debounce %lums)",
             config->gpio,
             config->active_high ? "high" : "low",
             config->debounce_ms);

    return ESP_OK;
}

void touch_sensor_deinit(void)
{
    if (!s_state.initialized) {
        return;
    }

    // Stop and delete timer
    if (s_state.release_timer != NULL) {
        xTimerStop(s_state.release_timer, 0);
        xTimerDelete(s_state.release_timer, 0);
        s_state.release_timer = NULL;
    }

    // Remove ISR handler
    gpio_isr_handler_remove(s_state.config.gpio);

    // Reset GPIO
    gpio_reset_pin(s_state.config.gpio);

    s_state.initialized = false;
    ESP_LOGI(TAG, "Touch sensor deinitialized");
}

// ============================================================================
// Private - ISR Handler
// ============================================================================

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    TickType_t now = xTaskGetTickCountFromISR();
    TickType_t debounce_ticks = pdMS_TO_TICKS(s_state.config.debounce_ms);

    // Debounce: ignore if event happened too recently
    if ((now - s_state.last_touch_time) < debounce_ticks) {
        return;
    }

    s_state.last_touch_time = now;

    // Read current GPIO state
    int level = gpio_get_level(s_state.config.gpio);
    bool is_touch_active = (level == 1) == s_state.config.active_high;

    BaseType_t high_priority_task_woken = pdFALSE;

    if (is_touch_active && !s_state.is_pressed) {
        // Touch PRESSED
        s_state.is_pressed = true;
        s_state.long_press_detected = false;

        // Start timer to detect if this becomes a long press
        xTimerStartFromISR(s_state.release_timer, &high_priority_task_woken);
    }
    else if (!is_touch_active && s_state.is_pressed) {
        // Touch RELEASED
        s_state.is_pressed = false;

        // Stop the timer
        xTimerStopFromISR(s_state.release_timer, &high_priority_task_woken);

        if (s_state.long_press_detected) {
            // Long press was detected - emit INPUT_RELEASE to signal end of long press
            esp_event_isr_post(
                TIMEMACHINE_EVENT,
                INPUT_RELEASE,
                NULL,
                0,
                &high_priority_task_woken
            );
        } else {
            // Short press - emit INPUT_TAP
            esp_event_isr_post(
                TIMEMACHINE_EVENT,
                INPUT_TAP,
                NULL,
                0,
                &high_priority_task_woken
            );
        }
    }

    if (high_priority_task_woken) {
        portYIELD_FROM_ISR();
    }
}

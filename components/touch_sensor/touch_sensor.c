#include "touch_sensor.h"
#include "timemachine_events.h"
#include "esp_log.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "touch_sensor";

static struct {
    bool initialized;
    touch_sensor_config_t config;
    TickType_t last_touch_time;
} s_state = {0};

// Forward declarations
static void IRAM_ATTR gpio_isr_handler(void* arg);

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

    // Configure GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << config->gpio),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = config->active_high ? GPIO_PULLDOWN_ONLY : GPIO_PULLUP_ONLY,
        .pull_down_en = config->active_high ? GPIO_PULLUP_ONLY : GPIO_PULLDOWN_ONLY,
        .intr_type = config->active_high ? GPIO_INTR_POSEDGE : GPIO_INTR_NEGEDGE
    };

    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO");
        return err;
    }

    // Install GPIO ISR service
    err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        // ESP_ERR_INVALID_STATE means service already installed, which is OK
        ESP_LOGE(TAG, "Failed to install ISR service");
        return err;
    }

    // Add ISR handler for this GPIO
    err = gpio_isr_handler_add(config->gpio, gpio_isr_handler, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add ISR handler");
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

    // Debounce: ignore if touch happened too recently
    if ((now - s_state.last_touch_time) < debounce_ticks) {
        return;
    }

    s_state.last_touch_time = now;

    // Post INPUT_TOUCH event (from ISR)
    BaseType_t high_priority_task_woken = pdFALSE;
    esp_err_t err = esp_event_isr_post(
        TIMEMACHINE_EVENT,
        INPUT_TOUCH,
        NULL,
        0,
        &high_priority_task_woken
    );

    if (err != ESP_OK) {
        // Can't log from ISR, just ignore error
    }

    if (high_priority_task_woken) {
        portYIELD_FROM_ISR();
    }
}

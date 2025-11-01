#pragma once

#include "esp_err.h"
#include "driver/gpio.h"


/**
 * @brief Touch sensor configuration
 */
typedef struct {
    gpio_num_t gpio;           /**< GPIO pin for TTP223 sensor */
    bool active_high;          /**< true if touch = HIGH, false if touch = LOW */
    uint32_t debounce_ms;      /**< Debounce time in milliseconds */
} touch_sensor_config_t;

/**
 * @brief Initialize touch sensor
 *
 * Configures GPIO and sets up interrupt handler to detect touches.
 * Emits INPUT_PRESS when touched, INPUT_RELEASE when released,
 * and INPUT_TAP for short taps (press + quick release).
 *
 * @param config Touch sensor configuration
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t touch_sensor_init(const touch_sensor_config_t *config);

/**
 * @brief Deinitialize touch sensor
 */
void touch_sensor_deinit(void);


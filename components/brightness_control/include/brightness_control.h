/**
 * @file brightness_control.h
 * @brief Display brightness control via long press
 *
 * Manages LED display brightness using long press gestures on the touch sensor.
 * When the user long-presses while on the clock panel, brightness cycles through
 * predefined levels until released.
 */

#pragma once

#include "esp_err.h"
#include <stdint.h>

#define BRIGHTNESS_MIN 2   /**< Minimum brightness (avoid complete darkness) */
#define BRIGHTNESS_MAX 15  /**< Maximum brightness (MAX7219 maximum) */

/**
 * @brief Brightness control configuration
 */
typedef struct {
    uint8_t initial_brightness;  /**< Initial brightness level (2-15) */
    uint32_t cycle_interval_ms;  /**< Time between brightness changes during long press */
} brightness_control_config_t;

/**
 * @brief Initialize brightness control
 *
 * Sets up event handlers for INPUT_PRESS and INPUT_RELEASE to control
 * brightness via long press gestures. Only active when clock panel is visible.
 *
 * @param config Brightness control configuration
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t brightness_control_init(const brightness_control_config_t *config);

/**
 * @brief Deinitialize brightness control
 */
void brightness_control_deinit(void);

/**
 * @brief Get current brightness level
 *
 * @return Current brightness level (2-15)
 */
uint8_t brightness_control_get_level(void);

/**
 * @brief Set brightness level
 *
 * @param level Brightness level (2-15)
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if out of range
 */
esp_err_t brightness_control_set_level(uint8_t level);

#pragma once

#include <stdbool.h>
#include "esp_err.h"


/**
 * @brief Time format options
 */
typedef enum {
    TIME_FORMAT_12H,  /**< 12-hour format with AM/PM */
    TIME_FORMAT_24H   /**< 24-hour format */
} time_format_t;

/**
 * @brief Clock configuration
 */
typedef struct {
    time_format_t format;   /**< Time format (12h or 24h) */
    bool show_seconds;      /**< Show seconds in display */
} clock_config_t;

/**
 * @brief Initialize clock component
 *
 * Creates internal timer and registers event handlers for panel
 * activation/deactivation. Timer updates display every second when
 * the clock panel is active.
 *
 * @param config Clock configuration
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t clock_panel_init(const clock_config_t *config);

/**
 * @brief Set time format
 *
 * @param format New time format
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t clock_panel_set_format(time_format_t format);

/**
 * @brief Get current time format
 *
 * @return Current time format
 */
time_format_t clock_panel_get_format(void);

/**
 * @brief Deinitialize clock component
 */
void clock_panel_deinit(void);


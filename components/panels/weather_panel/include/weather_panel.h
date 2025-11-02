/**
 * @file weather_panel.h
 * @brief Weather display panel
 *
 * Displays current temperature and weather icon on MAX7219 display.
 * Shows temperature in Celsius with weather condition icon.
 */

#pragma once

#include "esp_err.h"

/**
 * @brief Initialize weather panel
 *
 * Registers the weather panel with the panel manager and sets up
 * event handlers for panel activation/deactivation.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t weather_panel_init(void);

/**
 * @brief Deinitialize weather panel
 */
void weather_panel_deinit(void);

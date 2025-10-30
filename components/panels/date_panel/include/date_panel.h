/**
 * @file date_panel.h
 * @brief Date display panel
 *
 * Displays current date in "MMM DD" format (e.g., "Oct 30")
 */

#pragma once

#include "esp_err.h"

/**
 * @brief Initialize date panel
 *
 * Registers with panel manager and sets up event handlers
 * for panel activation/deactivation. Date is rendered when
 * the panel is activated.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t date_panel_init(void);

/**
 * @brief Deinitialize date panel
 */
void date_panel_deinit(void);

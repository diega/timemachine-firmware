/**
 * @file settings.h
 * @brief Settings component for managing configuration
 *
 * This component provides access to system configuration settings,
 * loading them from NVS or using Kconfig defaults. It also listens
 * to configuration change events and persists them to NVS.
 */

#pragma once

#include "esp_err.h"
#include "network.h"
#include "clock_panel.h"
#include "ntp_sync.h"
#include "i18n.h"

/**
 * @brief Initialize settings component
 *
 * Loads settings from NVS (or uses Kconfig defaults if not found).
 * Registers event handlers to persist configuration changes.
 *
 * @return ESP_OK on success
 */
esp_err_t settings_init(void);

/**
 * @brief Get current network configuration
 *
 * @return Current network config (from NVS or defaults)
 */
network_config_t settings_get_network(void);

/**
 * @brief Get current clock configuration
 *
 * @return Current clock config (from NVS or defaults)
 */
clock_config_t settings_get_clock(void);

/**
 * @brief Get current NTP configuration
 *
 * @return Current NTP config (from NVS or defaults)
 */
ntp_sync_config_t settings_get_ntp(void);

/**
 * @brief Get current language setting
 *
 * @return Current language (from NVS or defaults)
 */
language_t settings_get_language(void);

/**
 * @brief Get current display brightness
 *
 * @return Current brightness level (2-15)
 */
uint8_t settings_get_brightness(void);

/**
 * @brief Deinitialize settings component
 */
void settings_deinit(void);

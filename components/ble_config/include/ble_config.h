/**
 * @file ble_config.h
 * @brief BLE configuration component for Time Machine
 *
 * Exposes GATT services for runtime configuration via BLE.
 * Emits configuration change events that are handled by the
 * settings component and respective subsystems.
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief Initialize BLE configuration service
 *
 * Sets up BLE GATT server with configuration services:
 * - Network configuration (WiFi SSID, password, authmode)
 * - Clock configuration (time format, show seconds)
 * - NTP configuration (timezone, servers, sync interval)
 * - Language configuration
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ble_config_init(void);

/**
 * @brief Deinitialize BLE configuration service
 */
void ble_config_deinit(void);

/**
 * @brief Check if BLE is currently connected to a client
 *
 * @return true if connected, false otherwise
 */
bool ble_config_is_connected(void);

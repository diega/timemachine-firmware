#pragma once

#include <stdbool.h>
#include "esp_err.h"


/**
 * @brief Network component configuration
 */
typedef struct {
    const char *wifi_ssid;        /**< WiFi SSID */
    const char *wifi_password;    /**< WiFi password */
    uint8_t wifi_authmode;        /**< WiFi authentication mode */
    uint8_t max_retries;          /**< Maximum connection retry attempts */
} network_config_t;

/**
 * @brief Initialize network component and start WiFi connection
 *
 * This function starts the WiFi connection process asynchronously.
 * It will emit NETWORK_EVENT_CONNECTED or NETWORK_EVENT_FAILED when done.
 *
 * @param config Network configuration
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t network_init(const network_config_t *config);

/**
 * @brief Deinitialize network component
 */
void network_deinit(void);

/**
 * @brief Check if network is connected
 *
 * @return true if connected, false otherwise
 */
bool network_is_connected(void);


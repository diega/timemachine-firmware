#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize WiFi animation component
 *
 * This component listens for network events and displays an animated
 * WiFi connection indicator while connecting.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_animation_init(void);

/**
 * @brief Deinitialize WiFi animation component
 */
void wifi_animation_deinit(void);

#ifdef __cplusplus
}
#endif

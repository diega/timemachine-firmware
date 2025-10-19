#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Tick task configuration
 */
typedef struct {
    uint32_t interval_ms;  /**< Tick interval in milliseconds (default: 1000) */
} tick_task_config_t;

/**
 * @brief Initialize and start the tick task
 *
 * This task will publish TIME_TICK events periodically
 * by reading the system time.
 *
 * @param config Task configuration (can be NULL for defaults)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t tick_task_start(const tick_task_config_t *config);

/**
 * @brief Stop the tick task
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t tick_task_stop(void);

#ifdef __cplusplus
}
#endif

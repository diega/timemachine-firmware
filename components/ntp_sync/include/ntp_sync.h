#pragma once

#include <stdbool.h>
#include <time.h>
#include "esp_err.h"


/**
 * @brief NTP configuration structure
 */
typedef struct {
    const char *server1;        /**< Primary NTP server (e.g., "pool.ntp.org") */
    const char *server2;        /**< Secondary NTP server (optional) */
    const char *timezone;       /**< Timezone string (e.g., "EST5EDT,M3.2.0/2,M11.1.0") */
    uint32_t sync_interval_ms;  /**< Sync interval in milliseconds (default: 3600000 = 1 hour) */
} ntp_sync_config_t;

/**
 * @brief Initialize and start NTP synchronization
 *
 * This will perform an initial sync and then periodically sync in the background.
 * The function blocks until the initial sync completes or times out.
 *
 * @param config NTP configuration
 * @param timeout_sec Maximum time to wait for initial sync (seconds)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ntp_sync_init(const ntp_sync_config_t *config, uint32_t timeout_sec);

/**
 * @brief Check if time has been synchronized at least once
 *
 * @return true if time is synchronized, false otherwise
 */
bool ntp_sync_is_synced(void);

/**
 * @brief Deinitialize NTP synchronization and stop background task
 */
void ntp_sync_deinit(void);


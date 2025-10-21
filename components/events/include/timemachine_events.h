#pragma once

#include "esp_event.h"
#include "display_scene.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Timemachine event base
 */
ESP_EVENT_DECLARE_BASE(TIMEMACHINE_EVENT);

/**
 * @brief Display event base
 */
ESP_EVENT_DECLARE_BASE(DISPLAY_EVENT);

/**
 * @brief Panel IDs for navigation
 */
typedef enum {
    PANEL_CLOCK = 0,      /**< Clock panel (default) */
} panel_id_t;

/**
 * @brief Timemachine event IDs
 */
typedef enum {
    TIME_TICK,            /**< Time tick event (every second) */
    NTP_SYNCED,           /**< NTP sync completed */
    NETWORK_CONNECTING,   /**< Network connection in progress */
    NETWORK_CONNECTED,    /**< Network connected successfully */
    NETWORK_FAILED,       /**< Network connection failed */
    INPUT_TOUCH,          /**< Touch input detected */
    PANEL_ACTIVATED,      /**< Panel was activated */
    PANEL_DEACTIVATED,    /**< Panel was deactivated */
    WEATHER_UPDATE,       /**< Weather data updated (future) */
} timemachine_event_id_t;

/**
 * @brief Time tick event data
 */
typedef struct {
    struct tm timeinfo;  /**< Current time information */
    time_t timestamp;    /**< Unix timestamp */
} timemachine_time_tick_t;

/**
 * @brief NTP sync event data
 */
typedef struct {
    bool success;        /**< Sync was successful */
    time_t timestamp;    /**< Time when sync completed */
} timemachine_ntp_sync_t;

/**
 * @brief Display event IDs
 */
typedef enum {
    RENDER_SCENE,  /**< Render scene on display */
} display_event_id_t;

#ifdef __cplusplus
}
#endif

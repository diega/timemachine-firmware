#pragma once

#include "esp_event.h"
#include "scene.h"
#include <time.h>


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
    PANEL_DATE,           /**< Date panel */
} panel_id_t;

/**
 * @brief Timemachine event IDs
 */
typedef enum {
    NTP_SYNCED,           /**< NTP sync completed */
    NETWORK_CONNECTING,   /**< Network connection in progress */
    NETWORK_CONNECTED,    /**< Network connected successfully */
    NETWORK_FAILED,       /**< Network connection failed */
    INPUT_TAP,            /**< Touch input detected (short tap < 200ms) */
    INPUT_LONG_PRESS,     /**< Touch long press detected (≥ 200ms) */
    INPUT_RELEASE,        /**< Touch released after long press */
    PANEL_ACTIVATED,      /**< Panel was activated */
    PANEL_DEACTIVATED,    /**< Panel was deactivated */
    PANEL_SKIP_REQUESTED, /**< Panel requests to be skipped (no data available) */
    NETWORK_CONFIG_CHANGED,  /**< Network configuration changed */
    CLOCK_CONFIG_CHANGED,    /**< Clock configuration changed */
    NTP_CONFIG_CHANGED,      /**< NTP configuration changed */
    LANGUAGE_CHANGED,        /**< Language setting changed */
} timemachine_event_id_t;

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


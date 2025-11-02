/**
 * @file weather.h
 * @brief Weather data fetching from OpenWeather API
 *
 * Fetches current weather data including temperature and conditions
 * from OpenWeather API. Requires WiFi connection and API key configuration.
 */

#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Weather condition codes (simplified from OpenWeather)
 */
typedef enum {
    WEATHER_CLEAR = 0,        /**< Clear sky */
    WEATHER_CLOUDS,           /**< Cloudy */
    WEATHER_RAIN,             /**< Rain */
    WEATHER_SNOW,             /**< Snow */
    WEATHER_THUNDERSTORM,     /**< Thunderstorm */
    WEATHER_UNKNOWN,          /**< Unknown/unavailable */
} weather_condition_t;

/**
 * @brief Weather data structure
 */
typedef struct {
    float temperature;              /**< Temperature in Celsius */
    weather_condition_t condition;  /**< Weather condition */
    bool valid;                     /**< Data is valid */
} weather_data_t;

/**
 * @brief Weather configuration
 */
typedef struct {
    char api_key[64];          /**< OpenWeather API key */
    char location[64];         /**< City name or coordinates (lat,lon) */
    uint32_t update_interval;  /**< Update interval in seconds */
} weather_config_t;

/**
 * @brief Initialize weather component
 *
 * Starts periodic weather updates from OpenWeather API.
 * Requires network connection to function.
 *
 * @param config Weather configuration
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t weather_init(const weather_config_t *config);

/**
 * @brief Deinitialize weather component
 */
void weather_deinit(void);

/**
 * @brief Get current weather data
 *
 * @param data Pointer to store weather data
 * @return ESP_OK on success, ESP_ERR_INVALID_STATE if not initialized
 */
esp_err_t weather_get_data(weather_data_t *data);

/**
 * @brief Update weather configuration
 *
 * @param config New configuration
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t weather_update_config(const weather_config_t *config);

/**
 * @brief Manually trigger weather update
 *
 * @return ESP_OK if update started, error code otherwise
 */
esp_err_t weather_force_update(void);

/**
 * @file i18n.h
 * @brief Internationalization (i18n) support for Time Machine
 *
 * Provides multi-language support for user-facing strings like
 * day names, month names, etc.
 */

#pragma once

#include <stdint.h>
#include "esp_err.h"

/**
 * @brief Supported languages
 */
typedef enum {
    LANG_EN,  /**< English */
    LANG_ES,  /**< Spanish (Español) */
} language_t;

/**
 * @brief Initialize i18n system
 *
 * @param default_lang Default language to use
 * @return ESP_OK on success
 */
esp_err_t i18n_init(language_t default_lang);

/**
 * @brief Deinitialize i18n system
 */
void i18n_deinit(void);

/**
 * @brief Get localized day name (3 letters)
 *
 * @param day_of_week Day of week (0=Sunday, 6=Saturday)
 * @return Localized day name abbreviation
 */
const char* i18n_get_day_name(int day_of_week);

/**
 * @brief Get localized month name (3 letters)
 *
 * @param month Month (0=January, 11=December)
 * @return Localized month name abbreviation
 */
const char* i18n_get_month_name(int month);

/**
 * @brief Set current language
 *
 * @param lang Language to use
 */
void i18n_set_language(language_t lang);

/**
 * @brief Get current language
 *
 * @return Current language
 */
language_t i18n_get_language(void);

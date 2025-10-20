/**
 * @file mfactory_font.h
 * @brief mFactory font adapted from ESPTimeCast for ESP-IDF
 *
 * Original font from: https://github.com/mfactory-osaka/ESPTimeCast
 * Adapted from MD_MAX72XX format to esp-idf-lib/max7219 format
 *
 * Font format:
 * - Each character starts with a width byte (number of columns)
 * - Followed by column data bytes (one byte per column, LSB = top pixel)
 * - Total 256 characters (ASCII 0-255)
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Character descriptor
 */
typedef struct {
    uint8_t width;              // Number of columns (0-20)
    const uint8_t *data;        // Pointer to column data
} mfactory_char_t;

/**
 * @brief Get character descriptor from mFactory font
 *
 * @param ch ASCII character code (0-255)
 * @return Pointer to character descriptor, or NULL if invalid
 */
const mfactory_char_t* mfactory_get_char(uint8_t ch);

#ifdef __cplusplus
}
#endif

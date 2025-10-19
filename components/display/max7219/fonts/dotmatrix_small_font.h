/**
 * @file dotmatrix_small_font.h
 * @brief Small dot matrix style font (3x7) for LED matrix display
 *
 * Compact dot matrix LED display font with:
 * - 3 columns wide (plus 1 column spacing)
 * - 7 rows tall
 * - Retro LED/LCD display aesthetic
 *
 * Font format:
 * - Fixed-width bitmap font (3 columns per character + 1 space)
 * - Each column is one byte where LSB = top pixel, MSB = bottom pixel
 * - Character data stored in packed arrays with separate width/offset tables
 * - Supports letters (A-Z, a-z), numbers and symbols use default font
 */

#pragma once

#include "font.h"

/**
 * @brief Get character descriptor from small dot matrix font
 *
 * @param ch ASCII character code (0-255)
 * @return Pointer to character descriptor
 */
const font_char_t* dotmatrix_small_font_get_char(uint8_t ch);
const font_char_t* dotmatrix_small_font_get_char_last(uint8_t ch);

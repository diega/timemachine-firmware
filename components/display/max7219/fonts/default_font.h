/**
 * @file default_font.h
 * @brief Default bitmap font for LED matrix display
 *
 * Original font from: https://github.com/mfactory-osaka/ESPTimeCast
 * Adapted from MD_MAX72XX format to esp-idf-lib/max7219 format
 *
 * Font format:
 * - Variable-width bitmap font (characters can be 0-20 columns wide)
 * - Each column is one byte where LSB = top pixel, MSB = bottom pixel
 * - Character data stored in packed arrays with separate width/offset tables
 * - Supports full ASCII range (0-255)
 */

#pragma once

#include "font.h"

/**
 * @brief Get character descriptor from default font
 *
 * @param ch ASCII character code (0-255)
 * @return Pointer to character descriptor
 */
const font_char_t* default_font_get_char(uint8_t ch);


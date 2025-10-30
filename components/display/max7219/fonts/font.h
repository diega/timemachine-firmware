/**
 * @file font.h
 * @brief Font management system for MAX7219 LED matrix displays
 *
 * WHY FONTS ARE IN THIS DIRECTORY:
 * =================================
 * These fonts are specifically designed for the MAX7219 LED matrix hardware.
 * They use a column-based bitmap format where:
 * - Each character is a series of vertical columns (8 pixels tall)
 * - Each column is one byte (bit 0 = top pixel, bit 7 = bottom pixel)
 * - Characters have variable width (0-20 columns)
 *
 * This format is optimized for the MAX7219's column-based addressing and
 * is NOT portable to other display types (e.g., OLED, TFT, etc.).
 *
 * If we had display-agnostic fonts, they would live in a shared location.
 * But since these are tightly coupled to MAX7219 hardware characteristics,
 * they belong here in the MAX7219 driver directory.
 */

#pragma once

#include <stdint.h>

/**
 * @brief Character descriptor
 *
 * Describes a single character's bitmap data in MAX7219 column format
 */
typedef struct {
    uint8_t width;              /**< Number of columns (0-20) */
    const uint8_t *data;        /**< Pointer to column data (width bytes) */
} font_char_t;

/**
 * @brief Font descriptor
 *
 * Represents a complete font with metadata and character lookup function
 */
typedef struct font_s {
    const char *name;           /**< Human-readable font name */
    /**
     * @brief Get character descriptor from font
     * @param ch ASCII character code (0-255)
     * @return Pointer to character descriptor (never NULL)
     */
    const font_char_t* (*get_char)(uint8_t ch);
    /**
     * @brief Get character descriptor for last character in text
     * @param ch ASCII character code (0-255)
     * @return Pointer to character descriptor without trailing spacing (never NULL)
     */
    const font_char_t* (*get_char_last)(uint8_t ch);
} font_t;

/**
 * @brief Default font (compact, variable-width)
 *
 * This is the original font adapted from ESPTimeCast.
 * Good for general purpose text display with efficient space usage.
 */
extern const font_t font_default;

/**
 * @brief MD_MAX72XX library font (standard 5x7 fixed-width)
 *
 * This is the standard font from the MD_MAX72XX Arduino library.
 * Provides better readability with consistent character spacing.
 * Based on: https://github.com/MajicDesigns/MD_MAX72XX
 */
extern const font_t font_md_max72xx;

/**
 * @brief Dot matrix LED display font (5x7 with top line "roof")
 *
 * Classic dot matrix / LED display style font with a distinctive
 * horizontal line at the top of each letter (like an inverted underline).
 * Letters only (A-Z, a-z), numbers and symbols fall back to default font.
 * Creates a retro LED clock/display aesthetic.
 */
extern const font_t font_dotmatrix;

/**
 * @brief Small dot matrix LED display font (3x7 with top line "roof")
 *
 * Compact version of the dot matrix font for space-constrained displays.
 * Same techito style but only 3 columns wide per letter.
 * Letters only (A-Z, a-z), numbers and symbols fall back to default font.
 */
extern const font_t font_dotmatrix_small;

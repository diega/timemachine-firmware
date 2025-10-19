/**
 * @file md_max72xx_font.h
 * @brief MD_MAX72XX library font for LED matrix display
 *
 * Original font from: https://github.com/MajicDesigns/MD_MAX72XX
 * Copyright (C) 2012-14 Marco Colli. All rights reserved.
 * Licensed under LGPL 2.1
 *
 * This is the standard 5x7 font from the MD_MAX72XX Arduino library.
 * Converted to match the format used by this project.
 *
 * Font characteristics:
 * - Variable-width bitmap font
 * - Each column is one byte where LSB = top pixel, MSB = bottom pixel
 * - Supports ASCII range 0-255
 * - Generally more readable than the compact default font
 */

#pragma once

#include "font.h"

/**
 * @brief Get character descriptor from MD_MAX72XX font
 *
 * @param ch ASCII character code (0-255)
 * @return Pointer to character descriptor
 */
const font_char_t* md_max72xx_font_get_char(uint8_t ch);

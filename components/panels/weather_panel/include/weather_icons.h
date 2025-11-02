/**
 * @file weather_icons.h
 * @brief Weather icons for MAX7219 display
 *
 * 8x8 pixel icons for weather conditions designed for LED matrix displays
 */

#pragma once

#include <stdint.h>

// Icon bitmaps (8 columns x 8 rows each)
// Each byte represents a column, bit 0 = top, bit 7 = bottom

// Clear sky - sun icon
static const uint8_t weather_icon_clear[] = {
    0b00100100,  // . . # . . # . .
    0b00011000,  // . . . # # . . .
    0b01111110,  // . # # # # # # .
    0b11111111,  // # # # # # # # #
    0b11111111,  // # # # # # # # #
    0b01111110,  // . # # # # # # .
    0b00011000,  // . . . # # . . .
    0b00100100,  // . . # . . # . .
};

// Clouds - layered cloud icon
static const uint8_t weather_icon_clouds[] = {
    0b00000000,  // . . . . . . . .
    0b00011100,  // . . . # # # . .
    0b00111110,  // . . # # # # # .
    0b01111111,  // . # # # # # # #
    0b11111111,  // # # # # # # # #
    0b11111111,  // # # # # # # # #
    0b01111110,  // . # # # # # # .
    0b00000000,  // . . . . . . . .
};

// Rain - cloud with drops
static const uint8_t weather_icon_rain[] = {
    0b00011100,  // . . . # # # . .
    0b00111110,  // . . # # # # # .
    0b01111111,  // . # # # # # # #
    0b11111111,  // # # # # # # # #
    0b00100100,  // . . # . . # . .
    0b01000010,  // . # . . . . # .
    0b00100100,  // . . # . . # . .
    0b01000010,  // . # . . . . # .
};

// Snow - cloud with snowflakes
static const uint8_t weather_icon_snow[] = {
    0b00011100,  // . . . # # # . .
    0b00111110,  // . . # # # # # .
    0b01111111,  // . # # # # # # #
    0b11111111,  // # # # # # # # #
    0b00101010,  // . . # . # . # .
    0b00010100,  // . . . # . # . .
    0b00101010,  // . . # . # . # .
    0b00010100,  // . . . # . # . .
};

// Thunderstorm - cloud with lightning
static const uint8_t weather_icon_thunder[] = {
    0b00011100,  // . . . # # # . .
    0b00111110,  // . . # # # # # .
    0b01111111,  // . # # # # # # #
    0b11111111,  // # # # # # # # #
    0b00011000,  // . . . # # . . .
    0b00001100,  // . . . . # # . .
    0b00000110,  // . . . . . # # .
    0b00000010,  // . . . . . . # .
};

/**
 * @brief Get weather icon bitmap data
 *
 * @param condition Weather condition
 * @return Pointer to 8-byte icon bitmap, or NULL if no icon available
 */
static inline const uint8_t* weather_icon_get(int condition) {
    switch (condition) {
        case 0: // WEATHER_CLEAR
            return weather_icon_clear;
        case 1: // WEATHER_CLOUDS
            return weather_icon_clouds;
        case 2: // WEATHER_RAIN
            return weather_icon_rain;
        case 3: // WEATHER_SNOW
            return weather_icon_snow;
        case 4: // WEATHER_THUNDERSTORM
            return weather_icon_thunder;
        default:
            return NULL;
    }
}

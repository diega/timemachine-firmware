#pragma once

#include <stdint.h>


/**
 * @brief WiFi signal bars animation frames
 *
 * 3 frames showing WiFi signal bars growing from 1 to 3 bars.
 * Each frame is 8x8 pixels.
 */

// Frame 1: Small bar (left)
// Each byte is a column, bit 0 = top, bit 7 = bottom
static const uint8_t wifi_bar_1[] = {
    0xC0,  // 11000000 - Short bar (2 pixels tall)
    0xC0,  // 11000000
    0x00,  // Gap
    0x00,
    0x00,
    0x00,
    0x00,
    0x00
};

// Frame 2: Small + Medium bars
static const uint8_t wifi_bar_2[] = {
    0xC0,  // 11000000 - Short bar
    0xC0,  // 11000000
    0x00,  // Gap
    0xF0,  // 11110000 - Medium bar (4 pixels tall)
    0xF0,  // 11110000
    0x00,  // Gap
    0x00,
    0x00
};

// Frame 3: Small + Medium + Large bars
static const uint8_t wifi_bar_3[] = {
    0xC0,  // 11000000 - Short bar
    0xC0,  // 11000000
    0x00,  // Gap
    0xF0,  // 11110000 - Medium bar
    0xF0,  // 11110000
    0x00,  // Gap
    0xFC,  // 11111100 - Large bar (6 pixels tall)
    0xFC   // 11111100
};

// Array of frame pointers for animation
static const uint8_t *wifi_animation_frames[] = {
    wifi_bar_1,
    wifi_bar_2,
    wifi_bar_3
};

#define WIFI_ANIMATION_FRAME_COUNT 3
#define WIFI_ANIMATION_FRAME_DELAY_MS 300


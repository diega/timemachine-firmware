#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief WiFi signal bars animation frames
 *
 * 3 frames showing WiFi signal bars growing from 1 to 3 bars.
 * Each frame is 8x8 pixels.
 */

// Frame 1: Small bar (bottom left)
static const uint8_t wifi_bar_1[] = {
    0xE0,  // 11100000
    0xE0,  // 11100000
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00
};

// Frame 2: Small + Medium bars
static const uint8_t wifi_bar_2[] = {
    0xE0,  // 11100000
    0xE0,  // 11100000
    0x00,
    0xFC,  // 11111100
    0xFC,  // 11111100
    0x00,
    0x00,
    0x00
};

// Frame 3: Small + Medium + Large bars
static const uint8_t wifi_bar_3[] = {
    0xE0,  // 11100000
    0xE0,  // 11100000
    0x00,
    0xFC,  // 11111100
    0xFC,  // 11111100
    0x00,
    0xFF,  // 11111111
    0xFF   // 11111111
};

// Array of frame pointers for animation
static const uint8_t *wifi_animation_frames[] = {
    wifi_bar_1,
    wifi_bar_2,
    wifi_bar_3
};

#define WIFI_ANIMATION_FRAME_COUNT 3
#define WIFI_ANIMATION_FRAME_DELAY_MS 300

#ifdef __cplusplus
}
#endif

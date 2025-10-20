#pragma once

#include "display.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MAX7219 LED Matrix display driver
 *
 * Outputs time to 8x8 LED matrix controlled by MAX7219
 */
extern const display_driver_t display_max7219_driver;

#ifdef __cplusplus
}
#endif

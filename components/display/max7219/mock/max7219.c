/**
 * @file max7219_mock.c
 * @brief Mock implementation of MAX7219 driver for console testing
 *
 * This mock provides the same API as the real max7219 driver but renders
 * the display output to the console instead of driving real hardware.
 */

#include "max7219.h"  // The real header from managed_components
#include <stdio.h>
#include <string.h>

// Mock display buffer (4 cascaded devices)
static uint64_t s_display_buffer[4] = {0};
static bool s_initialized = false;

// Console rendering
static void print_display(void)
{
    printf("\n");
    printf("+----------------------------------------------------------------+\n");

    // Print each row (0-7, top to bottom)
    for (int row = 0; row < 8; row++) {
        printf("|");

        // Print each column (0-31, left to right)
        for (int col = 0; col < 32; col++) {
            int device = col / 8;
            int device_col = col % 8;

            // Extract bit for this row from the column byte
            uint8_t column_byte = (s_display_buffer[device] >> (device_col * 8)) & 0xFF;
            bool pixel_on = (column_byte & (1 << row)) != 0;

            printf("%s", pixel_on ? "##" : "  ");
        }

        printf("|\n");
    }

    printf("+----------------------------------------------------------------+\n");
    fflush(stdout);
}

// Mock implementations
esp_err_t max7219_init_desc(max7219_t *dev, spi_host_device_t host, uint32_t clock_speed_hz, gpio_num_t cs_pin)
{
    (void)host;
    (void)clock_speed_hz;
    (void)cs_pin;

    if (dev == NULL) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t max7219_free_desc(max7219_t *dev)
{
    (void)dev;
    return ESP_OK;
}

esp_err_t max7219_init(max7219_t *dev)
{
    if (dev == NULL) {
        return ESP_FAIL;
    }

    memset(s_display_buffer, 0, sizeof(s_display_buffer));
    s_initialized = true;

    printf("+================================================================+\n");
    printf("|         MAX7219 Mock Display (4 cascaded, 32x8)               |\n");
    printf("+================================================================+\n");
    print_display();

    return ESP_OK;
}

esp_err_t max7219_set_brightness(max7219_t *dev, uint8_t value)
{
    (void)dev;
    (void)value;
    return ESP_OK;
}

esp_err_t max7219_clear(max7219_t *dev)
{
    (void)dev;

    if (!s_initialized) {
        return ESP_FAIL;
    }

    memset(s_display_buffer, 0, sizeof(s_display_buffer));
    return ESP_OK;
}

esp_err_t max7219_draw_image_8x8(max7219_t *dev, uint8_t pos, const void *image)
{
    (void)dev;

    if (!s_initialized || image == NULL || pos >= 4) {
        return ESP_FAIL;
    }

    // Copy the 64-bit image to our buffer
    const uint64_t *img = (const uint64_t *)image;
    s_display_buffer[pos] = *img;

    // Re-render to console
    printf("\033[2J\033[H");  // Clear screen
    printf("+================================================================+\n");
    printf("|         MAX7219 Mock Display (4 cascaded, 32x8)               |\n");
    printf("+================================================================+\n");
    print_display();

    return ESP_OK;
}

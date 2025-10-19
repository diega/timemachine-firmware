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

    // MAX7219 hardware expects data in row-scan mode:
    // Each of the 8 bytes in the uint64_t represents one row (byte 0 = row 0, etc.)
    // Each bit in a byte represents one column (bit 0 = leftmost)
    // The display is upside down, so we flip vertically

    // Print each row (7 to 0, bottom to top)
    for (int row = 7; row >= 0; row--) {
        printf("|");

        // Print each column (0-31, left to right)
        for (int col = 0; col < 32; col++) {
            int device = col / 8;
            int device_col = col % 8;

            uint64_t device_data = s_display_buffer[device];

            // Extract the byte for this row
            uint8_t row_byte = (device_data >> (row * 8)) & 0xFF;

            // Check the bit for this column
            bool pixel_on = (row_byte & (1 << device_col)) != 0;

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

    if (!s_initialized || image == NULL) {
        return ESP_FAIL;
    }

    // The pos parameter is now a column offset (0, 8, 16, 24)
    // Convert to device index (0-3)
    uint8_t device_idx = pos / 8;

    if (device_idx >= 4) {
        return ESP_FAIL;
    }

    // The driver sends devices in reverse order:
    // device 0 gets buffer[3], device 1 gets buffer[2], etc.
    // So we need to reverse the mapping here too
    uint8_t buffer_idx = 3 - device_idx;

    // Copy the 64-bit image to our buffer
    const uint64_t *img = (const uint64_t *)image;
    s_display_buffer[buffer_idx] = *img;

    // Re-render to console
    printf("\033[2J\033[H");  // Clear screen
    printf("+================================================================+\n");
    printf("|         MAX7219 Mock Display (4 cascaded, 32x8)               |\n");
    printf("+================================================================+\n");
    print_display();

    return ESP_OK;
}

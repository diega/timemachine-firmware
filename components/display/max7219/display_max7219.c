#include "display_max7219.h"
#include "fonts/font.h"
#include "fonts/default_font.h"
#include "fonts/md_max72xx_font.h"
#include "scene.h"
#include <string.h>
#include "esp_log.h"
#include "max7219.h"
#include "driver/spi_master.h"

static const char *TAG = "display_max7219";

// ============================================================================
// Hardware Configuration
// ============================================================================

// SPI pin definitions for ESP32-C3
#define MAX7219_PIN_CLK   6   // SPI Clock
#define MAX7219_PIN_MOSI  7   // SPI Data (DIN)
#define MAX7219_PIN_CS    10  // Chip Select

#define SPI_HOST         SPI2_HOST
#define MAX7219_CASCADE  4    // 4 cascaded MAX7219 devices (32 columns total)

// ============================================================================
// Private State
// ============================================================================

static max7219_t s_dev = {0};
static bool s_initialized = false;

// Display buffer for 4 cascaded devices (32 columns x 8 rows)
static uint64_t s_display_buffer[MAX7219_CASCADE] = {0};

// ============================================================================
// Private - Display Buffer Management
// ============================================================================

static void clear_display_buffer(void)
{
    memset(s_display_buffer, 0, sizeof(s_display_buffer));
}

static uint64_t transpose_8x8(uint64_t input)
{
    uint64_t output = 0;

    // Rotate 90 degrees counter-clockwise
    // New position: (row, col) -> (7-col, row)
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            // Get bit at (row, col) in input
            uint64_t bit = (input >> (row * 8 + col)) & 1;
            // Set bit at (7-col, row) in output (90° CCW rotation)
            int new_row = 7 - col;
            int new_col = row;
            output |= (bit << (new_row * 8 + new_col));
        }
    }

    return output;
}

static void set_column(int col, uint8_t data)
{
    if (col < 0 || col >= 32) {
        return;
    }

    // Determine which device (0-3)
    int device = col / 8;
    int device_col = col % 8;

    // Clear the column first
    s_display_buffer[device] &= ~(0xFFULL << (device_col * 8));

    // Set new data
    uint64_t column_data = (uint64_t)data << (device_col * 8);
    s_display_buffer[device] |= column_data;
}

static int get_text_width(const char *text, const font_t *font)
{
    // Use default font if none specified
    if (font == NULL) {
        font = &font_default;
    }

    int width = 0;
    const char *p = text;

    while (*p) {
        char ch = *p;
        char next_ch = *(p + 1);

        // Use get_char_last for the last character to exclude trailing spacing
        const font_char_t *char_desc;
        if (next_ch == '\0') {
            char_desc = font->get_char_last((uint8_t)ch);
        } else {
            char_desc = font->get_char((uint8_t)ch);
        }

        if (char_desc && char_desc->width > 0) {
            width += char_desc->width;
        }

        p++;
    }

    return width;
}

static void render_text_at(int x_offset, const char *text, const font_t *font)
{
    int x = x_offset;

    // Use default font if none specified
    if (font == NULL) {
        font = &font_default;
    }

    while (*text) {
        char ch = *text;
        char next_ch = *(text + 1);

        // Use get_char_last for the last character, get_char for others
        const font_char_t *char_desc;
        if (next_ch == '\0') {
            char_desc = font->get_char_last((uint8_t)ch);
        } else {
            char_desc = font->get_char((uint8_t)ch);
        }

        // Render the character
        if (char_desc != NULL && char_desc->width > 0 && char_desc->data != NULL) {
            for (int col = 0; col < char_desc->width; col++) {
                if (x + col >= 32) {
                    break;  // Off screen
                }
                if (x + col >= 0) {
                    set_column(x + col, char_desc->data[col]);
                }
            }
            x += char_desc->width;
        }

        text++;
    }
}

static void render_bitmap_at(int x_offset, const uint8_t *bitmap, int width, int height)
{
    if (bitmap == NULL || width <= 0 || height <= 0) {
        return;
    }

    // Render bitmap columns (assuming height is 8 for 8x8 bitmaps)
    for (int col = 0; col < width; col++) {
        if (x_offset + col >= 32) {
            break;  // Off screen
        }
        if (x_offset + col >= 0) {
            set_column(x_offset + col, bitmap[col]);
        }
    }
}

// ============================================================================
// Private - Scene Rendering Logic
// ============================================================================

static void render_text_centered(const char *text, const font_t *font)
{
    clear_display_buffer();

    // Calculate width and center on display (32 columns)
    int text_width = get_text_width(text, font);
    int x_offset = (32 - text_width) / 2;

    // Render centered text
    render_text_at(x_offset, text, font);
}

static void render_scene(const display_scene_t *scene)
{
    if (scene == NULL) {
        return;
    }

    clear_display_buffer();

    // Calculate total width of all elements
    int total_width = 0;
    for (int i = 0; i < scene->element_count; i++) {
        const scene_element_t *elem = &scene->elements[i];
        if (elem->type == SCENE_ELEMENT_TEXT) {
            total_width += get_text_width(elem->data.text.str, elem->data.text.font);
        } else if (elem->type == SCENE_ELEMENT_ANIMATION) {
            total_width += elem->data.animation.width;
        }

        // Add spacing between elements (except last)
        if (i < scene->element_count - 1) {
            total_width += 2;  // 2 column spacing between elements
        }
    }

    // Center the entire scene
    int x_offset = (32 - total_width) / 2;

    // Render each element
    for (int i = 0; i < scene->element_count; i++) {
        const scene_element_t *elem = &scene->elements[i];

        if (elem->type == SCENE_ELEMENT_TEXT) {
            render_text_at(x_offset, elem->data.text.str, elem->data.text.font);
            x_offset += get_text_width(elem->data.text.str, elem->data.text.font);
        } else if (elem->type == SCENE_ELEMENT_ANIMATION) {
            // Render first frame of animation
            if (elem->data.animation.frames != NULL && elem->data.animation.frame_count > 0) {
                render_bitmap_at(x_offset, elem->data.animation.frames[0],
                               elem->data.animation.width, elem->data.animation.height);
            }
            x_offset += elem->data.animation.width;
        }

        // Add spacing between elements
        if (i < scene->element_count - 1) {
            x_offset += 2;
        }
    }
}

// ============================================================================
// Driver Implementation
// ============================================================================

static esp_err_t max7219_driver_init(void)
{
    if (s_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing MAX7219 display driver");

    // Initialize SPI bus
    spi_bus_config_t buscfg = {
        .mosi_io_num = MAX7219_PIN_MOSI,
        .miso_io_num = -1,  // Not used
        .sclk_io_num = MAX7219_PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0,
    };

    esp_err_t ret = spi_bus_initialize(SPI_HOST, &buscfg, SPI_DMA_DISABLED);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        // ESP_ERR_INVALID_STATE means bus already initialized (OK)
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }

    // Initialize device descriptor
    ret = max7219_init_desc(&s_dev, SPI_HOST, MAX7219_MAX_CLOCK_SPEED_HZ, MAX7219_PIN_CS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init descriptor: %s", esp_err_to_name(ret));
        spi_bus_free(SPI_HOST);
        return ret;
    }

    // Configure cascade
    s_dev.cascade_size = MAX7219_CASCADE;
    s_dev.mirrored = false;

    // Initialize MAX7219
    ret = max7219_init(&s_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize MAX7219: %s", esp_err_to_name(ret));
        max7219_free_desc(&s_dev);
        return ret;
    }

    // Set brightness to medium
    ret = max7219_set_brightness(&s_dev, 8);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set brightness: %s", esp_err_to_name(ret));
    }

    // Clear display
    ret = max7219_clear(&s_dev);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to clear display: %s", esp_err_to_name(ret));
    }

    s_initialized = true;
    ESP_LOGI(TAG, "MAX7219 display initialized (cascade: %d)", MAX7219_CASCADE);

    return ESP_OK;
}

static void max7219_driver_render(const display_scene_t *scene)
{
    if (!s_initialized || scene == NULL) {
        return;
    }

    // Render scene to buffer
    if (scene->element_count > 0 && scene->elements != NULL) {
        // Render full scene with elements
        render_scene(scene);
    } else if (scene->fallback_text != NULL) {
        // Fallback to simple text rendering (use default font)
        render_text_centered(scene->fallback_text, NULL);
    } else {
        return;
    }

    // Update physical displays (all cascaded devices)
    // Transpose each 8x8 block because MAX7219 expects data in row format
    // Send in reverse order because SPI cascade: first data sent ends up in last module
    for (int i = 0; i < MAX7219_CASCADE; i++) {
        int buffer_index = MAX7219_CASCADE - 1 - i;
        uint64_t transposed = transpose_8x8(s_display_buffer[buffer_index]);
        esp_err_t ret = max7219_draw_image_8x8(&s_dev, i * 8, &transposed);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to update device %d: %s", i, esp_err_to_name(ret));
        }
    }
}

static void max7219_driver_deinit(void)
{
    if (!s_initialized) {
        return;
    }

    // Clear display
    max7219_clear(&s_dev);

    // Free resources
    max7219_free_desc(&s_dev);

    s_initialized = false;
    ESP_LOGI(TAG, "MAX7219 display deinitialized");
}

// ============================================================================
// Driver Export
// ============================================================================

const display_driver_t display_max7219_driver = {
    .init = max7219_driver_init,
    .render = max7219_driver_render,
    .deinit = max7219_driver_deinit,
    .name = "max7219"
};

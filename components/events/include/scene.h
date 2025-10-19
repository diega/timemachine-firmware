#pragma once

#include <stdint.h>
#include <stdbool.h>

// Forward declaration - font_t is defined in display driver
typedef struct font_s font_t;

/**
 * @brief Scene element types
 */
typedef enum {
    SCENE_ELEMENT_TEXT,       /**< Static text */
    SCENE_ELEMENT_ANIMATION,  /**< Frame-based animation */
} scene_element_type_t;

/**
 * @brief Text element configuration
 */
typedef struct {
    const char *str;          /**< Text string to display */
    const font_t *font;       /**< Font to use (NULL = use default font) */
} scene_text_t;

/**
 * @brief Animation element configuration
 */
typedef struct {
    uint8_t frame_count;      /**< Number of frames in animation */
    uint32_t frame_delay_ms;  /**< Delay between frames in milliseconds */
    const uint8_t **frames;   /**< Array of frame bitmaps (8 bytes each for 8x8) */
    uint8_t width;            /**< Width of each frame in pixels */
    uint8_t height;           /**< Height of each frame in pixels (always 8) */
} scene_animation_t;

/**
 * @brief Scene element
 */
typedef struct {
    scene_element_type_t type;  /**< Element type */
    union {
        scene_text_t text;         /**< Text element data */
        scene_animation_t animation; /**< Animation element data */
    } data;
} scene_element_t;

/**
 * @brief Display scene
 *
 * A scene is a composition of elements. Simple drivers can use fallback_text,
 * while advanced drivers can render each element individually.
 */
typedef struct {
    uint8_t element_count;           /**< Number of elements in scene */
    const scene_element_t *elements; /**< Array of scene elements */
    const char *fallback_text;       /**< Simple text for basic drivers */
} display_scene_t;


#pragma once

#include "esp_err.h"
#include "scene.h"


/**
 * @brief Display driver interface
 *
 * Drivers handle rendering display scenes.
 * They listen to RENDER_SCENE events.
 */
typedef struct {
    esp_err_t (*init)(void);
    void (*render)(const display_scene_t *scene);
    void (*deinit)(void);
    const char *name;
} display_driver_t;

/**
 * @brief Initialize display
 *
 * Initializes the selected driver and registers event handlers
 * to listen for DISPLAY_EVENT_RENDER_TEXT events.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t display_init(void);

/**
 * @brief Deinitialize display and unregister event handlers
 */
void display_deinit(void);


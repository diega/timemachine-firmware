#include "display_console.h"
#include "display_scene.h"
#include <stdio.h>
#include "esp_log.h"

static const char *TAG = "display_console";

// ============================================================================
// Driver Implementation
// ============================================================================

static esp_err_t console_init(void)
{
    ESP_LOGI(TAG, "Console display initialized");
    return ESP_OK;
}

static void console_render(const display_scene_t *scene)
{
    if (scene == NULL) {
        return;
    }

    // Simple console driver just uses fallback text
    if (scene->fallback_text != NULL) {
        printf("Display: %s\n", scene->fallback_text);
    }
}

static void console_deinit(void)
{
    ESP_LOGI(TAG, "Console display deinitialized");
}

// ============================================================================
// Driver Export
// ============================================================================

const display_driver_t display_console_driver = {
    .init = console_init,
    .render = console_render,
    .deinit = console_deinit,
    .name = "console"
};

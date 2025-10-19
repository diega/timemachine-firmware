#include "display.h"
#include "display_max7219.h"
#include "timemachine_events.h"
#include "esp_log.h"
#include "esp_event.h"

static const char *TAG = "display";

static const display_driver_t *s_driver = NULL;
static bool s_initialized = false;
static esp_event_handler_instance_t s_display_event_handler = NULL;

// Forward declarations
static void display_event_handler(void* arg, esp_event_base_t base,
                                   int32_t event_id, void* event_data);

// ============================================================================
// Public API - Lifecycle
// ============================================================================

esp_err_t display_init(void)
{
    if (s_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    s_driver = &display_max7219_driver;

    ESP_LOGI(TAG, "Using driver: %s", s_driver->name);

    // Initialize driver
    esp_err_t err = s_driver->init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize driver");
        return err;
    }

    // Register event handler for display events
    err = esp_event_handler_instance_register(
        DISPLAY_EVENT,
        RENDER_SCENE,
        display_event_handler,
        NULL,
        &s_display_event_handler
    );

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register event handler");
        s_driver->deinit();
        return err;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "Display initialized with %s driver", s_driver->name);

    return ESP_OK;
}

void display_deinit(void)
{
    if (!s_initialized) {
        return;
    }

    // Unregister event handler
    if (s_display_event_handler != NULL) {
        esp_event_handler_instance_unregister(
            DISPLAY_EVENT,
            RENDER_SCENE,
            s_display_event_handler
        );
        s_display_event_handler = NULL;
    }

    // Deinitialize driver
    if (s_driver && s_driver->deinit) {
        s_driver->deinit();
    }

    s_initialized = false;

    ESP_LOGI(TAG, "Display deinitialized");
}

// ============================================================================
// Private - Event Handlers
// ============================================================================

static void display_event_handler(void* arg, esp_event_base_t base,
                                   int32_t event_id, void* event_data)
{
    if (event_id == RENDER_SCENE && s_driver && s_driver->render) {
        display_scene_t *scene = (display_scene_t *)event_data;

        if (scene) {
            s_driver->render(scene);
        }
    }
}

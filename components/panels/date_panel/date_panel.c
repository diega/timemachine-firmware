#include "date_panel.h"
#include "timemachine_events.h"
#include "panel_manager.h"
#include "display.h"
#include "fonts/font.h"
#include "i18n.h"
#include "esp_log.h"
#include "esp_event.h"
#include <stdio.h>
#include <time.h>

static const char *TAG = "date_panel";

static bool s_initialized = false;
static esp_event_handler_instance_t s_panel_activated_handler = NULL;
static esp_event_handler_instance_t s_panel_deactivated_handler = NULL;

// Forward declarations
static void render_date(void);
static void panel_activated_handler(void* arg, esp_event_base_t base,
                                   int32_t event_id, void* event_data);
static void panel_deactivated_handler(void* arg, esp_event_base_t base,
                                     int32_t event_id, void* event_data);

// ============================================================================
// Public API
// ============================================================================

esp_err_t date_panel_init(void)
{
    if (s_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing date panel...");

    // Register PANEL_ACTIVATED handler
    esp_err_t err = esp_event_handler_instance_register(
        TIMEMACHINE_EVENT,
        PANEL_ACTIVATED,
        panel_activated_handler,
        NULL,
        &s_panel_activated_handler
    );
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register PANEL_ACTIVATED handler");
        date_panel_deinit();
        return err;
    }

    // Register PANEL_DEACTIVATED handler
    err = esp_event_handler_instance_register(
        TIMEMACHINE_EVENT,
        PANEL_DEACTIVATED,
        panel_deactivated_handler,
        NULL,
        &s_panel_deactivated_handler
    );
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register PANEL_DEACTIVATED handler");
        date_panel_deinit();
        return err;
    }

    // Register panel with panel manager
    panel_info_t panel_info = {
        .id = PANEL_DATE,
        .name = "Date"
    };
    err = panel_manager_register_panel(&panel_info);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register panel");
        date_panel_deinit();
        return err;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "Date panel initialized");

    return ESP_OK;
}

void date_panel_deinit(void)
{
    if (!s_initialized) {
        return;
    }

    ESP_LOGI(TAG, "Deinitializing date panel...");

    // Unregister all event handlers
    if (s_panel_deactivated_handler != NULL) {
        esp_event_handler_instance_unregister(
            TIMEMACHINE_EVENT,
            PANEL_DEACTIVATED,
            s_panel_deactivated_handler
        );
        s_panel_deactivated_handler = NULL;
    }

    if (s_panel_activated_handler != NULL) {
        esp_event_handler_instance_unregister(
            TIMEMACHINE_EVENT,
            PANEL_ACTIVATED,
            s_panel_activated_handler
        );
        s_panel_activated_handler = NULL;
    }

    s_initialized = false;
    ESP_LOGI(TAG, "Date panel deinitialized");
}

// ============================================================================
// Private - Event Handlers
// ============================================================================

static void panel_activated_handler(void* arg, esp_event_base_t base,
                                   int32_t event_id, void* event_data)
{
    panel_id_t *panel_id = (panel_id_t *)event_data;

    if (*panel_id == PANEL_DATE) {
        ESP_LOGI(TAG, "Date panel activated");
        render_date();
    }
}

static void panel_deactivated_handler(void* arg, esp_event_base_t base,
                                     int32_t event_id, void* event_data)
{
    panel_id_t *panel_id = (panel_id_t *)event_data;

    if (*panel_id == PANEL_DATE) {
        ESP_LOGI(TAG, "Date panel deactivated");
    }
}

static void render_date(void)
{
    // Get current time from system
    time_t now = time(NULL);

    // If time is before 2020, consider it not synced (same as ntp_sync_is_synced)
    if (now < 1577836800) {  // Jan 1, 2020
        ESP_LOGW(TAG, "No time data available (time=%ld), requesting panel skip", now);
        // Request to skip this panel
        esp_event_post(
            TIMEMACHINE_EVENT,
            PANEL_SKIP_REQUESTED,
            NULL,
            0,
            0
        );
        return;  // Invalid time (not synced yet)
    }

    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    struct tm *time = &timeinfo;

    // Format date as separate month and day strings
    static char month_str[4];  // "MMM\0" = max 4 chars
    static char day_str[3];    // "DD\0" = max 3 chars

    const char *month = i18n_get_month_name(time->tm_mon);
    int day = time->tm_mday;

    snprintf(month_str, sizeof(month_str), "%s", month);
    snprintf(day_str, sizeof(day_str), "%d", day);

    // Build scene with two text elements: month (dotmatrix) + day (default)
    static scene_element_t date_elements[2];

    // Month with dotmatrix font (for the techito)
    date_elements[0].type = SCENE_ELEMENT_TEXT;
    date_elements[0].data.text.str = month_str;
    date_elements[0].data.text.font = &font_dotmatrix;

    // Day with default font (numbers don't have techito)
    date_elements[1].type = SCENE_ELEMENT_TEXT;
    date_elements[1].data.text.str = day_str;
    date_elements[1].data.text.font = &font_default;

    static display_scene_t date_scene;
    date_scene.element_count = 2;
    date_scene.elements = date_elements;

    // Fallback text for simple displays
    static char fallback_str[8];
    snprintf(fallback_str, sizeof(fallback_str), "%s %d", month, day);
    date_scene.fallback_text = fallback_str;

    // Emit RENDER_SCENE
    esp_err_t err = esp_event_post(
        DISPLAY_EVENT,
        RENDER_SCENE,
        &date_scene,
        sizeof(display_scene_t),
        portMAX_DELAY
    );

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to post display event: %s", esp_err_to_name(err));
    }
}

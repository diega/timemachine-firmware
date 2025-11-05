#include "panel_manager.h"
#include "esp_log.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include <string.h>

static const char *TAG = "panel_manager";

// Maximum number of panels that can be registered
#define MAX_PANELS 8

static struct {
    bool initialized;
    panel_manager_config_t config;
    panel_info_t panels[MAX_PANELS];
    uint8_t panel_count;
    uint8_t active_panel_idx;
    uint16_t inactivity_counter;
    TimerHandle_t inactivity_timer;
    esp_event_handler_instance_t input_touch_handler;
} s_state = {0};

// Forward declarations
static esp_err_t activate_panel(panel_id_t panel_id);
static esp_err_t deactivate_panel(panel_id_t panel_id);
static esp_err_t next_panel(void);
static void inactivity_timer_callback(TimerHandle_t xTimer);
static void input_touch_handler(void* arg, esp_event_base_t base,
                                int32_t event_id, void* event_data);

// ============================================================================
// Public API
// ============================================================================

esp_err_t panel_manager_init(const panel_manager_config_t *config)
{
    if (config == NULL) {
        ESP_LOGE(TAG, "Invalid config");
        return ESP_ERR_INVALID_ARG;
    }

    if (s_state.initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    memset(&s_state, 0, sizeof(s_state));
    s_state.config = *config;
    s_state.panel_count = 0;
    s_state.active_panel_idx = 0;
    s_state.inactivity_counter = 0;

    // Create inactivity timer (1 second period)
    s_state.inactivity_timer = xTimerCreate(
        "inactivity",
        pdMS_TO_TICKS(1000),  // 1 second period
        pdTRUE,               // Auto-reload
        NULL,
        inactivity_timer_callback
    );
    if (s_state.inactivity_timer == NULL) {
        ESP_LOGE(TAG, "Failed to create inactivity timer");
        return ESP_ERR_NO_MEM;
    }

    // Start inactivity timer
    if (xTimerStart(s_state.inactivity_timer, 0) != pdPASS) {
        ESP_LOGE(TAG, "Failed to start inactivity timer");
        xTimerDelete(s_state.inactivity_timer, 0);
        s_state.inactivity_timer = NULL;
        return ESP_FAIL;
    }

    // Register INPUT_TAP handler for panel navigation
    esp_err_t err = esp_event_handler_instance_register(
        TIMEMACHINE_EVENT,
        INPUT_TAP,
        input_touch_handler,
        NULL,
        &s_state.input_touch_handler
    );
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register INPUT_TAP handler");
        panel_manager_deinit();
        return err;
    }

    s_state.initialized = true;
    ESP_LOGI(TAG, "Panel manager initialized (default: %d, timeout: %ds)",
             config->default_panel, config->inactivity_timeout_s);

    return ESP_OK;
}

void panel_manager_deinit(void)
{
    if (!s_state.initialized) {
        return;
    }

    // Stop and delete timer
    if (s_state.inactivity_timer != NULL) {
        xTimerStop(s_state.inactivity_timer, 0);
        xTimerDelete(s_state.inactivity_timer, 0);
        s_state.inactivity_timer = NULL;
    }

    // Unregister event handlers
    if (s_state.input_touch_handler != NULL) {
        esp_event_handler_instance_unregister(
            TIMEMACHINE_EVENT,
            INPUT_TAP,
            s_state.input_touch_handler
        );
        s_state.input_touch_handler = NULL;
    }
    s_state.initialized = false;
    ESP_LOGI(TAG, "Panel manager deinitialized");
}

esp_err_t panel_manager_register_panel(const panel_info_t *panel)
{
    if (!s_state.initialized) {
        ESP_LOGE(TAG, "Not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (panel == NULL) {
        ESP_LOGE(TAG, "Invalid panel");
        return ESP_ERR_INVALID_ARG;
    }

    if (s_state.panel_count >= MAX_PANELS) {
        ESP_LOGE(TAG, "Maximum number of panels reached");
        return ESP_ERR_NO_MEM;
    }

    // Check if panel already registered
    for (uint8_t i = 0; i < s_state.panel_count; i++) {
        if (s_state.panels[i].id == panel->id) {
            ESP_LOGW(TAG, "Panel %d already registered", panel->id);
            return ESP_OK;
        }
    }

    s_state.panels[s_state.panel_count] = *panel;
    s_state.panel_count++;

    ESP_LOGI(TAG, "Registered panel: %s (id=%d)", panel->name, panel->id);

    // If this is the default panel, activate it
    if (panel->id == s_state.config.default_panel) {
        // Find index of this panel
        for (uint8_t i = 0; i < s_state.panel_count; i++) {
            if (s_state.panels[i].id == panel->id) {
                s_state.active_panel_idx = i;
                activate_panel(panel->id);
                break;
            }
        }
    }

    return ESP_OK;
}

panel_id_t panel_manager_get_active(void)
{
    if (s_state.panel_count == 0) {
        return s_state.config.default_panel;
    }
    return s_state.panels[s_state.active_panel_idx].id;
}

// ============================================================================
// Private Functions
// ============================================================================

static esp_err_t activate_panel(panel_id_t panel_id)
{
    ESP_LOGI(TAG, "Activating panel %d", panel_id);

    s_state.inactivity_counter = 0;

    // Emit PANEL_ACTIVATED event
    esp_err_t err = esp_event_post(
        TIMEMACHINE_EVENT,
        PANEL_ACTIVATED,
        &panel_id,
        sizeof(panel_id_t),
        portMAX_DELAY
    );

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to post PANEL_ACTIVATED event");
        return err;
    }

    return ESP_OK;
}

static esp_err_t deactivate_panel(panel_id_t panel_id)
{
    ESP_LOGI(TAG, "Deactivating panel %d", panel_id);

    // Emit PANEL_DEACTIVATED event
    esp_err_t err = esp_event_post(
        TIMEMACHINE_EVENT,
        PANEL_DEACTIVATED,
        &panel_id,
        sizeof(panel_id_t),
        portMAX_DELAY
    );

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to post PANEL_DEACTIVATED event");
        return err;
    }

    return ESP_OK;
}

static esp_err_t next_panel(void)
{
    if (s_state.panel_count == 0) {
        ESP_LOGW(TAG, "No panels registered");
        return ESP_ERR_INVALID_STATE;
    }

    panel_id_t current_panel_id = s_state.panels[s_state.active_panel_idx].id;

    // Deactivate current panel
    deactivate_panel(current_panel_id);

    // Calculate next panel index (circular)
    s_state.active_panel_idx = (s_state.active_panel_idx + 1) % s_state.panel_count;
    panel_id_t next_panel_id = s_state.panels[s_state.active_panel_idx].id;

    // Activate next panel
    activate_panel(next_panel_id);

    return ESP_OK;
}

static void input_touch_handler(void* arg, esp_event_base_t base,
                                int32_t event_id, void* event_data)
{
    ESP_LOGI(TAG, "Touch detected - switching to next panel");
    next_panel();
}

static void inactivity_timer_callback(TimerHandle_t xTimer)
{
    if (!s_state.initialized) {
        return;
    }

    // Increment inactivity counter
    s_state.inactivity_counter++;

    // Check if we need to return to default panel
    if (s_state.inactivity_counter >= s_state.config.inactivity_timeout_s) {
        if (s_state.panel_count == 0) {
            s_state.inactivity_counter = 0;
            return;
        }

        panel_id_t current_panel_id = s_state.panels[s_state.active_panel_idx].id;

        if (current_panel_id != s_state.config.default_panel) {
            ESP_LOGI(TAG, "Inactivity timeout - returning to default panel");

            // Find default panel index
            for (uint8_t i = 0; i < s_state.panel_count; i++) {
                if (s_state.panels[i].id == s_state.config.default_panel) {
                    deactivate_panel(current_panel_id);
                    s_state.active_panel_idx = i;
                    activate_panel(s_state.config.default_panel);
                    break;
                }
            }
        }

        // Reset counter
        s_state.inactivity_counter = 0;
    }
}

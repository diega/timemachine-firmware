#include "wifi_animation.h"
#include "wifi_bars.h"
#include "timemachine_events.h"
#include "display.h"
#include "fonts/font.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_timer.h"
#include <stdio.h>

static const char *TAG = "wifi_animation";

static bool s_initialized = false;
static bool s_animating = false;
static uint8_t s_current_frame = 0;
static esp_event_handler_instance_t s_network_connecting_handler = NULL;
static esp_event_handler_instance_t s_network_connected_handler = NULL;
static esp_event_handler_instance_t s_network_failed_handler = NULL;
static esp_timer_handle_t s_animation_timer = NULL;

#define ANIMATION_INTERVAL_MS 500  // Update every 500ms

// Forward declarations
static void network_connecting_handler(void* arg, esp_event_base_t base,
                                       int32_t event_id, void* event_data);
static void network_connected_handler(void* arg, esp_event_base_t base,
                                      int32_t event_id, void* event_data);
static void network_failed_handler(void* arg, esp_event_base_t base,
                                   int32_t event_id, void* event_data);
static void animation_timer_callback(void* arg);
static void start_animation(void);
static void stop_animation(void);
static void update_animation(void);

// ============================================================================
// Public API
// ============================================================================

esp_err_t wifi_animation_init(void)
{
    if (s_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing WiFi animation...");

    // Register NETWORK_CONNECTING handler
    esp_err_t err = esp_event_handler_instance_register(
        TIMEMACHINE_EVENT,
        NETWORK_CONNECTING,
        network_connecting_handler,
        NULL,
        &s_network_connecting_handler
    );
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register NETWORK_CONNECTING handler");
        wifi_animation_deinit();
        return err;
    }

    // Register NETWORK_CONNECTED handler
    err = esp_event_handler_instance_register(
        TIMEMACHINE_EVENT,
        NETWORK_CONNECTED,
        network_connected_handler,
        NULL,
        &s_network_connected_handler
    );
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register NETWORK_CONNECTED handler");
        wifi_animation_deinit();
        return err;
    }

    // Register NETWORK_FAILED handler
    err = esp_event_handler_instance_register(
        TIMEMACHINE_EVENT,
        NETWORK_FAILED,
        network_failed_handler,
        NULL,
        &s_network_failed_handler
    );
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register NETWORK_FAILED handler");
        wifi_animation_deinit();
        return err;
    }

    // Create animation timer
    esp_timer_create_args_t timer_args = {
        .callback = animation_timer_callback,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "wifi_anim"
    };
    err = esp_timer_create(&timer_args, &s_animation_timer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create animation timer");
        wifi_animation_deinit();
        return err;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "WiFi animation initialized");

    return ESP_OK;
}

void wifi_animation_deinit(void)
{
    if (s_initialized) {
        ESP_LOGI(TAG, "Deinitializing WiFi animation...");
        stop_animation();
    }

    // Clean up timer
    if (s_animation_timer != NULL) {
        esp_timer_stop(s_animation_timer);
        esp_timer_delete(s_animation_timer);
        s_animation_timer = NULL;
    }

    if (s_network_failed_handler != NULL) {
        esp_event_handler_instance_unregister(
            TIMEMACHINE_EVENT,
            NETWORK_FAILED,
            s_network_failed_handler
        );
        s_network_failed_handler = NULL;
    }

    if (s_network_connected_handler != NULL) {
        esp_event_handler_instance_unregister(
            TIMEMACHINE_EVENT,
            NETWORK_CONNECTED,
            s_network_connected_handler
        );
        s_network_connected_handler = NULL;
    }

    if (s_network_connecting_handler != NULL) {
        esp_event_handler_instance_unregister(
            TIMEMACHINE_EVENT,
            NETWORK_CONNECTING,
            s_network_connecting_handler
        );
        s_network_connecting_handler = NULL;
    }

    s_initialized = false;
}

// ============================================================================
// Private - Event Handlers
// ============================================================================

static void network_connecting_handler(void* arg, esp_event_base_t base,
                                       int32_t event_id, void* event_data)
{
    ESP_LOGI(TAG, "Network connecting - starting animation");
    start_animation();
}

static void network_connected_handler(void* arg, esp_event_base_t base,
                                      int32_t event_id, void* event_data)
{
    ESP_LOGI(TAG, "Network connected - stopping animation");
    stop_animation();
}

static void network_failed_handler(void* arg, esp_event_base_t base,
                                   int32_t event_id, void* event_data)
{
    ESP_LOGI(TAG, "Network failed - stopping animation");
    stop_animation();
}

static void animation_timer_callback(void* arg)
{
    if (!s_animating) {
        return;
    }

    update_animation();
}

static void update_animation(void)
{
    // Build scene with WiFi text + current animation frame
    static scene_element_t elements[2];
    static display_scene_t scene;
    static char fallback_text[16];

    ESP_LOGI(TAG, "Rendering frame %d/%d (ptr=%p)",
             s_current_frame, WIFI_ANIMATION_FRAME_COUNT,
             wifi_animation_frames[s_current_frame]);

    // Element 1: "WiFi" text (using md_max72xx font)
    elements[0].type = SCENE_ELEMENT_TEXT;
    elements[0].data.text.str = "WiFi";
    elements[0].data.text.font = &font_md_max72xx;

    // Element 2: WiFi bars bitmap (current frame)
    elements[1].type = SCENE_ELEMENT_ANIMATION;
    elements[1].data.animation.frame_count = 1;
    elements[1].data.animation.frame_delay_ms = 0;
    elements[1].data.animation.frames = &wifi_animation_frames[s_current_frame];
    elements[1].data.animation.width = 8;
    elements[1].data.animation.height = 8;

    // Debug: Print the first few bytes of current frame
    ESP_LOGI(TAG, "Frame data: %02x %02x %02x %02x %02x %02x %02x %02x",
             wifi_animation_frames[s_current_frame][0],
             wifi_animation_frames[s_current_frame][1],
             wifi_animation_frames[s_current_frame][2],
             wifi_animation_frames[s_current_frame][3],
             wifi_animation_frames[s_current_frame][4],
             wifi_animation_frames[s_current_frame][5],
             wifi_animation_frames[s_current_frame][6],
             wifi_animation_frames[s_current_frame][7]);

    // Fallback text for simple displays
    snprintf(fallback_text, sizeof(fallback_text), "WiFi%.*s",
             s_current_frame + 1, "...");

    scene.element_count = 2;
    scene.elements = elements;
    scene.fallback_text = fallback_text;

    // Emit RENDER_SCENE event
    esp_err_t err = esp_event_post(
        DISPLAY_EVENT,
        RENDER_SCENE,
        &scene,
        sizeof(display_scene_t),
        0
    );

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to post display event: %s", esp_err_to_name(err));
    }

    // Advance to next frame
    uint8_t old_frame = s_current_frame;
    s_current_frame = (s_current_frame + 1) % WIFI_ANIMATION_FRAME_COUNT;
    ESP_LOGI(TAG, "Advanced frame: %d -> %d", old_frame, s_current_frame);
}

// ============================================================================
// Private - Animation Control
// ============================================================================

static void start_animation(void)
{
    if (s_animating) {
        return;
    }

    s_current_frame = 0;
    s_animating = true;

    // Mostrar primer frame inmediatamente
    update_animation();

    // Start timer for subsequent frames
    if (s_animation_timer != NULL) {
        esp_timer_start_periodic(s_animation_timer, ANIMATION_INTERVAL_MS * 1000); // microseconds
    }
}

static void stop_animation(void)
{
    if (!s_animating) {
        return;
    }

    s_animating = false;
    s_current_frame = 0;

    // Stop timer
    if (s_animation_timer != NULL) {
        esp_timer_stop(s_animation_timer);
    }
}

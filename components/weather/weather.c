/**
 * @file weather.c
 * @brief Weather component implementation
 */

#include "weather.h"
#include "timemachine_events.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

static const char *TAG = "weather";

#define OPENWEATHER_API_URL "https://api.openweathermap.org/data/2.5/weather"
#define HTTP_RESPONSE_BUFFER_SIZE 2048

static struct {
    bool initialized;
    weather_config_t config;
    weather_data_t current_data;
    TimerHandle_t update_timer;
    char response_buffer[HTTP_RESPONSE_BUFFER_SIZE];
    size_t response_len;
    esp_event_handler_instance_t network_connected_handler;
    TaskHandle_t fetch_task_handle;
    bool fetch_requested;
} s_state = {0};

// Forward declarations
static void update_timer_callback(TimerHandle_t timer);
static esp_err_t fetch_weather_data(void);
static esp_err_t parse_weather_response(const char *json_str);
static weather_condition_t map_weather_condition(int owm_code);
static void network_connected_handler(void* arg, esp_event_base_t base,
                                       int32_t event_id, void* event_data);
static esp_err_t http_event_handler(esp_http_client_event_t *evt);
static void fetch_task(void *pvParameters);

// ============================================================================
// Public API
// ============================================================================

esp_err_t weather_init(const weather_config_t *config)
{
    if (config == NULL) {
        ESP_LOGE(TAG, "Invalid config");
        return ESP_ERR_INVALID_ARG;
    }

    if (s_state.initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    // Validate config
    if (strlen(config->api_key) == 0 || strlen(config->location) == 0) {
        ESP_LOGE(TAG, "API key and location are required");
        return ESP_ERR_INVALID_ARG;
    }

    s_state.config = *config;
    s_state.current_data.valid = false;
    s_state.response_len = 0;

    // Create update timer
    s_state.update_timer = xTimerCreate(
        "weather_update",
        pdMS_TO_TICKS(config->update_interval * 1000),
        pdTRUE,  // Auto-reload
        NULL,
        update_timer_callback
    );

    if (s_state.update_timer == NULL) {
        ESP_LOGE(TAG, "Failed to create update timer");
        return ESP_ERR_NO_MEM;
    }

    // Register for network connected events to trigger first update
    esp_err_t err = esp_event_handler_instance_register(
        TIMEMACHINE_EVENT,
        NETWORK_CONNECTED,
        network_connected_handler,
        NULL,
        &s_state.network_connected_handler
    );
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register network handler");
        xTimerDelete(s_state.update_timer, 0);
        return err;
    }

    // Create fetch task with large stack for HTTPS/TLS
    BaseType_t ret = xTaskCreate(
        fetch_task,
        "weather_fetch",
        8192,  // Large stack for HTTPS
        NULL,
        5,     // Priority
        &s_state.fetch_task_handle
    );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create fetch task");
        xTimerDelete(s_state.update_timer, 0);
        esp_event_handler_instance_unregister(
            TIMEMACHINE_EVENT,
            NETWORK_CONNECTED,
            s_state.network_connected_handler
        );
        return ESP_FAIL;
    }

    s_state.initialized = true;
    ESP_LOGI(TAG, "Weather initialized (location: %s, interval: %lus)",
             config->location, config->update_interval);

    // Start timer to begin periodic updates
    xTimerStart(s_state.update_timer, 0);

    // Trigger immediate first fetch
    ESP_LOGI(TAG, "Triggering initial weather fetch");
    s_state.fetch_requested = true;
    xTaskNotifyGive(s_state.fetch_task_handle);

    return ESP_OK;
}

void weather_deinit(void)
{
    if (!s_state.initialized) {
        return;
    }

    // Delete fetch task
    if (s_state.fetch_task_handle != NULL) {
        vTaskDelete(s_state.fetch_task_handle);
        s_state.fetch_task_handle = NULL;
    }

    if (s_state.update_timer != NULL) {
        xTimerStop(s_state.update_timer, 0);
        xTimerDelete(s_state.update_timer, 0);
        s_state.update_timer = NULL;
    }

    if (s_state.network_connected_handler != NULL) {
        esp_event_handler_instance_unregister(
            TIMEMACHINE_EVENT,
            NETWORK_CONNECTED,
            s_state.network_connected_handler
        );
        s_state.network_connected_handler = NULL;
    }

    s_state.initialized = false;
    ESP_LOGI(TAG, "Weather deinitialized");
}

esp_err_t weather_get_data(weather_data_t *data)
{
    if (!s_state.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    *data = s_state.current_data;
    return ESP_OK;
}

esp_err_t weather_update_config(const weather_config_t *config)
{
    if (!s_state.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    s_state.config = *config;

    // Restart timer with new interval
    xTimerChangePeriod(s_state.update_timer,
                       pdMS_TO_TICKS(config->update_interval * 1000),
                       0);

    // Trigger immediate update
    return weather_force_update();
}

esp_err_t weather_force_update(void)
{
    if (!s_state.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    return fetch_weather_data();
}

// ============================================================================
// Private - HTTP and API
// ============================================================================

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            // Accumulate response data
            if (s_state.response_len + evt->data_len < HTTP_RESPONSE_BUFFER_SIZE) {
                memcpy(s_state.response_buffer + s_state.response_len,
                       evt->data, evt->data_len);
                s_state.response_len += evt->data_len;
                s_state.response_buffer[s_state.response_len] = '\0';
            } else {
                ESP_LOGW(TAG, "Response buffer overflow");
            }
            break;

        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG, "HTTP error");
            break;

        default:
            break;
    }
    return ESP_OK;
}

static esp_err_t fetch_weather_data(void)
{
    // Build API URL
    char url[256];

    // Check if location is numeric (city ID) or text (city name)
    bool is_numeric = true;
    for (const char *p = s_state.config.location; *p; p++) {
        if (*p < '0' || *p > '9') {
            is_numeric = false;
            break;
        }
    }

    // Use 'id' parameter for numeric IDs, 'q' for city names
    if (is_numeric) {
        snprintf(url, sizeof(url),
                 "%s?id=%s&appid=%s&units=metric",
                 OPENWEATHER_API_URL,
                 s_state.config.location,
                 s_state.config.api_key);
    } else {
        snprintf(url, sizeof(url),
                 "%s?q=%s&appid=%s&units=metric",
                 OPENWEATHER_API_URL,
                 s_state.config.location,
                 s_state.config.api_key);
    }

    ESP_LOGI(TAG, "Fetching weather data...");

    // Reset response buffer
    s_state.response_len = 0;
    memset(s_state.response_buffer, 0, HTTP_RESPONSE_BUFFER_SIZE);

    // Configure HTTP client
    esp_http_client_config_t http_config = {
        .url = url,
        .event_handler = http_event_handler,
        .timeout_ms = 10000,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .crt_bundle_attach = esp_crt_bundle_attach,  // Use certificate bundle
    };

    esp_http_client_handle_t client = esp_http_client_init(&http_config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_FAIL;
    }

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP Status = %d, content_length = %d",
                 status, (int)s_state.response_len);

        if (status == 200) {
            err = parse_weather_response(s_state.response_buffer);
        } else {
            ESP_LOGW(TAG, "HTTP request failed with status %d", status);
            err = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return err;
}

static esp_err_t parse_weather_response(const char *json_str)
{
    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON response");
        return ESP_FAIL;
    }

    // Extract temperature from main.temp
    cJSON *main = cJSON_GetObjectItem(root, "main");
    cJSON *temp = cJSON_GetObjectItem(main, "temp");

    if (!cJSON_IsNumber(temp)) {
        ESP_LOGE(TAG, "Temperature not found in response");
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    // Extract weather condition from weather[0].id
    cJSON *weather_array = cJSON_GetObjectItem(root, "weather");
    if (!cJSON_IsArray(weather_array) || cJSON_GetArraySize(weather_array) == 0) {
        ESP_LOGE(TAG, "Weather array not found in response");
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    cJSON *weather = cJSON_GetArrayItem(weather_array, 0);
    cJSON *weather_id = cJSON_GetObjectItem(weather, "id");

    if (!cJSON_IsNumber(weather_id)) {
        ESP_LOGE(TAG, "Weather ID not found in response");
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    // Update current data
    s_state.current_data.temperature = (float)temp->valuedouble;
    s_state.current_data.condition = map_weather_condition(weather_id->valueint);
    s_state.current_data.valid = true;

    ESP_LOGI(TAG, "Weather updated: %.1f°C, condition: %d",
             s_state.current_data.temperature,
             s_state.current_data.condition);

    cJSON_Delete(root);
    return ESP_OK;
}

static weather_condition_t map_weather_condition(int owm_code)
{
    // OpenWeather condition codes:
    // 2xx = Thunderstorm
    // 3xx = Drizzle (treat as rain)
    // 5xx = Rain
    // 6xx = Snow
    // 7xx = Atmosphere (fog, etc - treat as clouds)
    // 800 = Clear
    // 80x = Clouds

    if (owm_code >= 200 && owm_code < 300) {
        return WEATHER_THUNDERSTORM;
    } else if (owm_code >= 300 && owm_code < 600) {
        return WEATHER_RAIN;
    } else if (owm_code >= 600 && owm_code < 700) {
        return WEATHER_SNOW;
    } else if (owm_code == 800) {
        return WEATHER_CLEAR;
    } else if (owm_code >= 801 && owm_code < 900) {
        return WEATHER_CLOUDS;
    }

    return WEATHER_UNKNOWN;
}

// ============================================================================
// Private - Event Handlers
// ============================================================================

static void update_timer_callback(TimerHandle_t timer)
{
    // Request fetch from dedicated task
    s_state.fetch_requested = true;
    if (s_state.fetch_task_handle != NULL) {
        xTaskNotifyGive(s_state.fetch_task_handle);
    }
}

static void network_connected_handler(void* arg, esp_event_base_t base,
                                       int32_t event_id, void* event_data)
{
    ESP_LOGI(TAG, "Network connected, triggering weather update");
    // Request fetch from dedicated task
    s_state.fetch_requested = true;
    if (s_state.fetch_task_handle != NULL) {
        xTaskNotifyGive(s_state.fetch_task_handle);
    }
}

// ============================================================================
// Private - Fetch Task
// ============================================================================

static void fetch_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Weather fetch task started");

    while (1) {
        // Wait for notification
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (s_state.fetch_requested) {
            s_state.fetch_requested = false;
            fetch_weather_data();
        }
    }
}

#include "network.h"
#include "timemachine_events.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

static const char *TAG = "network";

#define NETWORK_CONNECTED_BIT BIT0
#define NETWORK_FAIL_BIT      BIT1

static EventGroupHandle_t s_network_event_group = NULL;
static network_config_t s_config = {0};
static int s_retry_num = 0;
static bool s_initialized = false;
static bool s_connected = false;
static esp_event_handler_instance_t s_config_changed_handler = NULL;

// Forward declarations
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data);
static void on_network_config_changed(void* arg, esp_event_base_t event_base,
                                       int32_t event_id, void* event_data);

// ============================================================================
// Public API
// ============================================================================

esp_err_t network_init(const network_config_t *config)
{
    if (config == NULL) {
        ESP_LOGE(TAG, "Invalid config");
        return ESP_ERR_INVALID_ARG;
    }

    if (s_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing network...");

    // Copy configuration
    s_config = *config;

    // Create event group
    s_network_event_group = xEventGroupCreate();
    if (s_network_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create event group");
        return ESP_ERR_NO_MEM;
    }

    // Initialize network interface
    ESP_ERROR_CHECK(esp_netif_init());

    // Create default WiFi station interface
    esp_netif_create_default_wifi_sta();

    // Initialize WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &wifi_event_handler,
        NULL,
        NULL
    ));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT,
        ESP_EVENT_ANY_ID,
        &wifi_event_handler,
        NULL,
        NULL
    ));

    // Register configuration change handler
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        TIMEMACHINE_EVENT,
        NETWORK_CONFIG_CHANGED,
        &on_network_config_changed,
        NULL,
        &s_config_changed_handler
    ));

    // Configure WiFi station
    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = s_config.wifi_authmode,
        },
    };

    // Copy SSID and password
    strncpy((char *)wifi_config.sta.ssid, s_config.wifi_ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, s_config.wifi_password, sizeof(wifi_config.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi initialization finished, connecting to %s...", s_config.wifi_ssid);

    s_initialized = true;

    // Connection happens asynchronously via event handlers
    return ESP_OK;
}

void network_deinit(void)
{
    if (!s_initialized) {
        return;
    }

    ESP_LOGI(TAG, "Deinitializing network...");

    // Unregister config change handler
    if (s_config_changed_handler != NULL) {
        esp_event_handler_instance_unregister(
            TIMEMACHINE_EVENT,
            NETWORK_CONFIG_CHANGED,
            s_config_changed_handler
        );
        s_config_changed_handler = NULL;
    }

    // Stop WiFi
    esp_wifi_stop();
    esp_wifi_deinit();

    // Clean up event group
    if (s_network_event_group) {
        vEventGroupDelete(s_network_event_group);
        s_network_event_group = NULL;
    }

    s_initialized = false;
    s_connected = false;

    ESP_LOGI(TAG, "Network deinitialized");
}

bool network_is_connected(void)
{
    return s_connected;
}

// ============================================================================
// Private - Event Handlers
// ============================================================================

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        // Emit connecting event
        esp_event_post(TIMEMACHINE_EVENT, NETWORK_CONNECTING, NULL, 0, 0);
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < s_config.max_retries) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retrying WiFi connection (%d/%d)", s_retry_num, s_config.max_retries);
        } else {
            xEventGroupSetBits(s_network_event_group, NETWORK_FAIL_BIT);
            s_connected = false;

            // Emit failure event
            esp_event_post(TIMEMACHINE_EVENT, NETWORK_FAILED, NULL, 0, 0);
            ESP_LOGE(TAG, "WiFi connection failed after %d retries", s_config.max_retries);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        s_connected = true;
        xEventGroupSetBits(s_network_event_group, NETWORK_CONNECTED_BIT);

        // Emit success event
        esp_event_post(TIMEMACHINE_EVENT, NETWORK_CONNECTED, NULL, 0, 0);
    }
}

static void on_network_config_changed(void* arg, esp_event_base_t event_base,
                                       int32_t event_id, void* event_data)
{
    ESP_LOGI(TAG, "<<< NETWORK_CONFIG_CHANGED event received >>>");

    if (event_data == NULL) {
        ESP_LOGE(TAG, "Event data is NULL!");
        return;
    }

    network_config_t *new_config = (network_config_t*)event_data;

    ESP_LOGI(TAG, "New network configuration:");
    ESP_LOGI(TAG, "  SSID: [%s]", new_config->wifi_ssid);
    ESP_LOGI(TAG, "  Password: [%s]", new_config->wifi_password);
    ESP_LOGI(TAG, "  Authmode: %d", new_config->wifi_authmode);
    ESP_LOGI(TAG, "  Max retries: %d", new_config->max_retries);

    // Update stored configuration
    s_config = *new_config;

    // Reset retry counter
    s_retry_num = 0;

    ESP_LOGI(TAG, "Disconnecting current WiFi...");
    // Disconnect current WiFi
    esp_err_t ret = esp_wifi_disconnect();
    ESP_LOGI(TAG, "Disconnect result: %s", esp_err_to_name(ret));

    // Reconfigure WiFi with new credentials
    wifi_config_t wifi_config = {0};
    wifi_config.sta.threshold.authmode = s_config.wifi_authmode;

    strncpy((char *)wifi_config.sta.ssid, s_config.wifi_ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, s_config.wifi_password, sizeof(wifi_config.sta.password) - 1);

    ESP_LOGI(TAG, "Setting new WiFi config...");
    ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    ESP_LOGI(TAG, "Set config result: %s", esp_err_to_name(ret));

    // Reconnect
    ESP_LOGI(TAG, "Attempting to connect to [%s]...", s_config.wifi_ssid);
    ret = esp_wifi_connect();
    ESP_LOGI(TAG, "Connect result: %s", esp_err_to_name(ret));
}

#include "ntp_sync.h"
#include "timemachine_events.h"
#include <string.h>
#include <time.h>
#include "esp_log.h"
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

static const char *TAG = "ntp_sync";

#define TIME_SYNCED_BIT BIT0

static ntp_sync_config_t s_config = {0};
static EventGroupHandle_t s_sync_event_group = NULL;
static TaskHandle_t s_sync_task_handle = NULL;
static bool s_initialized = false;
static bool s_synced = false;
static esp_event_handler_instance_t s_config_changed_handler = NULL;

// Forward declarations
static void time_sync_notification_cb(struct timeval *tv);
static void ntp_sync_task_loop(void *pvParameters);
static void on_ntp_config_changed(void* arg, esp_event_base_t event_base,
                                   int32_t event_id, void* event_data);

// ============================================================================
// Public API
// ============================================================================

esp_err_t ntp_sync_init(const ntp_sync_config_t *config, uint32_t timeout_sec)
{
    if (config == NULL) {
        ESP_LOGE(TAG, "Invalid config");
        return ESP_ERR_INVALID_ARG;
    }

    if (s_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing NTP sync...");

    // Copy configuration
    s_config.server1 = config->server1;
    s_config.server2 = config->server2;
    s_config.timezone = config->timezone;
    s_config.sync_interval_ms = (config->sync_interval_ms > 0) ? config->sync_interval_ms : 3600000;

    // Create event group for sync notification
    s_sync_event_group = xEventGroupCreate();
    if (s_sync_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create event group");
        return ESP_ERR_NO_MEM;
    }

    // Set timezone
    if (s_config.timezone) {
        setenv("TZ", s_config.timezone, 1);
        tzset();
        ESP_LOGI(TAG, "Timezone set to: %s", s_config.timezone);
    }

    // Configure SNTP
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);

    if (s_config.server1) {
        esp_sntp_setservername(0, s_config.server1);
        ESP_LOGI(TAG, "Primary NTP server: %s", s_config.server1);
    }

    if (s_config.server2) {
        esp_sntp_setservername(1, s_config.server2);
        ESP_LOGI(TAG, "Secondary NTP server: %s", s_config.server2);
    }

    // Set notification callback
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);

    // Start SNTP for initial sync with retries
    ESP_LOGI(TAG, "Starting initial NTP sync...");

    const int MAX_RETRIES = 3;
    bool sync_success = false;

    for (int retry = 0; retry < MAX_RETRIES && !sync_success; retry++) {
        if (retry > 0) {
            ESP_LOGW(TAG, "NTP sync attempt %d/%d...", retry + 1, MAX_RETRIES);
        }

        // Clear sync bit before starting
        xEventGroupClearBits(s_sync_event_group, TIME_SYNCED_BIT);

        // Start SNTP
        esp_sntp_init();

        ESP_LOGI(TAG, "Waiting for NTP response (timeout: %lu seconds)...", timeout_sec);

        // Wait for initial sync
        EventBits_t bits = xEventGroupWaitBits(
            s_sync_event_group,
            TIME_SYNCED_BIT,
            pdFALSE,
            pdFALSE,
            pdMS_TO_TICKS(timeout_sec * 1000)
        );

        // Stop SNTP after attempt
        esp_sntp_stop();

        if (bits & TIME_SYNCED_BIT) {
            sync_success = true;
            ESP_LOGI(TAG, "Initial NTP sync completed on attempt %d", retry + 1);
        } else {
            ESP_LOGW(TAG, "NTP sync timeout on attempt %d", retry + 1);

            // Wait a bit before retrying
            if (retry < MAX_RETRIES - 1) {
                ESP_LOGI(TAG, "Waiting 2 seconds before retry...");
                vTaskDelay(pdMS_TO_TICKS(2000));
            }
        }
    }

    if (!sync_success) {
        ESP_LOGE(TAG, "Initial NTP sync failed after %d attempts", MAX_RETRIES);
        vEventGroupDelete(s_sync_event_group);
        s_sync_event_group = NULL;
        return ESP_ERR_TIMEOUT;
    }

    ESP_LOGI(TAG, "Initial NTP sync completed");

    // Emit NTP synced event
    timemachine_ntp_sync_t sync_data = {
        .success = true,
        .timestamp = time(NULL)
    };
    esp_event_post(
        TIMEMACHINE_EVENT,
        NTP_SYNCED,
        &sync_data,
        sizeof(sync_data),
        0
    );

    // Stop SNTP temporarily (will be restarted by background task)
    esp_sntp_stop();

    // Start background sync task
    BaseType_t ret = xTaskCreate(
        ntp_sync_task_loop,
        "ntp_sync",
        4096,
        NULL,
        4,  // Priority
        &s_sync_task_handle
    );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create sync task");
        vEventGroupDelete(s_sync_event_group);
        s_sync_event_group = NULL;
        return ESP_FAIL;
    }

    // Register NTP_CONFIG_CHANGED handler
    esp_err_t err = esp_event_handler_instance_register(
        TIMEMACHINE_EVENT,
        NTP_CONFIG_CHANGED,
        on_ntp_config_changed,
        NULL,
        &s_config_changed_handler
    );
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register NTP_CONFIG_CHANGED handler");
        ntp_sync_deinit();
        return err;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "NTP sync initialized (interval: %lu ms)", s_config.sync_interval_ms);

    return ESP_OK;
}

bool ntp_sync_is_synced(void)
{
    if (!s_synced) {
        return false;
    }

    time_t now = 0;
    time(&now);

    // If time is before 2020, consider it not synced
    return (now > 1577836800); // Jan 1, 2020
}

void ntp_sync_deinit(void)
{
    if (!s_initialized) {
        return;
    }

    ESP_LOGI(TAG, "Deinitializing NTP sync...");

    // Unregister config change handler
    if (s_config_changed_handler != NULL) {
        esp_event_handler_instance_unregister(
            TIMEMACHINE_EVENT,
            NTP_CONFIG_CHANGED,
            s_config_changed_handler
        );
        s_config_changed_handler = NULL;
    }

    // Stop background task
    if (s_sync_task_handle != NULL) {
        vTaskDelete(s_sync_task_handle);
        s_sync_task_handle = NULL;
    }

    // Stop SNTP
    esp_sntp_stop();

    // Clean up event group
    if (s_sync_event_group) {
        vEventGroupDelete(s_sync_event_group);
        s_sync_event_group = NULL;
    }

    s_initialized = false;
    s_synced = false;

    ESP_LOGI(TAG, "NTP sync deinitialized");
}

// ============================================================================
// Private - Background Sync Task
// ============================================================================
// NOTE: We use a FreeRTOS task instead of a timer because this sync operation
// blocks for up to 30 seconds waiting for NTP response. Timer callbacks MUST NOT
// block as they run in the timer daemon task context, which would stall all other
// timers in the system. A dedicated task allows us to safely block without issues.

static void ntp_sync_task_loop(void *pvParameters)
{
    ESP_LOGI(TAG, "Background NTP sync task started (interval: %lu ms)", s_config.sync_interval_ms);

    while (1) {
        // Wait for sync interval
        vTaskDelay(pdMS_TO_TICKS(s_config.sync_interval_ms));

        ESP_LOGI(TAG, "Performing periodic NTP sync...");

        // Clear the sync bit
        xEventGroupClearBits(s_sync_event_group, TIME_SYNCED_BIT);

        // Start SNTP sync
        esp_sntp_init();

        // Wait for sync to complete (with timeout)
        EventBits_t bits = xEventGroupWaitBits(
            s_sync_event_group,
            TIME_SYNCED_BIT,
            pdFALSE,
            pdFALSE,
            pdMS_TO_TICKS(30000)  // 30 second timeout
        );

        // Stop SNTP after sync attempt
        esp_sntp_stop();

        if (bits & TIME_SYNCED_BIT) {
            ESP_LOGI(TAG, "Periodic NTP sync successful");

            // Publish sync event (optional, for logging/monitoring)
            timemachine_ntp_sync_t sync_data = {
                .success = true,
                .timestamp = time(NULL)
            };
            esp_event_post(
                TIMEMACHINE_EVENT,
                NTP_SYNCED,
                &sync_data,
                sizeof(sync_data),
                0
            );
        } else {
            ESP_LOGW(TAG, "Periodic NTP sync timeout");
        }
    }
}

// ============================================================================
// Private - Callbacks
// ============================================================================

static void time_sync_notification_cb(struct timeval *tv)
{
    // Validate timestamp - reject if before Jan 1, 2020 (1577836800)
    if (tv->tv_sec < 1577836800) {
        ESP_LOGW(TAG, "Rejected invalid NTP timestamp: %ld (too old)", tv->tv_sec);
        return;
    }

    ESP_LOGI(TAG, "Time synchronized! Unix time: %ld", tv->tv_sec);
    s_synced = true;

    if (s_sync_event_group) {
        xEventGroupSetBits(s_sync_event_group, TIME_SYNCED_BIT);
    }
}

static void on_ntp_config_changed(void* arg, esp_event_base_t event_base,
                                   int32_t event_id, void* event_data)
{
    ntp_sync_config_t *new_config = (ntp_sync_config_t*)event_data;

    ESP_LOGI(TAG, "NTP configuration changed");

    // Update stored configuration
    s_config.server1 = new_config->server1;
    s_config.server2 = new_config->server2;
    s_config.timezone = new_config->timezone;
    s_config.sync_interval_ms = (new_config->sync_interval_ms > 0) ?
                                 new_config->sync_interval_ms : 3600000;

    // Update timezone
    if (s_config.timezone) {
        setenv("TZ", s_config.timezone, 1);
        tzset();
        ESP_LOGI(TAG, "Timezone updated to: %s", s_config.timezone);
    }

    // Reconfigure SNTP servers
    esp_sntp_stop();

    if (s_config.server1) {
        esp_sntp_setservername(0, s_config.server1);
        ESP_LOGI(TAG, "Primary NTP server updated: %s", s_config.server1);
    }

    if (s_config.server2) {
        esp_sntp_setservername(1, s_config.server2);
        ESP_LOGI(TAG, "Secondary NTP server updated: %s", s_config.server2);
    }

    // Note: sync_interval_ms will be picked up by the background task on next iteration
    ESP_LOGI(TAG, "NTP sync interval updated to: %lu ms", s_config.sync_interval_ms);
}

/**
 * @file i18n.c
 * @brief Internationalization (i18n) implementation
 */

#include "i18n.h"
#include "timemachine_events.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include <stddef.h>

static const char *TAG = "i18n";

// Current language
static language_t s_current_language = LANG_EN;
static bool s_initialized = false;
static esp_event_handler_instance_t s_language_changed_handler = NULL;

// Forward declarations
static void on_language_changed(void* arg, esp_event_base_t event_base,
                                 int32_t event_id, void* event_data);

// Day names (3 letters) - Sunday to Saturday
static const char* DAY_NAMES_EN[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static const char* DAY_NAMES_ES[] = {
    "Dom", "Lun", "Mar", "Mie", "Jue", "Vie", "Sab"
};

// Month names (3 letters) - January to December
static const char* MONTH_NAMES_EN[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static const char* MONTH_NAMES_ES[] = {
    "Ene", "Feb", "Mar", "Abr", "May", "Jun",
    "Jul", "Ago", "Sep", "Oct", "Nov", "Dic"
};

esp_err_t i18n_init(language_t default_lang)
{
    if (s_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    s_current_language = default_lang;
    ESP_LOGI(TAG, "Initializing i18n with language: %d", default_lang);

    // Register LANGUAGE_CHANGED handler
    esp_err_t err = esp_event_handler_instance_register(
        TIMEMACHINE_EVENT,
        LANGUAGE_CHANGED,
        on_language_changed,
        NULL,
        &s_language_changed_handler
    );
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register LANGUAGE_CHANGED handler");
        return err;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "i18n initialized");

    return ESP_OK;
}

void i18n_deinit(void)
{
    if (!s_initialized) {
        return;
    }

    ESP_LOGI(TAG, "Deinitializing i18n...");

    // Unregister language change handler
    if (s_language_changed_handler != NULL) {
        esp_event_handler_instance_unregister(
            TIMEMACHINE_EVENT,
            LANGUAGE_CHANGED,
            s_language_changed_handler
        );
        s_language_changed_handler = NULL;
    }

    s_initialized = false;
    ESP_LOGI(TAG, "i18n deinitialized");
}

const char* i18n_get_day_name(int day_of_week)
{
    // Validate range
    if (day_of_week < 0 || day_of_week > 6) {
        return "???";
    }

    switch (s_current_language) {
        case LANG_ES:
            return DAY_NAMES_ES[day_of_week];
        case LANG_EN:
        default:
            return DAY_NAMES_EN[day_of_week];
    }
}

const char* i18n_get_month_name(int month)
{
    // Validate range
    if (month < 0 || month > 11) {
        return "???";
    }

    switch (s_current_language) {
        case LANG_ES:
            return MONTH_NAMES_ES[month];
        case LANG_EN:
        default:
            return MONTH_NAMES_EN[month];
    }
}

void i18n_set_language(language_t lang)
{
    s_current_language = lang;
}

language_t i18n_get_language(void)
{
    return s_current_language;
}

static void on_language_changed(void* arg, esp_event_base_t event_base,
                                 int32_t event_id, void* event_data)
{
    language_t *new_lang = (language_t*)event_data;

    ESP_LOGI(TAG, "Language changed to: %d", *new_lang);

    // Update current language
    s_current_language = *new_lang;

    // Display will update when panel renders next with new language
}

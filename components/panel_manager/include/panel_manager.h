#pragma once

#include "esp_err.h"
#include "timemachine_events.h"


/**
 * @brief Panel manager configuration
 */
typedef struct {
    panel_id_t default_panel;      /**< Default panel to show (typically PANEL_CLOCK) */
} panel_manager_config_t;

/**
 * @brief Panel registration data
 */
typedef struct {
    panel_id_t id;          /**< Unique panel ID */
    const char *name;       /**< Panel name for debugging */
} panel_info_t;

/**
 * @brief Initialize panel manager
 *
 * The panel manager coordinates which panel is currently active.
 * On initialization, it activates the default panel.
 *
 * @param config Configuration parameters
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t panel_manager_init(const panel_manager_config_t *config);

/**
 * @brief Deinitialize panel manager
 */
void panel_manager_deinit(void);

/**
 * @brief Register a panel with the manager
 *
 * Panels must register themselves during initialization.
 * If the registered panel is the default panel, it will be activated automatically.
 *
 * @param panel Panel information
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t panel_manager_register_panel(const panel_info_t *panel);

/**
 * @brief Get currently active panel ID
 *
 * @return Active panel ID
 */
panel_id_t panel_manager_get_active(void);


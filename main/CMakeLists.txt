idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES settings network ntp_sync display panel_manager touch_sensor clock_panel tick_task events nvs_flash i18n wifi_animation)

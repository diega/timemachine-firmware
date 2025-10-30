idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES settings network ntp_sync display panel_manager touch_sensor clock_panel date_panel tick_task events nvs_flash wifi_animation i18n)

idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES settings ble_config brightness_control network ntp_sync display panel_manager touch_sensor clock_panel date_panel weather_panel weather events nvs_flash wifi_animation i18n)

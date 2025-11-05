idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES settings network ntp_sync display panel_manager clock_panel events nvs_flash wifi_animation i18n)

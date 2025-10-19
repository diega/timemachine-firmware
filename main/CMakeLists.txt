idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES settings network ntp_sync display panel_manager clock_panel tick_task events nvs_flash i18n)

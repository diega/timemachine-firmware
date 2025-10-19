idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES network ntp_sync display panel_manager clock tick_task events nvs_flash)

idf_component_register(
    SRCS "weather.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_http_client esp_event json esp_https_ota events mbedtls
)

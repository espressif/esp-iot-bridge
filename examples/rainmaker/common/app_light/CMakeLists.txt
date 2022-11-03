idf_component_register(SRCS "app_light.c"
                       INCLUDE_DIRS "."
                       REQUIRES app_reset ws2812_led rmaker_common esp_rainmaker)

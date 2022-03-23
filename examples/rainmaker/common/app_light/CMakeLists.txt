idf_component_register(SRCS "app_light.c"
                       INCLUDE_DIRS "."
                       REQUIRES button app_reset ws2812_led rmaker_common esp_rainmaker)

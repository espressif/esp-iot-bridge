idf_component_register(SRCS "app_gateway.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES gateway app_rainmaker app_wifi)

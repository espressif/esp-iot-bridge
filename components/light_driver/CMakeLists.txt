idf_component_register(SRCS "light_driver.c" "light_nvs.c"
                       INCLUDE_DIRS "include"
                       REQUIRES "nvs_flash")
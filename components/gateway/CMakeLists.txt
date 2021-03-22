idf_component_register(SRC_DIRS "src"
                       INCLUDE_DIRS "include"
                       REQUIRES "utils" "json" "mqtt" "app_update" "esp_https_ota" "console" "fatfs" "esp_modem" "vendor_model" "example_init" "light_driver")
target_compile_options(${COMPONENT_LIB} PRIVATE -DLOG_LOCAL_LEVEL=ESP_LOG_VERBOSE)
idf_component_register(SRC_DIRS "src"
                       INCLUDE_DIRS "include"
                       REQUIRES "wifi_provisioning" "json" "mqtt" "app_update" "esp_https_ota" "console" "fatfs")
target_compile_options(${COMPONENT_LIB} PRIVATE -DLOG_LOCAL_LEVEL=ESP_LOG_VERBOSE)
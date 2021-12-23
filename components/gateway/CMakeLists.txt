if(CONFIG_IDF_TARGET_ESP32 OR CONFIG_IDF_TARGET_ESP32C3)
    set(srcs "src/board.c"
             "src/gateway_wifi.c"
             "src/led_pwm.c")
endif()

if(CONFIG_IDF_TARGET_ESP32)
    list(APPEND srcs "src/gateway_ble.c"
                     "src/gateway_eth.c"
                     "src/gateway_modem.c"
                     "src/gateway_netif_virtual.c"
                     "src/modem_board.c")
endif()

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "include"
                       REQUIRES "utils" "json" "mqtt" "app_update" "esp_https_ota" "console" "fatfs" "esp_modem" "vendor_model" "example_init" "light_driver")
target_compile_options(${COMPONENT_LIB} PRIVATE -DLOG_LOCAL_LEVEL=ESP_LOG_VERBOSE)
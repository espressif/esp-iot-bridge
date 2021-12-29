set(srcs "src/board.c"
         "src/gateway_wifi.c"
         "src/led_pwm.c")

if(CONFIG_IDF_TARGET_ESP32)
    list(APPEND srcs "src/gateway_eth.c"
                     "src/gateway_modem.c"
                     "src/gateway_netif_virtual.c"
                     "src/modem_board.c")
endif()

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "include"
                       REQUIRES "utils" "json" "mqtt" "app_update" "esp_https_ota" "console" "fatfs" "esp_modem" "light_driver")

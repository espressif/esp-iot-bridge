set(srcs "src/esp_modem.c"
         "src/esp_modem_dce_command_lib.c"
         "src/esp_modem_dce_common_commands.c"
         "src/esp_modem_dce.c"
         "src/esp_modem_netif.c"
         "src/esp_modem_recov_helper.c"
         "src/esp_sim800.c"
         "src/esp_sim7600.c"
         "src/esp_bg96.c")

set(include_dirs "include")
set(priv_include_dirs "private_include")
set(requires "driver" "esp_usbh_cdc" "esp_netif")

if(CONFIG_MODEM_LEGACY_API)
        list(APPEND srcs "src/esp_modem_compat.c")
        list(APPEND include_dirs "include_compat")
endif()

if(CONFIG_IDF_TARGET_ESP32 OR CONFIG_IDF_TARGET_ESP32C3)
        list(APPEND srcs "src/esp_modem_dte_uart.c")
elseif(CONFIG_IDF_TARGET_ESP32S2 OR CONFIG_IDF_TARGET_ESP32S3)
        list(APPEND srcs "src/esp_modem_dte_usb.c")
endif()

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "${include_dirs}"
                    PRIV_INCLUDE_DIRS "${priv_include_dirs}"
                    REQUIRES "${requires}")

set(srcs "src/gateway_common.c")
set(requires "")
set(include_dirs "include")
set(priv_includes "priv_inc")

if (CONFIG_GATEWAY_EXTERNAL_NETIF_STATION OR CONFIG_GATEWAY_DATA_FORWARDING_NETIF_SOFTAP)
    list(APPEND srcs "src/gateway_wifi.c")
endif()

if (CONFIG_SET_VENDOR_IE)
    list(APPEND srcs "src/gateway_vendor_ie.c")
endif()

if (CONFIG_GATEWAY_EXTERNAL_NETIF_MODEM)
    list(APPEND srcs "src/gateway_modem.c")
    if(CONFIG_GATEWAY_MODEM_USB)
        list(APPEND srcs "src/modem_board_usb.c")
    endif()
    if(CONFIG_GATEWAY_MODEM_CUSTOM_BOARD)
        list(APPEND srcs "src/modem_board_sim7600.c")
    endif()
endif()

if (CONFIG_GATEWAY_DATA_FORWARDING_NETIF_USB)
    list(APPEND srcs "priv_src/gateway_usb.c")
endif()

if (CONFIG_GATEWAY_DATA_FORWARDING_NETIF_SDIO)
    list(APPEND srcs "priv_src/gateway_sdio.c")
endif()

if (CONFIG_GATEWAY_DATA_FORWARDING_NETIF_SPI)
    list(APPEND srcs "priv_src/gateway_spi.c")
endif()

list(APPEND requires "network_adapter" "esp_modem" "tinyusb" "led")
idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "${include_dirs}"
                       PRIV_INCLUDE_DIRS "${priv_includes}"
                       REQUIRES "${requires}")

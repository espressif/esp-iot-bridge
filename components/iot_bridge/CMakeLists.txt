set(srcs "src/bridge_common.c")
set(requires "esp_eth" "esp_netif" "esp_wifi" "esp_timer" "esp_modem")
set(include_dirs "include" "include/priv_inc")

if (CONFIG_BRIDGE_EXTERNAL_NETIF_STATION OR CONFIG_BRIDGE_DATA_FORWARDING_NETIF_SOFTAP)
    list(APPEND srcs "src/bridge_wifi.c")
endif()

if (CONFIG_BRIDGE_EXTERNAL_NETIF_MODEM)
    list(APPEND srcs "src/bridge_modem.c")
endif()

if ("${IDF_TARGET}" STREQUAL "esp32s2" OR "${IDF_TARGET}" STREQUAL "esp32s3")
    list(APPEND requires "esp_modem_usb_dte")
endif()

if (DEFINED ENV{BRIDGE_PRIV})
    if ($ENV{BRIDGE_PRIV} STREQUAL "ON")
        list(APPEND requires "iot_bridge_priv")
    endif()
endif()

if (CONFIG_BRIDGE_DATA_FORWARDING_NETIF_SDIO OR CONFIG_BRIDGE_DATA_FORWARDING_NETIF_SPI)
    add_subdirectory(driver)
endif()

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "${include_dirs}"
                       REQUIRES "${requires}")

execute_process(COMMAND git rev-parse --short HEAD
            OUTPUT_VARIABLE SRC_COMMIT_ID
            OUTPUT_STRIP_TRAILING_WHITESPACE)

set_source_files_properties(SOURCE ${srcs}
                            PROPERTIES COMPILE_DEFINITIONS
                            "COMMIT_ID=\"${SRC_COMMIT_ID}\"")

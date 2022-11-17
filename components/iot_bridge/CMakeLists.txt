set(srcs "src/bridge_common.c")
set(requires "")
set(include_dirs "include")
set(priv_includes "priv_inc")
set(priv_srcs "")

if (CONFIG_BRIDGE_EXTERNAL_NETIF_STATION OR CONFIG_BRIDGE_DATA_FORWARDING_NETIF_SOFTAP)
    list(APPEND srcs "src/bridge_wifi.c")
endif()

if (CONFIG_BRIDGE_EXTERNAL_NETIF_MODEM)
    list(APPEND srcs "src/bridge_modem.c")
    if(CONFIG_BRIDGE_MODEM_USB)
        list(APPEND srcs "src/modem_board_usb.c")
    endif()
    if(CONFIG_BRIDGE_MODEM_CUSTOM_BOARD)
        list(APPEND srcs "src/modem_board_sim7600.c")
    endif()
endif()

if (CONFIG_BRIDGE_DATA_FORWARDING_NETIF_USB)
    list(APPEND priv_srcs "priv_src/bridge_usb.c")
endif()

if (CONFIG_BRIDGE_EXTERNAL_NETIF_ETHERNET OR CONFIG_BRIDGE_DATA_FORWARDING_NETIF_ETHERNET)
    list(APPEND priv_srcs "priv_src/bridge_eth.c")
endif()

if (CONFIG_BRIDGE_DATA_FORWARDING_NETIF_SDIO)
    list(APPEND priv_srcs "priv_src/bridge_sdio.c")
endif()

if (CONFIG_BRIDGE_DATA_FORWARDING_NETIF_SPI)
    list(APPEND priv_srcs "priv_src/bridge_spi.c")
endif()

if ("${IDF_TARGET}" STREQUAL "esp32s2" OR "${IDF_TARGET}" STREQUAL "esp32s3")
    list(APPEND requires "usb_device")
endif()

list(APPEND requires "network_adapter" "esp_modem" "led" "json" "esp_eth")

idf_component_register(SRCS "${srcs}" "${priv_srcs}"
                       INCLUDE_DIRS "${include_dirs}"
                       PRIV_INCLUDE_DIRS "${priv_includes}"
                       REQUIRES "${requires}")

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/priv_src")
    execute_process(COMMAND git rev-parse --short HEAD
                OUTPUT_VARIABLE PRIV_SRC_COMMIT_ID
                OUTPUT_STRIP_TRAILING_WHITESPACE
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/priv_src")
endif()

execute_process(COMMAND git rev-parse --short HEAD
            OUTPUT_VARIABLE SRC_COMMIT_ID
            OUTPUT_STRIP_TRAILING_WHITESPACE)

set_source_files_properties(SOURCE ${srcs}
                            PROPERTIES COMPILE_DEFINITIONS
                            "COMMIT_ID=\"${SRC_COMMIT_ID}\"")

set_source_files_properties(SOURCE ${priv_srcs}
                            PROPERTIES COMPILE_DEFINITIONS
                            "COMMIT_ID=\"${PRIV_SRC_COMMIT_ID}\"")
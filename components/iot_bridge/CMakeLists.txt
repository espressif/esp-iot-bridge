set(srcs "src/bridge_common.c" "src/customer_hook.c")
set(requires "esp_eth" "esp_netif" "esp_wifi" "esp_timer" "nvs_flash" "esp_modem")
set(include_dirs "include")

if (CONFIG_BRIDGE_EXTERNAL_NETIF_STATION OR CONFIG_BRIDGE_DATA_FORWARDING_NETIF_SOFTAP)
    list(APPEND srcs "src/bridge_wifi.c")
endif()

if (CONFIG_BRIDGE_DATA_FORWARDING_NETIF_USB)
    list(APPEND srcs "src/bridge_usb.c")
endif()

if (CONFIG_BRIDGE_ETHERNET_NETIF_ENABLE)
    list(APPEND srcs "src/bridge_eth.c")
endif()

if (CONFIG_BRIDGE_NETIF_ETHERNET_AUTO_WAN_OR_LAN)
    list(APPEND srcs "src/esp_eth_netif_glue_wrap.c")
endif()

if (CONFIG_BRIDGE_EXTERNAL_NETIF_SPI OR
    CONFIG_BRIDGE_EXTERNAL_NETIF_SDIO OR
    CONFIG_BRIDGE_DATA_FORWARDING_NETIF_SPI OR
    CONFIG_BRIDGE_DATA_FORWARDING_NETIF_SDIO)
    if (CONFIG_BRIDGE_EXTERNAL_NETIF_SPI OR CONFIG_BRIDGE_DATA_FORWARDING_NETIF_SPI)
        list(APPEND srcs "src/bridge_spi.c")
    endif()
    if (CONFIG_BRIDGE_EXTERNAL_NETIF_SDIO OR CONFIG_BRIDGE_DATA_FORWARDING_NETIF_SDIO)
        list(APPEND srcs "src/bridge_sdio.c")
    endif()
    add_subdirectory(drivers)
endif()

if (CONFIG_BRIDGE_EXTERNAL_NETIF_MODEM)
    list(APPEND srcs "src/bridge_modem.c")
endif()

if ("${IDF_TARGET}" STREQUAL "esp32s2" OR "${IDF_TARGET}" STREQUAL "esp32s3")
    list(APPEND requires "esp_modem_usb_dte")
    if("${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}.${IDF_VERSION_PATCH}" VERSION_GREATER_EQUAL "5.1.4")
        list(APPEND requires "esp_tinyusb")
    endif()
endif()

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "${include_dirs}"
                       REQUIRES "${requires}")

# Apply ESP-IDF Patch
execute_process(COMMAND
                git apply ${CMAKE_CURRENT_LIST_DIR}/patch/dhcp.patch
                WORKING_DIRECTORY $ENV{IDF_PATH}/components/lwip/lwip)
execute_process(COMMAND
                git apply ${CMAKE_CURRENT_LIST_DIR}/patch/dhcpserver.patch
                WORKING_DIRECTORY $ENV{IDF_PATH})
execute_process(COMMAND
                git apply ${CMAKE_CURRENT_LIST_DIR}/patch/ip4_forward.patch
                WORKING_DIRECTORY $ENV{IDF_PATH}/components/lwip/lwip/)
execute_process(COMMAND
                git apply ${CMAKE_CURRENT_LIST_DIR}/patch/ip4_napt.patch
                WORKING_DIRECTORY $ENV{IDF_PATH}/components/lwip/lwip/)

execute_process(COMMAND
                git rev-parse --short HEAD
                OUTPUT_VARIABLE SRC_COMMIT_ID
                OUTPUT_STRIP_TRAILING_WHITESPACE)

set_source_files_properties(SOURCE ${srcs}
                            PROPERTIES COMPILE_DEFINITIONS
                            "COMMIT_ID=\"${SRC_COMMIT_ID}\"")

target_compile_options(__idf_lwip PRIVATE
                       "-DIP_NAPT_PORTMAP=1"
                       "-DIP_PORTMAP_MAX=255"
                       "-DIP_NAPT_MAX=CONFIG_BRIDGE_IP_NAPT_SIZE"
                       "-DIP_NAPT_TIMEOUT_MS_TCP=CONFIG_BRIDGE_IP_NAPT_TIMEOUT_MS_TCP"
                       "-DIP_NAPT_ADD_FAILED_HOOK=custom_hook_ip4_napt_add_failed"
                       "-DESP_IDF_LWIP_HOOK_FILENAME=\"${CMAKE_CURRENT_SOURCE_DIR}/include/customer_hook.h\"")

idf_component_set_property(${COMPONENT_NAME} WHOLE_ARCHIVE TRUE)

include(package_manager)
cu_pkg_define_version(${CMAKE_CURRENT_LIST_DIR})

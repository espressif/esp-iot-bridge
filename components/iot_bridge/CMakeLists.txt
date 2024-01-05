set(srcs "src/bridge_common.c")
set(requires "esp_eth" "esp_netif" "esp_wifi" "esp_timer")
set(include_dirs "include")

if (CONFIG_BRIDGE_EXTERNAL_NETIF_STATION OR CONFIG_BRIDGE_DATA_FORWARDING_NETIF_SOFTAP)
    list(APPEND srcs "src/bridge_wifi.c")
endif()

if (CONFIG_BRIDGE_DATA_FORWARDING_NETIF_USB)
    list(APPEND srcs "src/bridge_usb.c")
endif()

if (CONFIG_BRIDGE_EXTERNAL_NETIF_ETHERNET OR CONFIG_BRIDGE_DATA_FORWARDING_NETIF_ETHERNET)
    list(APPEND srcs "src/bridge_eth.c")
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

if("${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}" VERSION_GREATER_EQUAL "4.4")
    list(APPEND requires "esp_modem")
    if (CONFIG_BRIDGE_EXTERNAL_NETIF_MODEM)
        list(APPEND srcs "src/bridge_modem.c")
    endif()
endif()

if ("${IDF_TARGET}" STREQUAL "esp32s2" OR "${IDF_TARGET}" STREQUAL "esp32s3")
    if("${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}" VERSION_GREATER_EQUAL "4.4")
        list(APPEND requires "esp_modem_usb_dte" "usb_device")
    endif()
endif()

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "${include_dirs}"
                       REQUIRES "${requires}")

if("${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}" VERSION_GREATER_EQUAL "5.0")
    execute_process(COMMAND
                    git apply ${CMAKE_CURRENT_LIST_DIR}/patch/ip4_forward.patch
                    WORKING_DIRECTORY $ENV{IDF_PATH}/components/lwip/lwip/)
endif()

execute_process(COMMAND
                git rev-parse --short HEAD
                OUTPUT_VARIABLE SRC_COMMIT_ID
                OUTPUT_STRIP_TRAILING_WHITESPACE)

set_source_files_properties(SOURCE ${srcs}
                            PROPERTIES COMPILE_DEFINITIONS
                            "COMMIT_ID=\"${SRC_COMMIT_ID}\"")

set(FALLBACK_DNS_SERVER "8.8.8.8")
target_compile_options(__idf_lwip PRIVATE 
                       "-DIP_PORTMAP_MAX=255"
                       "-DFALLBACK_DNS_SERVER_ADDRESS(p)=ipaddr_aton(\"${FALLBACK_DNS_SERVER}\",(p))")

include(package_manager)
cu_pkg_define_version(${CMAKE_CURRENT_LIST_DIR})

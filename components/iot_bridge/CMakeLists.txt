set(srcs "src/bridge_common.c" "src/bridge_lwip_hook.c")
set(requires "esp_eth" "esp_netif" "esp_wifi" "esp_timer" "nvs_flash" "esp_modem" "esp_driver_gpio")
set(include_dirs "include")

if("${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}.${IDF_VERSION_PATCH}" VERSION_GREATER_EQUAL "5.4.0")
    list(APPEND requires "esp_driver_gpio" "esp_driver_sdio")
endif()

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

execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/config)
execute_process(COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/include/esp_bridge_lwip_hook.h ${CMAKE_BINARY_DIR}/config/)

set_property(GLOBAL PROPERTY CUSTOMER_LWIP_HOOK_FILE_LISTS "${CMAKE_BINARY_DIR}/config/customer_lwip_hook.h")
get_property(hook_file GLOBAL PROPERTY CUSTOMER_LWIP_HOOK_FILE_LISTS)

if(NOT "${hook_file}" STREQUAL "")
    file(WRITE ${hook_file} "#include \"esp_bridge_lwip_hook.h\"\n")
endif()

# Apply ESP-IDF Patch from patches.list
# Include patch utilities for reusable functions
include(${CMAKE_CURRENT_LIST_DIR}/cmake/patch_utils.cmake)

# Apply patches using the utility function
apply_patches_from_list()

execute_process(COMMAND
                git rev-parse --short HEAD
                OUTPUT_VARIABLE SRC_COMMIT_ID
                OUTPUT_STRIP_TRAILING_WHITESPACE)

set_source_files_properties(SOURCE ${srcs}
                            PROPERTIES COMPILE_DEFINITIONS
                            "COMMIT_ID=\"${SRC_COMMIT_ID}\"")

target_compile_options(__idf_lwip PRIVATE
                       "-DESP_IDF_LWIP_HOOK_FILENAME=\"customer_lwip_hook.h\"")

idf_component_set_property(${COMPONENT_NAME} WHOLE_ARCHIVE TRUE)

include(package_manager)
cu_pkg_define_version(${CMAKE_CURRENT_LIST_DIR})

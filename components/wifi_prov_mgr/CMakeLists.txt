set(srcs)
set(OLD_PROV_MGR "${IDF_PATH}/components/wifi_provisioning/src/")

if(IDF_VERSION_MAJOR STREQUAL "4")
set(NEW_PROV_MGR "${CMAKE_CURRENT_LIST_DIR}/v4.4/manager.c")
elseif(IDF_VERSION_MAJOR STREQUAL "5")
set(NEW_PROV_MGR "${CMAKE_CURRENT_LIST_DIR}/v5.0/manager.c")
endif()

if (CONFIG_ESP_GATEWAY_PROV_TRANSPORT)
list(APPEND srcs "src/wifi_prov_mgr.c")
execute_process(COMMAND cp ${NEW_PROV_MGR} ${OLD_PROV_MGR} OUTPUT_VARIABLE info
        RESULT_VARIABLE exit_code
        OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "include"
                       REQUIRES "wifi_provisioning" "qrcode" "gateway")
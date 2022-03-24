set(OLD_PROV_MGR "${IDF_PATH}/components/wifi_provisioning/src/")
set(NEW_PROV_MGR "${CMAKE_CURRENT_LIST_DIR}/manager.c")
execute_process(COMMAND cp ${NEW_PROV_MGR} ${OLD_PROV_MGR} OUTPUT_VARIABLE info
        RESULT_VARIABLE exit_code
        OUTPUT_STRIP_TRAILING_WHITESPACE)

idf_component_register(SRCS "src/wifi_prov_mgr.c"
                       INCLUDE_DIRS "include"
                       REQUIRES "wifi_provisioning" "esp_qrcode")

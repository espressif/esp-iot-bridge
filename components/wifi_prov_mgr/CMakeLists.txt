set(srcs)
set(requires "wifi_provisioning" "qrcode" "esp_wifi")

if (CONFIG_ESP_BRIDGE_PROV_TRANSPORT)
    list(APPEND srcs "src/wifi_prov_mgr.c")
endif()

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "include"
                       REQUIRES "${requires}")
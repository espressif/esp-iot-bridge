set(srcs "src/wifi_prov_mgr.c")
set(requires "qrcode" "esp_wifi")

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "include"
                       REQUIRES "${requires}")

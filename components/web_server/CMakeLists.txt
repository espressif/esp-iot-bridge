set(require_components vfs esp_http_client esp_https_ota json freertos spiffs
    bootloader_support app_update wpa_supplicant spi_flash esp_http_server)

list(APPEND require_components iot_bridge)

set(embed_txt_files ./fs_image/index.html)

idf_component_register(SRC_DIRS "src"
                       INCLUDE_DIRS "include"
                       REQUIRES ${require_components}
                       EMBED_TXTFILES ${embed_txt_files})

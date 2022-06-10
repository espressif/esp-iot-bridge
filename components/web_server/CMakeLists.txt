set(require_components mqtt mdns esp_http_client esp_https_ota json freertos spiffs
    bootloader_support app_update wpa_supplicant spi_flash esp_http_server)

list(APPEND require_components gateway)

set(embed_txt_files ./fs_image/index.html)

idf_component_register(SRC_DIRS "src"
                       INCLUDE_DIRS "include"
                       REQUIRES ${require_components}
                       EMBED_TXTFILES ${embed_txt_files})

idf_build_get_property(target IDF_TARGET)

set(srcs)
set(include_dir)
set(requires)

if(target STREQUAL "esp32s2" OR target STREQUAL "esp32s3")
    list(APPEND srcs "esp_usbh_cdc.c")
    list(APPEND include_dir "include"
                            "${IDF_PATH}/components/usb/private_include")
    list(APPEND requires "usb" "esp_ringbuf")
endif()

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS ${include_dir}
                       REQUIRES ${requires})

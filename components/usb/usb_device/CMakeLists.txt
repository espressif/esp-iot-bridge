idf_build_get_property(target IDF_TARGET)

set(srcs)
set(includes_public)
set(includes_private)
set(compile_options)

if(target STREQUAL "esp32s2" OR target STREQUAL "esp32s3")

if(IDF_VERSION_MAJOR STREQUAL "4")
    set(requires "tinyusb")
elseif(IDF_VERSION_MAJOR STREQUAL "5")
    set(requires "esp_tinyusb")
endif()

if(CONFIG_TINYUSB)
    if(target STREQUAL "esp32s3")
        set(tusb_mcu "OPT_MCU_ESP32S3")
        set(tusb_family "esp32sx")
    elseif(target STREQUAL "esp32s2")
        set(tusb_mcu "OPT_MCU_ESP32S2")
        set(tusb_family "esp32sx")
    else()
        # CONFIG_TINYUSB dependency has been garanteed by Kconfig logic,
        # So it's not possible that cmake goes here
        message(FATAL_ERROR "TinyUSB is not support on ${target}.")
        return()
    endif()

    list(APPEND compile_options
        "-DCFG_TUSB_MCU=${tusb_mcu}"
        "-DCFG_TUSB_DEBUG=${CONFIG_TINYUSB_DEBUG_LEVEL}"
        )

    idf_component_get_property(freertos_component_dir freertos COMPONENT_DIR)

    list(APPEND includes_public
        "additions/include"
        "additions/include_private"
        "additions/tusb/src/lib/networking"
        "additions/tusb/src/class/dfu"
        "additions/tusb/src/class/bth"
        "additions/tusb/src/class/vendor"
        )

    # The FreeRTOS API include convention in tinyusb is different from esp-idf
    if(IDF_VERSION_MAJOR STREQUAL "4")
        list(APPEND srcs "additions/src/usbd_4.c")
        list(APPEND includes_public "${freertos_component_dir}/include/freertos")
    elseif(IDF_VERSION_MAJOR STREQUAL "5")
        list(APPEND includes_public
            "${freertos_component_dir}/esp_additions/include/freertos"
            "${freertos_component_dir}/FreeRTOS-Kernel/include/freertos"
        )
    endif()

    list(APPEND srcs
        "additions/src/descriptors_control.c"
        "additions/src/tinyusb.c"
        "additions/src/tusb_tasks.c"
        "additions/src/usb_descriptors.c"
        )

    # when no builtin class driver is enabled, an uint8_t data compared with `BUILTIN_DRIVER_COUNT` will always be false
    set_source_files_properties("tinyusb/src/device/usbd.c" PROPERTIES COMPILE_FLAGS "-Wno-type-limits")

    if(CONFIG_TINYUSB_CDCACM_ENABLED)
        list(APPEND srcs
            "additions/src/cdc.c"
            "additions/src/tusb_cdc_acm.c"
            "additions/src/tusb_console.c"
            "additions/src/vfs_tinyusb.c"
            )
    endif() # CONFIG_TINYUSB_CDCACM_ENABLED

    if(CONFIG_TINYUSB_HID_ENABLED)
        list(APPEND srcs "additions/src/tusb_hid.c")
    endif()

    if(CONFIG_TINYUSB_MSC_ENABLED)
        list(APPEND srcs "additions/src/tusb_msc.c")
    endif()

    if(CONFIG_TINYUSB_NET_ENABLED)
        list(APPEND srcs
            "additions/src/tusb_net.c"
            "additions/tusb/src/class/net/ecm_rndis_device.c"
            "additions/tusb/src/lib/networking/rndis_reports.c")
    endif()

    if(CONFIG_TINYUSB_BTH_ENABLED)
        list(APPEND srcs
            "additions/src/tusb_bth.c"
            "additions/tusb/src/class/bth/bth_device.c")
    endif()

    if(CONFIG_TINYUSB_DFU_ENABLED)
        list(APPEND srcs
            "additions/src/tusb_dfu.c"
            "additions/tusb/src/class/dfu/dfu_device.c")
    endif()

    if(CONFIG_TINYUSB_WEBUSB_ENABLED)
        list(APPEND srcs
            "additions/src/tusb_webusb.c"
            "additions/tusb/src/class/vendor/vendor_device.c")
    endif()
endif() # CONFIG_TINYUSB

endif()

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS ${includes_public}
                       PRIV_INCLUDE_DIRS ${includes_private}
                       PRIV_REQUIRES "vfs" "fatfs" "bt" "app_update"
                       REQUIRES "${requires}")

if(CONFIG_TINYUSB)
    target_compile_options(${COMPONENT_LIB} PRIVATE ${compile_options})
endif()

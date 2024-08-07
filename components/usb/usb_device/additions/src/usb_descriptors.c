/*
 * SPDX-FileCopyrightText: 2020-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "usb_descriptors.h"
#include "descriptors_control.h"
#include "sdkconfig.h"

#define USB_TUSB_PID 0x4012

tusb_desc_strarray_device_t descriptor_str_tinyusb = {
    // array of pointer to string descriptors
    (char[]){0x09, 0x04}, // 0: is supported language is English (0x0409)
    "TinyUSB",            // 1: Manufacturer
    "TinyUSB Device",     // 2: Product
    "123456",             // 3: Serials, should use chip ID
    "TinyUSB CDC",        // 4: CDC Interface
    "TinyUSB WebUSB",     // 5. Webusb
    "TinyUSB MSC",        // 6: MSC Interface
    "TinyUSB HID"         // 7: HID
};
/* End of TinyUSB default */

/**** Kconfig driven Descriptor ****/
tusb_desc_device_t descriptor_kconfig = {
    .bLength = sizeof(descriptor_kconfig),
    .bDescriptorType = TUSB_DESC_DEVICE,
#if CFG_TUD_VENDOR
    .bcdUSB = 0x0210,
#else
    .bcdUSB = 0x0200,
#endif

#if CFG_TUD_CDC
    // Use Interface Association Descriptor (IAD) for CDC
    // As required by USB Specs IAD's subclass must be common class (2) and protocol must be IAD (1)
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
#elif CFG_TUD_BTH
    .bDeviceClass = TUSB_CLASS_WIRELESS_CONTROLLER,
    .bDeviceSubClass = TUD_BT_APP_SUBCLASS,
    .bDeviceProtocol = TUD_BT_PROTOCOL_PRIMARY_CONTROLLER,
#else
    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
#endif

    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

#if CONFIG_TINYUSB_DESC_USE_ESPRESSIF_VID
    .idVendor = USB_ESPRESSIF_VID,
#else
    .idVendor = CONFIG_TINYUSB_DESC_CUSTOM_VID,
#endif

#if CONFIG_TINYUSB_DESC_USE_DEFAULT_PID
    .idProduct = USB_TUSB_PID,
#else
    .idProduct = CONFIG_TINYUSB_DESC_CUSTOM_PID,
#endif

    .bcdDevice = CONFIG_TINYUSB_DESC_BCD_DEVICE,

    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,

    .bNumConfigurations = 0x01
};

tusb_desc_strarray_device_t descriptor_str_kconfig = {
    // array of pointer to string descriptors
    (char[]){0x09, 0x04},                    // 0: is supported language is English (0x0409)
    CONFIG_TINYUSB_DESC_MANUFACTURER_STRING, // 1: Manufacturer
    CONFIG_TINYUSB_DESC_PRODUCT_STRING,      // 2: Product
    CONFIG_TINYUSB_DESC_SERIAL_STRING,       // 3: Serials, should use chip ID

#if CFG_TUD_CDC
    CONFIG_TINYUSB_DESC_CDC_STRING,          // CDC Interface
#endif

#if CFG_TUD_NET
    CONFIG_TINYUSB_DESC_NET_STRING,          // NET Interface
    "",                                      // MAC
#endif

#if CFG_TUD_VENDOR
    "TinyUSB vendor",                        // Vendor Interface
#endif

#if CFG_TUD_MSC
    CONFIG_TINYUSB_DESC_MSC_STRING,          // MSC Interface
#endif

#if CFG_TUD_HID
    CONFIG_TINYUSB_DESC_HID_STRING           // HIDs
#endif

#if CFG_TUD_BTH
    CONFIG_TINYUSB_DESC_BTH_STRING,          // BTH
#endif

#if CFG_TUD_DFU
    "FLASH",                       // 4: DFU Partition 1
    "EEPROM",                      // 5: DFU Partition 2
#endif

};
/* End of Kconfig driven Descriptor */

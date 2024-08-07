/*
 * SPDX-FileCopyrightText: 2019 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TUSB_VENDOR_DEVICE_H_
#define _TUSB_VENDOR_DEVICE_H_

#include "common/tusb_common.h"

#ifndef CFG_TUD_VENDOR_EPSIZE
#define CFG_TUD_VENDOR_EPSIZE     64
#endif

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------+
// Vendor Descriptor Templates
//--------------------------------------------------------------------+

#define TUD_VENDOR_DESC_LEN  (9+7+7)

// Interface number, string index, EP Out & IN address, EP size
#define TUD_VENDOR_DESCRIPTOR(_itfnum, _stridx, _epout, _epin, _epsize) \
  /* Interface */\
  9, TUSB_DESC_INTERFACE, _itfnum, 0, 2, TUSB_CLASS_VENDOR_SPECIFIC, 0x00, 0x00, _stridx,\
  /* Endpoint Out */\
  7, TUSB_DESC_ENDPOINT, _epout, TUSB_XFER_BULK, U16_TO_U8S_LE(_epsize), 0,\
  /* Endpoint In */\
  7, TUSB_DESC_ENDPOINT, _epin, TUSB_XFER_BULK, U16_TO_U8S_LE(_epsize), 0

//--------------------------------------------------------------------+
// Application API (Multiple Interfaces)
//--------------------------------------------------------------------+
bool     tud_vendor_n_mounted(uint8_t itf);

uint32_t tud_vendor_n_available(uint8_t itf);
uint32_t tud_vendor_n_read(uint8_t itf, void* buffer, uint32_t bufsize);
bool     tud_vendor_n_peek(uint8_t itf, uint8_t* ui8);
void     tud_vendor_n_read_flush(uint8_t itf);

uint32_t tud_vendor_n_write(uint8_t itf, void const* buffer, uint32_t bufsize);
uint32_t tud_vendor_n_write_available(uint8_t itf);

static inline
uint32_t tud_vendor_n_write_str(uint8_t itf, char const* str);

//--------------------------------------------------------------------+
// Application API (Single Port)
//--------------------------------------------------------------------+
static inline bool     tud_vendor_mounted(void);
static inline uint32_t tud_vendor_available(void);
static inline uint32_t tud_vendor_read(void* buffer, uint32_t bufsize);
static inline bool     tud_vendor_peek(uint8_t* ui8);
static inline void     tud_vendor_read_flush(void);
static inline uint32_t tud_vendor_write(void const* buffer, uint32_t bufsize);
static inline uint32_t tud_vendor_write_str(char const* str);
static inline uint32_t tud_vendor_write_available(void);

//--------------------------------------------------------------------+
// Application Callback API (weak is optional)
//--------------------------------------------------------------------+

// Invoked when received new data
TU_ATTR_WEAK void tud_vendor_rx_cb(uint8_t itf);

//--------------------------------------------------------------------+
// Inline Functions
//--------------------------------------------------------------------+

static inline uint32_t tud_vendor_n_write_str(uint8_t itf, char const* str)
{
    return tud_vendor_n_write(itf, str, strlen(str));
}

static inline bool tud_vendor_mounted(void)
{
    return tud_vendor_n_mounted(0);
}

static inline uint32_t tud_vendor_available(void)
{
    return tud_vendor_n_available(0);
}

static inline uint32_t tud_vendor_read(void* buffer, uint32_t bufsize)
{
    return tud_vendor_n_read(0, buffer, bufsize);
}

static inline bool tud_vendor_peek(uint8_t* ui8)
{
    return tud_vendor_n_peek(0, ui8);
}

static inline void tud_vendor_read_flush(void)
{
    tud_vendor_n_read_flush(0);
}

static inline uint32_t tud_vendor_write(void const* buffer, uint32_t bufsize)
{
    return tud_vendor_n_write(0, buffer, bufsize);
}

static inline uint32_t tud_vendor_write_str(char const* str)
{
    return tud_vendor_n_write_str(0, str);
}

static inline uint32_t tud_vendor_write_available(void)
{
    return tud_vendor_n_write_available(0);
}

//--------------------------------------------------------------------+
// Internal Class Driver API
//--------------------------------------------------------------------+
void     vendord_init(void);
void     vendord_reset(uint8_t rhport);
uint16_t vendord_open(uint8_t rhport, tusb_desc_interface_t const * itf_desc, uint16_t max_len);
bool     vendord_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t event, uint32_t xferred_bytes);

#ifdef __cplusplus
}
#endif

#endif /* _TUSB_VENDOR_DEVICE_H_ */

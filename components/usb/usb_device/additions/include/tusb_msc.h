/*
 * SPDX-FileCopyrightText: 2020-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "tusb.h"
#include "tinyusb.h"

/**
 * @brief Configuration structure for MSC
 */
typedef struct {
    uint8_t pdrv;             /* Physical drive nmuber (0..) */
} tinyusb_config_msc_t;

/**
 * @brief Initialize MSC Device.
 *
 * @param cfg - init configuration structure
 * @return esp_err_t
 */
esp_err_t tusb_msc_init(const tinyusb_config_msc_t *cfg);

#ifdef __cplusplus
}
#endif

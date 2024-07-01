/*
 * SPDX-FileCopyrightText: 2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "esp_err.h"
#include "tinyusb.h"

/**
 * @brief Initialize NET Device.
 */
void tusb_net_init(void);

void ecm_close(void);

void ecm_open(void);

#ifdef __cplusplus
}
#endif

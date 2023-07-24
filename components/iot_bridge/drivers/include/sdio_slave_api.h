/*
 * SPDX-FileCopyrightText: 2015-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifdef CONFIG_IDF_TARGET_ESP32
#elif defined CONFIG_IDF_TARGET_ESP32S2
    #error "SDIO is not supported for ESP32S2. Please use SPI"
#endif

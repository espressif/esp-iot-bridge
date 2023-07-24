/*
 * SPDX-FileCopyrightText: 2015-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "wifi_dongle_adapter.h"
#include "interface.h"

typedef struct {
	interface_context_t *context;
} adapter;

void network_adapter_driver_init(void);

esp_err_t pkt_dhcp_status_change(void *buffer, uint16_t len);

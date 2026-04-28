/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdint.h>
#include "esp_event.h"
#include "esp_netif_types.h"

#ifdef __cplusplus
extern "C" {
#endif

ESP_EVENT_DECLARE_BASE(BRIDGE_EVENT);

/**
 * Event ID under @ref BRIDGE_EVENT: After DHCP writes the DNS to lwIP, notify Bridge to synchronize the DNS for the data forwarding interface.
 * The @c event_data is a @ref bridge_dns_update_event_data_t, where @c esp_netif corresponds to the lwIP
 * @c netif parameter in lwIP's @c dhcp_dns_update_customer_cb(struct netif *netif, void *param).
 */
#define BRIDGE_EVENT_ID_DNS_UPDATE 0

typedef struct {
    esp_netif_t *esp_netif;
} bridge_dns_update_event_data_t;

#ifdef __cplusplus
}
#endif

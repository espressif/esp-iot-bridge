/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#define IP_NAPT_PORTMAP           1
#define IP_PORTMAP_MAX            255
#define IP_NAPT_MAX               CONFIG_BRIDGE_IP_NAPT_SIZE
#define IP_NAPT_TIMEOUT_MS_TCP    CONFIG_BRIDGE_IP_NAPT_TIMEOUT_MS_TCP
#define IP_NAPT_ADD_FAILED_HOOK() custom_hook_ip4_napt_add_failed()

int custom_hook_ip4_napt_add_failed(void);

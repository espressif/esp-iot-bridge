/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_wifi.h"
#include "lwip/lwip_napt.h"

__attribute__((weak)) int custom_hook_ip4_napt_add_failed(void)
{
    printf("IP_NAPT_MAX:%d, IP_PORTMAP_MAX:%d, IP_NAPT_TIMEOUT_MS_TCP:%d\r\n", IP_NAPT_MAX, IP_PORTMAP_MAX, IP_NAPT_TIMEOUT_MS_TCP);
    return 0;
}

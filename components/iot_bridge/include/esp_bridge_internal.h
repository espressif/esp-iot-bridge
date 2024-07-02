/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <esp_err.h>
#include "esp_wifi.h"

#include "lwip/opt.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/lwip_napt.h"

#include "esp_bridge.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* Definitions for error constants. */
#define ESP_ERR_DUPLICATE_ADDITION    0x110   /*!< Netif was added repeatedly */

typedef esp_err_t(*dhcps_change_cb_t)(esp_ip_addr_t *ip_info);

#ifndef IOT_BRIDGE_NAPT_TABLE_CLEAR
void ip_napt_table_clear(void);
#define IOT_BRIDGE_NAPT_TABLE_CLEAR()    ip_napt_table_clear()
#endif

/**
 * @brief  Cause the TCP/IP stack to bring up an interface
 * This function is called automatically by default called from event handlers/actions
 *
 * @note This function is not normally used with Wi-Fi AP interface. If the AP interface is started, it is up.
 *
 * @param[in]  esp_netif Handle to esp-netif instance
 *
 * @return
 *     - ESP_OK
 *     - ESP_ERR_ESP_NETIF_IF_NOT_READY
 */
esp_err_t esp_netif_up(esp_netif_t *esp_netif);

/**
 * @brief  Create a netif interface and configure it.
 *
 * @param[in]  config netif configuration
 * @param[in]  ip_info custom ip address, if you choose to use the system to automatically assign, set NULL.
 * @param[in]  mac custom mac address, if you choose to use the system to automatically assign, set NULL.
 * @param[in]  enable_dhcps whether to enable DHCP server
 *
 * @return
 *     - instance: create netif instance successfully
 *     - NULL: create modem netif instance failed because some error occurred
 */
esp_netif_t *esp_bridge_create_netif(esp_netif_config_t *config, esp_netif_ip_info_t *ip_info, uint8_t mac[6], bool enable_dhcps);

/**
 * @brief  Add netif instance to the list.
 *
 * @param[in]  netif netif instance
 * @param[in]  dhcps_change_cb reset Nic when netif IP was changed
 *
 * @return
 *     - ESP_OK: Add netif instance successfully
 *     - others: other failure occurred include netif duplicate addition or Out of memory
 */
#define esp_bridge_netif_list_add(netif, dhcps_change_cb) \
    _esp_bridge_netif_list_add(netif, dhcps_change_cb, COMMIT_ID)

esp_err_t _esp_bridge_netif_list_add(esp_netif_t *netif, dhcps_change_cb_t dhcps_change_cb, const char *commit_id);

/**
 * @brief  Remove netif instance to the list.
 *
 * @param[in]  netif netif instance
 *
 * @return
 *     - ESP_OK: Remove netif instance successfully
 */
esp_err_t esp_bridge_netif_list_remove(esp_netif_t *netif);

/**
 * @brief  Request to allocate an ip information that does not conflict with the existing netif ip network segment.
 *
 * @param[out]  ip_info ip information
 *
 * @return
 *     - ESP_OK: request ip successfully
 *     - ESP_FAIL: request ip failure
 */
esp_err_t esp_bridge_netif_request_ip(esp_netif_ip_info_t *ip_info);

/**
 * @brief  Request to allocate an mac that does not conflict with the existing netif ip network segment.
 *
 * @param[out]  mac netif mac
 *
 * @return
 *     - ESP_OK: request mac successfully
 */
esp_err_t esp_bridge_netif_request_mac(uint8_t *mac);

#ifdef __cplusplus
}
#endif

/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "esp_netif.h"

#if defined(CONFIG_BRIDGE_EXTERNAL_NETIF_STATION) || defined(CONFIG_BRIDGE_DATA_FORWARDING_NETIF_SOFTAP)
#include "esp_wifi_types.h"

esp_err_t esp_bridge_wifi_set(wifi_mode_t mode,
                              const char *ssid,
                              const char *password,
                              const char *bssid);

esp_err_t esp_bridge_wifi_set_config(wifi_interface_t interface, wifi_config_t *conf);

#endif

#if defined(CONFIG_BRIDGE_EXTERNAL_NETIF_MODEM)
/**
* @brief Create modem netif for bridge.
*
* @param[in] ip_info: custom ip address, if set NULL, it will automatically be assigned.
* @param[in] mac: custom mac address, if set NULL, it will automatically be assigned.
* @param[in] data_forwarding: whether to use as data forwarding netif
* @param[in] enable_dhcps: whether to enable DHCP server
*
* @return
*      - instance: the netif instance created successfully
*      - NULL: failed because some error occurred
*/
esp_netif_t *esp_bridge_create_modem_netif(esp_netif_ip_info_t *ip_info, uint8_t mac[6], bool data_forwarding, bool enable_dhcps);
#endif

#if defined(CONFIG_BRIDGE_EXTERNAL_NETIF_STATION)
/**
* @brief Create station netif for bridge.
*
* @param[in] ip_info: custom ip address, if set NULL, it will automatically be assigned.
* @param[in] mac: custom mac address, if set NULL, it will automatically be assigned.
* @param[in] data_forwarding: whether to use as data forwarding netif
* @param[in] enable_dhcps: whether to enable DHCP server
*
* @return
*      - instance: the netif instance created successfully
*      - NULL: failed because some error occurred
*/
esp_netif_t *esp_bridge_create_station_netif(esp_netif_ip_info_t *ip_info, uint8_t mac[6], bool data_forwarding, bool enable_dhcps);
#endif

#if defined(CONFIG_BRIDGE_DATA_FORWARDING_NETIF_SOFTAP)
/**
* @brief Create softap netif for bridge.
*
* @param[in] ip_info: custom ip address, if set NULL, it will automatically be assigned.
* @param[in] mac: custom mac address, if set NULL, it will automatically be assigned.
* @param[in] data_forwarding: whether to use as data forwarding netif
* @param[in] enable_dhcps: whether to enable DHCP server
*
* @return
*      - instance: the netif instance created successfully
*      - NULL: failed because some error occurred
*/
esp_netif_t *esp_bridge_create_softap_netif(esp_netif_ip_info_t *ip_info, uint8_t mac[6], bool data_forwarding, bool enable_dhcps);
#endif

#if defined(CONFIG_BRIDGE_ETHERNET_NETIF_ENABLE)
/**
* @brief Create eth netif for bridge.
*
* @param[in] ip_info: custom ip address, if set NULL, it will automatically be assigned.
* @param[in] mac: custom mac address, if set NULL, it will automatically be assigned.
* @param[in] data_forwarding: whether to use as data forwarding netif
* @param[in] enable_dhcps: whether to enable DHCP server
*
* @return
*      - instance: the netif instance created successfully
*      - NULL: failed because some error occurred
*/
esp_netif_t *esp_bridge_create_eth_netif(esp_netif_ip_info_t *ip_info, uint8_t mac[6], bool data_forwarding, bool enable_dhcps);
#endif

#if defined(CONFIG_BRIDGE_DATA_FORWARDING_NETIF_USB)
/**
* @brief Create usb netif for bridge.
*
* @param[in] ip_info: custom ip address, if set NULL, it will automatically be assigned.
* @param[in] mac: custom mac address, if set NULL, it will automatically be assigned.
* @param[in] data_forwarding: whether to use as data forwarding netif
* @param[in] enable_dhcps: whether to enable DHCP server
*
* @return
*      - instance: the netif instance created successfully
*      - NULL: failed because some error occurred
*/
esp_netif_t *esp_bridge_create_usb_netif(esp_netif_ip_info_t *ip_info, uint8_t mac[6], bool data_forwarding, bool enable_dhcps);
#endif

#if defined(CONFIG_BRIDGE_EXTERNAL_NETIF_SDIO) || defined(CONFIG_BRIDGE_DATA_FORWARDING_NETIF_SDIO)
/**
* @brief Create sdio netif for bridge.
*
* @param[in] ip_info: custom ip address, if set NULL, it will automatically be assigned.
* @param[in] mac: custom mac address, if set NULL, it will automatically be assigned.
* @param[in] data_forwarding: whether to use as data forwarding netif
* @param[in] enable_dhcps: whether to enable DHCP server
*
* @return
*      - instance: the netif instance created successfully
*      - NULL: failed because some error occurred
*/
esp_netif_t *esp_bridge_create_sdio_netif(esp_netif_ip_info_t *ip_info, uint8_t mac[6], bool data_forwarding, bool enable_dhcps);
#endif

#if defined(CONFIG_BRIDGE_EXTERNAL_NETIF_SPI) || defined(CONFIG_BRIDGE_DATA_FORWARDING_NETIF_SPI)
/**
* @brief Create spi netif for bridge.
*
* @param[in] ip_info: custom ip address, if set NULL, it will automatically be assigned.
* @param[in] mac: custom mac address, if set NULL, it will automatically be assigned.
* @param[in] data_forwarding: whether to use as data forwarding netif
* @param[in] enable_dhcps: whether to enable DHCP server
*
* @return
*      - instance: the netif instance created successfully
*      - NULL: failed because some error occurred
*/
esp_netif_t *esp_bridge_create_spi_netif(esp_netif_ip_info_t *ip_info, uint8_t mac[6], bool data_forwarding, bool enable_dhcps);
#endif

/**
 * @brief Update DNS information for netif interface(s).
 *
 * This function updates the DNS information of the netif interface(s). If the second parameter
 * (`data_forwarding_netif`) is not NULL, it retrieves the DNS information from the first parameter
 * (`external_netif`) and sets it as the DNS information for the DHCP server of `data_forwarding_netif`.
 * If the second parameter is NULL, it updates the DNS information for all data forwarding netif interfaces.
 *
 * @param[in] external_netif Pointer to the external netif (source of DNS information).
 * @param[in] data_forwarding_netif Pointer to the data forwarding netif (target for DNS information).
 *                                  If NULL, update DNS information for all external netifs.
 *
 * @note The function assumes that the DNS information in the external_netif is accessible
 * and valid for the data_forwarding_netif.
 *
 * @warning This function may override existing DNS information in the all data forwarding_netifs
 * (if data_forwarding_netif is NULL).
 *
 * @return
 *     - ESP_OK: DNS information updated successfully.
 *     - ESP_ERR_INVALID_ARG: One or more input arguments are invalid.
 */
esp_err_t esp_bridge_update_dns_info(esp_netif_t *external_netif, esp_netif_t *data_forwarding_netif);

/**
* @brief Create all netif which are enabled in menuconfig, for example, station, modem, ethernet.
*
*/
void esp_bridge_create_all_netif(void);

/**
 * @brief  Registered users check network segment conflict interface.
 *
 * @param[in]  custom_check_cb check network segment callback
 *
 * @return
 *     - True: Registration success
 */
bool esp_bridge_network_segment_check_register(bool (*custom_check_cb)(uint32_t ip));

/**
 * @brief  Check whether the other data-forwarding netif IP network segment conflicts with this one.
 *         If yes, it will update the data-forwarding netif to a new IP network segment, otherwise, do nothing.
 *
 * @param[in]  esp_netif the netif information
 *
 * @return
 *     - ESP_OK
 */
esp_err_t esp_bridge_netif_network_segment_conflict_update(esp_netif_t *esp_netif);

/**
 * @brief Load IP information with the specified name from NVS into the given structure.
 *
 * @param[in] name The key name of the netif associated with the IP information to be loaded.
 * @param[out] ip_info A pointer to the structure to store the loaded IP information.
 * @param[out] conflict_check A pointer to store whether check IP segment conflict.
 *
 * @return
 *     - ESP_OK: IP information loaded successfully.
 *     - Other: Error code indicating failure during loading.
 */
esp_err_t esp_bridge_load_ip_info_from_nvs(const char *name, esp_netif_ip_info_t *ip_info, bool *conflict_check);

/**
 * @brief Set the IP information for a network interface and optionally save it to NVS.
 *
 * @param[in] netif The network interface to set the IP information for.
 * @param[in] ip_info A pointer to the IP information structure to set.
 * @param[in] save_to_nvs Whether to save the IP information to NVS.
 * @param[in] conflict_check Whether to check IP segment conflict on this netif.
 *
 * @return
 *     - ESP_OK: IP information set successfully.
 *     - Other: Error code indicating failure during setting.
 */
esp_err_t esp_bridge_netif_set_ip_info(esp_netif_t *netif, esp_netif_ip_info_t *ip_info, bool save_to_nvs, bool conflict_check);

#ifdef __cplusplus
}
#endif

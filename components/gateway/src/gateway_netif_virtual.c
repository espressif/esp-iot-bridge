/* Net-suite test code

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/debug.h"
#include "lwip/tcp.h"

uint8_t virtual_mac[16];
esp_netif_t* virtual_netif;

//
// Internal functions declaration referenced in io object
//
static esp_err_t netsuite_io_transmit(void *h, void *buffer, size_t len);
static esp_err_t netsuite_io_transmit_wrap(void *h, void *buffer, size_t len, void *netstack_buf);
static esp_err_t netsuite_io_attach(esp_netif_t * esp_netif, void * args);

/**
 * @brief IO object netif related configuration with data-path function callbacks
 * and pointer to the IO object instance (unused as this is a singleton)
 */
const esp_netif_driver_ifconfig_t c_driver_ifconfig = {
        .driver_free_rx_buffer = NULL,
        .transmit = netsuite_io_transmit,
        .transmit_wrap = netsuite_io_transmit_wrap,
        .handle = "netsuite-io-object" // this IO object is a singleton, its handle uses as a name
};

/**
 * @brief IO object base structure used to point to internal attach function
 */
const esp_netif_driver_base_t s_driver_base = {
        .post_attach =  netsuite_io_attach
};

/**
 * @brief Transmit function called from esp_netif to output network stack data
 *
 * Note: This API has to conform to esp-netif transmit prototype
 *
 * @param h Opaque pointer representing the io driver (unused, const string in this case)
 * @param data data buffer
 * @param length length of data to send
 *
 * @return ESP_OK on success
 */
static esp_err_t netsuite_io_transmit(void *h, void *buffer, size_t len)
{
    esp_err_t pkt_virnet2eth(void *buffer, uint16_t len);
    pkt_virnet2eth(buffer, len);
    return ESP_OK;
}

/**
 * @brief Transmit wrapper that is typically used for buffer handling and optimization.
 * Here just wraps the netsuite_io_transmit().
 *
 * @note The netstack_buf could be a ref-counted network stack buffer and might be used
 * by the lower layers directly if an additional handling is practical.
 * See docs on `esp_wifi_internal_tx_by_ref()` in components/esp_wifi/include/esp_private/wifi.h
 */
static esp_err_t netsuite_io_transmit_wrap(void *h, void *buffer, size_t len, void *netstack_buf)
{
    return netsuite_io_transmit(h, buffer, len);
}

/**
 * @brief Post attach adapter for netsuite i/o
 *
 * Used to exchange internal callbacks and context between esp-netif and the I/O object.
 * In case of netsuite I/O, it only updates the driver config with internal callbacks and
 * its instance pointer (const string in this case)
 *
 * @param esp_netif handle to esp-netif object
 * @param args pointer to netsuite IO
 *
 * @return ESP_OK on success
 */
static esp_err_t netsuite_io_attach(esp_netif_t * esp_netif, void * args)
{
    ESP_ERROR_CHECK(esp_netif_set_driver_config(esp_netif, &c_driver_ifconfig));
    return ESP_OK;
}

/**
 * Created (initializes) the i/o object and returns handle ready to be attached to the esp-netif
 */
void *netsuite_io_new(void)
{
    return (void *)&s_driver_base;
}

void esp_gateway_netif_virtual_init(void)
{
    // Netif configs
    //
    esp_netif_ip_info_t ip_info;
    esp_netif_inherent_config_t netif_common_config = {
            .flags = ESP_NETIF_FLAG_AUTOUP,
            .ip_info = (esp_netif_ip_info_t*)&ip_info,
            .if_key = "Virtual_key",
            .if_desc = "Netif"
    };
    esp_netif_set_ip4_addr(&ip_info.ip, 10, 0 , 0, 1);
    esp_netif_set_ip4_addr(&ip_info.gw, 10, 0 , 0, 1);
    esp_netif_set_ip4_addr(&ip_info.netmask, 255, 255 , 255, 0);

    esp_netif_config_t config = {
        .base = &netif_common_config,                 // use specific behaviour configuration
        .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH,      // use default WIFI-like network stack configuration
    };

    // Netif creation and configuration
    //
    ESP_ERROR_CHECK(esp_netif_init());
    esp_netif_t* netif = esp_netif_new(&config);
    assert(netif);
    virtual_netif = netif;
    esp_eth_set_default_handlers(netif);
    esp_netif_attach(netif, netsuite_io_new());

    // Start the netif in a manual way, no need for events
    //
    esp_netif_set_mac(netif, virtual_mac);
}

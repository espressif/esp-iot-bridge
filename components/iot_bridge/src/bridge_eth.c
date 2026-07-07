/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdlib.h>

#include "esp_log.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_check.h"

#include "netif/ethernet.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"

#include "esp_mac.h"

#include "esp_bridge.h"
#include "esp_bridge_internal.h"

#include "sdkconfig.h"

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0)
#include "ethernet_init.h"
#if CONFIG_ETHERNET_INTERNAL_SUPPORT
#define BRIDGE_ETH_USE_INTERNAL 1
#elif CONFIG_ETHERNET_SPI_SUPPORT
#define BRIDGE_ETH_USE_SPI 1
#endif
#else
#if CONFIG_BRIDGE_USE_INTERNAL_ETHERNET
#define BRIDGE_ETH_USE_INTERNAL 1
#elif CONFIG_BRIDGE_USE_SPI_ETHERNET
#define BRIDGE_ETH_USE_SPI 1
#endif

#if defined(BRIDGE_ETH_USE_SPI)
#include "driver/spi_master.h"
#endif
#endif // ESP_IDF_VERSION < 6.0

static const char *TAG = "bridge_eth";

static esp_netif_t *eth_wan_netif = NULL;
static esp_netif_t *eth_lan_netif = NULL;

static esp_eth_phy_t *phy = NULL;

#if defined(CONFIG_BRIDGE_NETIF_ETHERNET_AUTO_WAN_OR_LAN)
esp_eth_netif_glue_handle_t esp_bridge_eth_new_netif_glue(esp_eth_handle_t eth_hdl);
#endif

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(6, 0, 0)
/**
 * @brief ESP-Bridge Default configuration for Ethernet PHY object
 *
 */
#define ESP_BRIDGE_ETH_PHY_DEFAULT_CONFIG()                \
    {                                                      \
        .phy_addr = CONFIG_BRIDGE_ETH_PHY_ADDR,            \
        .reset_timeout_ms = 100,                           \
        .autonego_timeout_ms = 4000,                       \
        .reset_gpio_num = CONFIG_BRIDGE_ETH_PHY_RST_GPIO,  \
    }
#endif

// Event handler for Ethernet
static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGI(TAG, "Ethernet HW Addr "MACSTR"", MAC2STR(mac_addr));
            break;

        case ETHERNET_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "Ethernet Link Down");
            break;

        case ETHERNET_EVENT_START:
            ESP_LOGI(TAG, "Ethernet Started");
            break;

        case ETHERNET_EVENT_STOP:
            ESP_LOGI(TAG, "Ethernet Stopped");
            break;

        default:
            break;
    }
}
#if defined(CONFIG_BRIDGE_EXTERNAL_NETIF_ETHERNET) || defined(CONFIG_BRIDGE_NETIF_ETHERNET_AUTO_WAN_OR_LAN)
/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    ESP_LOGI(TAG, "Ethernet Gotx IP Address");
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    esp_bridge_update_dns_info(event->esp_netif, NULL);
    esp_bridge_netif_network_segment_conflict_update(event->esp_netif);
}
#endif
void esp_bridge_eth_event_handler_register(void)
{
    // Register user defined event handers
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
#if defined(CONFIG_BRIDGE_EXTERNAL_NETIF_ETHERNET) || defined(CONFIG_BRIDGE_NETIF_ETHERNET_AUTO_WAN_OR_LAN)
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));
#endif /* CONFIG_BRIDGE_EXTERNAL_NETIF_ETHERNET */
}

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0)
#if defined(BRIDGE_ETH_USE_INTERNAL) || defined(BRIDGE_ETH_USE_SPI)

#if CONFIG_BRIDGE_ETH_PHY_YT8531_INIT
static esp_err_t esp_bridge_eth_phy_yt8531_init(esp_eth_handle_t eth_handle)
{
    bool auto_nego_en = true;
    ESP_RETURN_ON_ERROR(esp_eth_ioctl(eth_handle, ETH_CMD_S_AUTONEGO, &auto_nego_en), TAG, "set auto negotiation failed");

    esp_eth_phy_reg_rw_data_t phy_reg = {.reg_value_p = NULL};
    uint32_t reg_val;
    phy_reg.reg_value_p = &reg_val;

    reg_val = 0xA001;
    phy_reg.reg_addr = 0x1E;
    ESP_RETURN_ON_ERROR(esp_eth_ioctl(eth_handle, ETH_CMD_WRITE_PHY_REG, &phy_reg), TAG, "write EXT addr reg (Chip_Config) failed");
    phy_reg.reg_addr = 0x1F;
    ESP_RETURN_ON_ERROR(esp_eth_ioctl(eth_handle, ETH_CMD_READ_PHY_REG, &phy_reg), TAG, "read Chip_Config failed");
    reg_val |= (1U << 8);
    ESP_RETURN_ON_ERROR(esp_eth_ioctl(eth_handle, ETH_CMD_WRITE_PHY_REG, &phy_reg), TAG, "write Chip_Config failed");

    reg_val = 0xA003;
    phy_reg.reg_addr = 0x1E;
    ESP_RETURN_ON_ERROR(esp_eth_ioctl(eth_handle, ETH_CMD_WRITE_PHY_REG, &phy_reg), TAG, "write EXT addr reg (RGMII_Config1) failed");
    phy_reg.reg_addr = 0x1F;
    ESP_RETURN_ON_ERROR(esp_eth_ioctl(eth_handle, ETH_CMD_READ_PHY_REG, &phy_reg), TAG, "read RGMII_Config1 failed");
    reg_val = (reg_val & ~0x00FFU) | (13U << 4) | (13U << 0);
    ESP_RETURN_ON_ERROR(esp_eth_ioctl(eth_handle, ETH_CMD_WRITE_PHY_REG, &phy_reg), TAG, "write RGMII_Config1 failed");

    ESP_LOGI(TAG, "YT8531 RGMII PHY delays configured");
    return ESP_OK;
}
#endif // CONFIG_BRIDGE_ETH_PHY_YT8531_INIT

static esp_err_t esp_bridge_eth_init_common(esp_netif_t *eth_netif, uint8_t mac[6])
{
    static esp_eth_handle_t *s_eth_handles = NULL;
    static esp_eth_handle_t s_eth_handle = NULL;
    static bool s_eth_driver_started = false;
    esp_err_t ret = ESP_OK;

    if (!s_eth_handle) {
        uint8_t eth_cnt = 0;
        ESP_RETURN_ON_ERROR(ethernet_init_all(&s_eth_handles, &eth_cnt), TAG, "ethernet_init_all failed");
        if (eth_cnt != 1) {
            ESP_LOGE(TAG, "Expected 1 Ethernet port, got %u", eth_cnt);
            return ESP_FAIL;
        }
        s_eth_handle = s_eth_handles[0];

#if defined(BRIDGE_ETH_USE_SPI)
        if (mac != NULL) {
            ESP_LOGI(TAG, "Set SPI Ethernet MAC address: %02x:%02x:%02x:%02x:%02x:%02x",
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            ESP_RETURN_ON_ERROR(esp_eth_ioctl(s_eth_handle, ETH_CMD_S_MAC_ADDR, mac), TAG, "set MAC failed");
        } else {
            uint8_t default_mac[] = { 0x02, 0x00, 0x00, 0x12, 0x34, 0x56 };
            ESP_LOGI(TAG, "Set SPI Ethernet default MAC address: 02:00:00:12:34:56");
            ESP_RETURN_ON_ERROR(esp_eth_ioctl(s_eth_handle, ETH_CMD_S_MAC_ADDR, default_mac), TAG, "set default MAC failed");
        }
#endif

#if CONFIG_BRIDGE_ETH_PHY_YT8531_INIT
        ESP_RETURN_ON_ERROR(esp_bridge_eth_phy_yt8531_init(s_eth_handle), TAG, "YT8531 init failed");
#endif

        ESP_RETURN_ON_ERROR(esp_eth_get_phy_instance(s_eth_handle, &phy), TAG, "get PHY instance failed");
    }

#if defined(CONFIG_BRIDGE_NETIF_ETHERNET_AUTO_WAN_OR_LAN)
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_bridge_eth_new_netif_glue(s_eth_handle)));
#else
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(s_eth_handle)));
#endif

    if (!s_eth_driver_started) {
        ESP_RETURN_ON_ERROR(esp_eth_start(s_eth_handle), TAG, "eth start failed");
        s_eth_driver_started = true;
    }

    return ret;
}

#endif // BRIDGE_ETH_USE_INTERNAL || BRIDGE_ETH_USE_SPI
#endif // ESP_IDF_VERSION >= 6.0

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(6, 0, 0)
#if defined(BRIDGE_ETH_USE_INTERNAL)

typedef esp_eth_phy_t *(*esp_bridge_eth_phy_model_t)(const eth_phy_config_t *config);

esp_bridge_eth_phy_model_t esp_bridge_eth_phy_model[] = {
#if CONFIG_BRIDGE_ETH_PHY_IP101
    esp_eth_phy_new_ip101,
#endif
#if CONFIG_BRIDGE_ETH_PHY_RTL8201
    esp_eth_phy_new_rtl8201,
#endif
#if CONFIG_BRIDGE_ETH_PHY_LAN87XX
    esp_eth_phy_new_lan87xx,
#endif // CONFIG_BRIDGE_ETH_PHY_LAN87XX
#if CONFIG_BRIDGE_ETH_PHY_DP83848
    esp_eth_phy_new_dp83848,
#endif
#if CONFIG_BRIDGE_ETH_PHY_KSZ80XX
    esp_eth_phy_new_ksz80xx,
#endif // CONFIG_BRIDGE_ETH_PHY_KSZ80XX
};

esp_err_t esp_bridge_eth_init(esp_netif_t* eth_netif)
{
    esp_err_t ret = ESP_FAIL;
    static esp_eth_handle_t eth_handle = NULL;

    if (!eth_handle) {
        // Init MAC configs to default
        eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
        eth_phy_config_t phy_config = ESP_BRIDGE_ETH_PHY_DEFAULT_CONFIG();
        uint8_t phy_model_max = sizeof(esp_bridge_eth_phy_model)/sizeof(esp_bridge_eth_phy_model[0]);

        eth_esp32_emac_config_t esp32_emac_config = ETH_ESP32_EMAC_DEFAULT_CONFIG();
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 3, 0)
        esp32_emac_config.smi_gpio.mdc_num = CONFIG_BRIDGE_ETH_MDC_GPIO;
        esp32_emac_config.smi_gpio.mdio_num = CONFIG_BRIDGE_ETH_MDIO_GPIO;
#else
        esp32_emac_config.smi_mdc_gpio_num = CONFIG_BRIDGE_ETH_MDC_GPIO;
        esp32_emac_config.smi_mdio_gpio_num = CONFIG_BRIDGE_ETH_MDIO_GPIO;
#endif
        esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&esp32_emac_config, &mac_config);

        for (uint8_t i = 0; i < phy_model_max; i++) {
            phy = esp_bridge_eth_phy_model[i](&phy_config);
            esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);
            if (esp_eth_driver_install(&eth_config, &eth_handle) == ESP_OK){
                ret = ESP_OK;
                break;
            }
        }
    }

    /* attach Ethernet driver to TCP/IP stack */
#if defined(CONFIG_BRIDGE_NETIF_ETHERNET_AUTO_WAN_OR_LAN)
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_bridge_eth_new_netif_glue(eth_handle)));
#else
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle)));
#endif

    /* start Ethernet driver state machine */
    esp_eth_start(eth_handle);

    return ret;
}
#endif // BRIDGE_ETH_USE_INTERNAL

#if defined(BRIDGE_ETH_USE_SPI)

#if CONFIG_ETH_SPI_ETHERNET_KSZ8851SNL
esp_err_t esp_spi_eth_new_ksz8851snl(spi_device_interface_config_t *spi_devcfg, esp_netif_t *eth_netif_spi, esp_eth_handle_t *eth_handle_spi)
{
    esp_eth_mac_t *mac_spi;
    eth_mac_config_t mac_config_spi = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config_spi = ETH_PHY_DEFAULT_CONFIG();

    eth_ksz8851snl_config_t ksz8851snl_config = ETH_KSZ8851SNL_DEFAULT_CONFIG(CONFIG_BRIDGE_ETH_SPI_HOST, spi_devcfg);

    ksz8851snl_config.int_gpio_num = CONFIG_BRIDGE_ETH_SPI_INT0_GPIO;
    phy_config_spi.phy_addr = CONFIG_BRIDGE_ETH_SPI_PHY_ADDR0;
    phy_config_spi.reset_gpio_num = CONFIG_BRIDGE_ETH_SPI_PHY_RST0_GPIO;

    mac_spi = esp_eth_mac_new_ksz8851snl(&ksz8851snl_config, &mac_config_spi);
    phy = esp_eth_phy_new_ksz8851snl(&phy_config_spi);
    esp_eth_config_t eth_config_spi_ksz8851snl = ETH_DEFAULT_CONFIG(mac_spi, phy);
    return esp_eth_driver_install(&eth_config_spi_ksz8851snl, eth_handle_spi);
}
#endif

#if CONFIG_ETH_SPI_ETHERNET_DM9051
esp_err_t esp_spi_eth_new_dm9051(spi_device_interface_config_t *spi_devcfg, esp_netif_t *eth_netif_spi, esp_eth_handle_t *eth_handle_spi)
{
    esp_eth_mac_t *mac_spi;
    eth_mac_config_t mac_config_spi = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config_spi = ETH_PHY_DEFAULT_CONFIG();

    eth_dm9051_config_t dm9051_config = ETH_DM9051_DEFAULT_CONFIG(CONFIG_BRIDGE_ETH_SPI_HOST, spi_devcfg);

    dm9051_config.int_gpio_num = CONFIG_BRIDGE_ETH_SPI_INT0_GPIO;
    dm9051_config.poll_period_ms = 0;
    phy_config_spi.phy_addr = CONFIG_BRIDGE_ETH_SPI_PHY_ADDR0;
    phy_config_spi.reset_gpio_num = CONFIG_BRIDGE_ETH_SPI_PHY_RST0_GPIO;

    mac_spi = esp_eth_mac_new_dm9051(&dm9051_config, &mac_config_spi);
    phy = esp_eth_phy_new_dm9051(&phy_config_spi);
    esp_eth_config_t eth_config_spi_dm9051 = ETH_DEFAULT_CONFIG(mac_spi, phy);
    return esp_eth_driver_install(&eth_config_spi_dm9051, eth_handle_spi);
}
#endif

#if CONFIG_ETH_SPI_ETHERNET_W5500
esp_err_t esp_spi_eth_new_w5500(spi_device_interface_config_t *spi_devcfg, esp_netif_t *eth_netif_spi, esp_eth_handle_t *eth_handle_spi)
{
    esp_eth_mac_t *mac_spi;
    eth_mac_config_t mac_config_spi = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config_spi = ETH_PHY_DEFAULT_CONFIG();

    eth_w5500_config_t w5500_config = ETH_W5500_DEFAULT_CONFIG(CONFIG_BRIDGE_ETH_SPI_HOST, spi_devcfg);

    w5500_config.int_gpio_num = CONFIG_BRIDGE_ETH_SPI_INT0_GPIO;
    phy_config_spi.phy_addr = CONFIG_BRIDGE_ETH_SPI_PHY_ADDR0;
    phy_config_spi.reset_gpio_num = CONFIG_BRIDGE_ETH_SPI_PHY_RST0_GPIO;

    mac_spi = esp_eth_mac_new_w5500(&w5500_config, &mac_config_spi);
    phy = esp_eth_phy_new_w5500(&phy_config_spi);
    esp_eth_config_t eth_config_spi_w5500 = ETH_DEFAULT_CONFIG(mac_spi, phy);
    return esp_eth_driver_install(&eth_config_spi_w5500, eth_handle_spi);
}
#endif

typedef esp_err_t (*esp_bridge_spi_eth_module_t)(spi_device_interface_config_t *spi_devcfg, esp_netif_t *eth_netif_spi, esp_eth_handle_t *eth_handle_spi);

esp_bridge_spi_eth_module_t esp_bridge_spi_eth_module[] = {
#if CONFIG_ETH_SPI_ETHERNET_DM9051
    esp_spi_eth_new_dm9051,
#endif
#if CONFIG_ETH_SPI_ETHERNET_KSZ8851SNL
    esp_spi_eth_new_ksz8851snl,
#endif
#if CONFIG_ETH_SPI_ETHERNET_W5500
    esp_spi_eth_new_w5500,
#endif
};

esp_err_t esp_bridge_eth_spi_init(esp_netif_t* eth_netif_spi, uint8_t mac[6])
{
    esp_err_t ret = ESP_FAIL;
    static bool spi_bus_inited = false;
    static bool eth_driver_started = false;
    static esp_eth_handle_t eth_handle_spi = NULL;

    if (!spi_bus_inited) {
        esp_err_t err = gpio_install_isr_service(0);
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
            ESP_LOGE(TAG, "GPIO ISR service install failed: %s", esp_err_to_name(err));
            return err;
        }

        spi_bus_config_t buscfg = {
            .miso_io_num = CONFIG_BRIDGE_ETH_SPI_MISO_GPIO,
            .mosi_io_num = CONFIG_BRIDGE_ETH_SPI_MOSI_GPIO,
            .sclk_io_num = CONFIG_BRIDGE_ETH_SPI_SCLK_GPIO,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
        };
        err = spi_bus_initialize(CONFIG_BRIDGE_ETH_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
            ESP_LOGE(TAG, "SPI bus init failed: %s", esp_err_to_name(err));
            return err;
        }
        spi_bus_inited = true;
    }

    if (!eth_handle_spi) {
        spi_device_interface_config_t devcfg = {
            .mode = 0,
            .clock_speed_hz = CONFIG_BRIDGE_ETH_SPI_CLOCK_MHZ * 1000 * 1000,
            .queue_size = 20,
            .spics_io_num = CONFIG_BRIDGE_ETH_SPI_CS0_GPIO
        };

        uint8_t spi_eth_module_max = sizeof(esp_bridge_spi_eth_module)/sizeof(esp_bridge_spi_eth_module[0]);

        for (uint8_t i = 0; i < spi_eth_module_max; i++) {
            if (esp_bridge_spi_eth_module[i](&devcfg, eth_netif_spi, &eth_handle_spi) == ESP_OK) {
                ret = ESP_OK;
                break;
            }
        }
    } else {
        ret = ESP_OK;
    }

    if (!eth_handle_spi) {
        ESP_LOGE(TAG, "SPI Ethernet init failed");
        return ESP_FAIL;
    }

    if (mac != NULL) {
        ESP_LOGI(TAG, "Set SPI Ethernet MAC address: %02x:%02x:%02x:%02x:%02x:%02x",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        ESP_ERROR_CHECK(esp_eth_ioctl(eth_handle_spi, ETH_CMD_S_MAC_ADDR, mac));
    } else {
        ESP_LOGI(TAG, "Set SPI Ethernet default MAC address: 02:00:00:12:34:56");
        ESP_ERROR_CHECK(esp_eth_ioctl(eth_handle_spi, ETH_CMD_S_MAC_ADDR, (uint8_t[]) {
            0x02, 0x00, 0x00, 0x12, 0x34, 0x56
        }));
    }

#if defined(CONFIG_BRIDGE_NETIF_ETHERNET_AUTO_WAN_OR_LAN)
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif_spi, esp_bridge_eth_new_netif_glue(eth_handle_spi)));
#else
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif_spi, esp_eth_new_netif_glue(eth_handle_spi)));
#endif

    if (!eth_driver_started) {
        ret = esp_eth_start(eth_handle_spi);
        eth_driver_started = true;
    } else {
        ret = ESP_OK;
    }

    return ret;
}
#endif // BRIDGE_ETH_USE_SPI
#endif // ESP_IDF_VERSION < 6.0

static esp_err_t esp_bridge_eth_reset_phy(void)
{
    if (phy == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    phy->reset_hw(phy);
    ESP_LOGW(TAG, "Hardware Reset Ethernet PHY");
    return ESP_OK;
}

static esp_err_t eth_netif_dhcp_status_change_cb(esp_ip_addr_t *ip_info)
{
    return esp_bridge_eth_reset_phy();
}

static void eth_driver_free_rx_buffer(void *h, void* buffer)
{
    if (buffer) {
        free(buffer);
    }
}

static esp_err_t eth_io_transmit(void *h, void *buffer, size_t len)
{
    // TODO
    return ESP_OK;
}

static esp_err_t eth_io_transmit_wrap(void *h, void *buffer, size_t len, void *netstack_buf)
{
    return eth_io_transmit(h, buffer, len);
}

static esp_err_t esp_bridge_set_eth_netif(esp_bridge_netif_type_t type, esp_netif_t* netif)
{
    if (type == IOT_BRIDGE_NETIF_WAN) {
        eth_wan_netif = netif;
    } else if (type == IOT_BRIDGE_NETIF_LAN) {
        eth_lan_netif = netif;
    }
    return ESP_OK;
}

esp_netif_t* esp_bridge_get_eth_netif(esp_bridge_netif_type_t type)
{
    if (type == IOT_BRIDGE_NETIF_WAN) {
        return eth_wan_netif;
    } else if (type == IOT_BRIDGE_NETIF_LAN) {
        return eth_lan_netif;
    }
    return NULL;
}

esp_netif_t* esp_bridge_create_eth_netif(esp_netif_ip_info_t* ip_info, uint8_t mac[6], bool data_forwarding, bool enable_dhcps)
{
    esp_netif_ip_info_t netif_ip_info = { 0 };
    esp_netif_inherent_config_t esp_netif_common_config = {
        .get_ip_event = IP_EVENT_ETH_GOT_IP,
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 0)
        .lost_ip_event = IP_EVENT_ETH_LOST_IP,
#endif
        .if_key = "ETH_WAN",
        .if_desc = "eth",
        .flags = (esp_netif_flags_t)(ESP_NETIF_FLAG_GARP | ESP_NETIF_FLAG_EVENT_IP_MODIFIED),
        .route_prio = 50,
    };

    if (data_forwarding) {
        esp_netif_common_config.flags |= ESP_NETIF_DHCP_SERVER;
        esp_netif_common_config.if_key = "ETH_LAN";
        esp_netif_common_config.route_prio = 10;
    } else {
        esp_netif_common_config.flags |= ESP_NETIF_DHCP_CLIENT;
    }

    const esp_netif_driver_ifconfig_t eth_driver_ifconfig = {
        .driver_free_rx_buffer = eth_driver_free_rx_buffer,
        .transmit = eth_io_transmit,
        .transmit_wrap = eth_io_transmit_wrap,
        .handle = "ETH" // this IO object is a singleton, its handle uses as a name
    };

    esp_bridge_eth_event_handler_register();

    esp_netif_config_t eth_config = ESP_NETIF_DEFAULT_ETH(); // this configuration is for eth client
    eth_config.driver = &eth_driver_ifconfig;
    eth_config.base = &esp_netif_common_config;

    esp_netif_t* netif = esp_bridge_create_netif(&eth_config, ip_info, mac, enable_dhcps);
    if (netif) {
        if (data_forwarding) {
            esp_bridge_set_eth_netif(IOT_BRIDGE_NETIF_LAN, netif);
        } else {
            esp_bridge_set_eth_netif(IOT_BRIDGE_NETIF_WAN, netif);
        }
        esp_netif_action_stop(netif, NULL, 0, NULL);
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0)
#if defined(BRIDGE_ETH_USE_INTERNAL) || defined(BRIDGE_ETH_USE_SPI)
        esp_bridge_eth_init_common(netif, mac);
#endif
#elif defined(BRIDGE_ETH_USE_INTERNAL)
        esp_bridge_eth_init(netif);
#elif defined(BRIDGE_ETH_USE_SPI)
        esp_bridge_eth_spi_init(netif, mac);
#endif
        esp_netif_up(netif);

        ESP_LOGI(TAG, "[%-12s]", esp_netif_get_ifkey(netif));

        if (data_forwarding) {
            esp_bridge_netif_list_add(netif, eth_netif_dhcp_status_change_cb, eth_netif_dhcp_status_change_cb);
            esp_netif_get_ip_info(netif, &netif_ip_info);
            ESP_LOGI(TAG, "ETH IP Address:" IPSTR, IP2STR(&netif_ip_info.ip));
#if CONFIG_LWIP_IPV4_NAPT
            esp_netif_napt_enable(netif);
#endif
        } else {
            esp_bridge_netif_list_add(netif, NULL, NULL);
        }

        if (enable_dhcps) {
            esp_netif_dhcps_start(netif);
        }
    }

    return netif;
}

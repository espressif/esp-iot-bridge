#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "esp_vfs_fat.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "freertos/event_groups.h"
#include "esp_wifi.h"

#include "lwip/opt.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/lwip_napt.h"

#include "esp_utils.h"


#define GATEWAY_EVENT_STA_CONNECTED  BIT0

static bool s_wifi_is_connected = false;
static EventGroupHandle_t s_wifi_event_group = NULL;
static const char *TAG = "gateway_wifi";

/* Event handler for catching system events */
static void event_handler(void *arg, esp_event_base_t event_base,
                          int event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        // esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
        /* Signal main application to continue execution */
        s_wifi_is_connected = true;
        xEventGroupSetBits(s_wifi_event_group, GATEWAY_EVENT_STA_CONNECTED);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Disconnected. Connecting to the AP again...");
        esp_wifi_connect();
        s_wifi_is_connected = false;
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        ESP_LOGI(TAG, "STA Connecting to the AP again...");
    }
}

esp_netif_t *esp_gateway_wifi_init(wifi_mode_t mode)
{
    if (s_wifi_event_group) {
        return ESP_FAIL;
    }

    esp_netif_t *wifi_netif = NULL;

    s_wifi_event_group = xEventGroupCreate();

    if (mode & WIFI_MODE_STA) {
        wifi_netif = esp_netif_create_default_wifi_sta();
    }

    if (mode & WIFI_MODE_AP) {
        wifi_netif = esp_netif_create_default_wifi_ap();
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* Register our event handler for Wi-Fi, IP and Provisioning related events */
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(mode));
    // ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_start());

    return wifi_netif;
}

esp_err_t esp_gateway_wifi_set(wifi_mode_t mode, const char *ssid, const char *password)
{
    ESP_PARAM_CHECK(ssid);
    ESP_PARAM_CHECK(password);

    wifi_config_t wifi_cfg = {0};

    if (mode & WIFI_MODE_STA) {
        strlcpy((char *)wifi_cfg.sta.ssid, ssid, sizeof(wifi_cfg.sta.ssid));
        strlcpy((char *)wifi_cfg.sta.password, password, sizeof(wifi_cfg.sta.password));
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_cfg));

        ESP_LOGI(TAG, "sta ssid: %s password: %s", ssid, password);
    }

    if (mode & WIFI_MODE_AP) {
        wifi_cfg.ap.max_connection = 10;
        wifi_cfg.ap.authmode = strlen(password) < 8 ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;
        strlcpy((char *)wifi_cfg.ap.ssid, ssid, sizeof(wifi_cfg.ap.ssid));
        strlcpy((char *)wifi_cfg.ap.password, password, sizeof(wifi_cfg.ap.password));

        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_cfg));
    
        ESP_LOGI(TAG, "softap ssid: %s password: %s", ssid, password);
    }

    return ESP_OK;
}

esp_err_t esp_gateway_wifi_napt_enable()
{
    ip_napt_enable(_g_esp_netif_soft_ap_ip.ip.addr, 1);
    ESP_LOGI(TAG, "NAT is enabled");

    return ESP_OK;
}


esp_err_t esp_gateway_wifi_set_dhcps(esp_netif_t *netif, uint32_t addr)
{
    esp_netif_dns_info_t dns;
    dns.ip.u_addr.ip4.addr = addr;
    dns.ip.type = IPADDR_TYPE_V4;
    dhcps_offer_t dhcps_dns_value = OFFER_DNS;
    ESP_ERROR_CHECK(esp_netif_dhcps_option(netif, ESP_NETIF_OP_SET, ESP_NETIF_DOMAIN_NAME_SERVER, &dhcps_dns_value, sizeof(dhcps_dns_value)));
    ESP_ERROR_CHECK(esp_netif_set_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns));
    return ESP_OK;
}


esp_err_t esp_gateway_wifi_sta_connected(uint32_t wait_ms)
{
    esp_wifi_connect();
    xEventGroupWaitBits(s_wifi_event_group, GATEWAY_EVENT_STA_CONNECTED,
                        true, true, pdMS_TO_TICKS(wait_ms));
    return ESP_OK;
}

bool esp_gateway_wifi_is_connected()
{
    return s_wifi_is_connected;
}
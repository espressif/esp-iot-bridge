// Copyright 2021 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <string.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_modem.h"
#include "lwip/lwip_napt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_netif_ppp.h"
#include "driver/gpio.h"

static const char *TAG = "gateway_modem";

static const int CONNECT_BIT = BIT0;
static const int DISCONNECT_BIT = BIT1;

extern esp_modem_dce_t *sim7600_board_create(esp_modem_dce_config_t *config);

static void on_modem_event(void *arg, esp_event_base_t event_base,
                           int32_t event_id, void *event_data)
{
    EventGroupHandle_t connection_events = arg;
    if (event_base == IP_EVENT) {
        ESP_LOGI(TAG, "IP event! %d", event_id);
        if (event_id == IP_EVENT_PPP_GOT_IP) {
            esp_netif_dns_info_t dns_info;

            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            esp_netif_t *netif = event->esp_netif;

            ESP_LOGI(TAG, "Modem Connected to PPP Server");
            ESP_LOGI(TAG, "%s ip: " IPSTR ", mask: " IPSTR ", gw: " IPSTR, esp_netif_get_desc(netif),
                     IP2STR(&event->ip_info.ip),
                     IP2STR(&event->ip_info.netmask),
                     IP2STR(&event->ip_info.gw));
            esp_netif_get_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns_info);
            ESP_LOGI(TAG, "Main DNS: " IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
            esp_netif_get_dns_info(netif, ESP_NETIF_DNS_BACKUP, &dns_info);
            ESP_LOGI(TAG, "Backup DNS: " IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
            xEventGroupSetBits(connection_events, CONNECT_BIT);

        } else if (event_id == IP_EVENT_PPP_LOST_IP) {
            ESP_LOGI(TAG, "Modem Disconnect from PPP Server");
            xEventGroupSetBits(connection_events, DISCONNECT_BIT);
        } else if (event_id == IP_EVENT_GOT_IP6) {
            ESP_LOGI(TAG, "GOT IPv6 event!");
            ip_event_got_ip6_t *event = (ip_event_got_ip6_t *)event_data;
            ESP_LOGI(TAG, "Got IPv6 address " IPV6STR, IPV62STR(event->ip6_info.ip));
        }
    } else if (event_base == ESP_MODEM_EVENT) {
        ESP_LOGI(TAG, "Modem event! %d", event_id);
        switch (event_id) {
            case ESP_MODEM_EVENT_PPP_START:
                ESP_LOGI(TAG, "Modem PPP Started");
                break;
            case ESP_MODEM_EVENT_PPP_STOP:
                ESP_LOGI(TAG, "Modem PPP Stopped");
                break;
            default:
                break;
        }
    } else if (event_base == NETIF_PPP_STATUS) {
        ESP_LOGI(TAG, "PPP netif event! %d", event_id);
        if (event_id == NETIF_PPP_ERRORCONNECT) {
            xEventGroupSetBits(connection_events, DISCONNECT_BIT);
        }
    }
}

esp_netif_t *esp_gateway_modem_init(void)
{
    EventGroupHandle_t connection_events = xEventGroupCreate();

    // init the DTE
    esp_modem_dte_config_t dte_config = ESP_MODEM_DTE_DEFAULT_CONFIG();
    dte_config.tx_io_num = GPIO_NUM_32;
    dte_config.rx_io_num = GPIO_NUM_33;
    dte_config.event_task_stack_size = 4096;
    dte_config.rx_buffer_size = 16384;
    dte_config.tx_buffer_size = 16384;
    dte_config.event_queue_size = 256;
    dte_config.event_task_priority = 15;
    esp_modem_dce_config_t dce_config = ESP_MODEM_DCE_DEFAULT_CONFIG(CONFIG_EXAMPLE_MODEM_PPP_APN);
    dce_config.populate_command_list = true;
    esp_netif_config_t ppp_netif_config = ESP_NETIF_DEFAULT_PPP();


    // Initialize esp-modem units, DTE, DCE, ppp-netif
    esp_modem_dte_t *dte = esp_modem_dte_new(&dte_config);
#if defined(CONFIG_EXAMPLE_MODEM_CUSTOM_BOARD)
    esp_modem_dce_t *dce = sim7600_board_create(&dce_config);
#else
    esp_modem_dce_t *dce = esp_modem_dce_new(&dce_config);
#endif
    esp_netif_t *ppp_netif = esp_netif_new(&ppp_netif_config);

    assert(ppp_netif);

    ESP_ERROR_CHECK(esp_modem_set_event_handler(dte, on_modem_event, ESP_EVENT_ANY_ID, connection_events));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, on_modem_event, connection_events));
    ESP_ERROR_CHECK(esp_event_handler_register(NETIF_PPP_STATUS, ESP_EVENT_ANY_ID, on_modem_event, connection_events));

    ESP_ERROR_CHECK(esp_modem_default_attach(dte, dce, ppp_netif));

    ESP_ERROR_CHECK(esp_modem_default_start(dte)); // use retry
    ESP_ERROR_CHECK(esp_modem_start_ppp(dte));
    /* Wait for the first connection */
    EventBits_t bits;
    do {
        bits = xEventGroupWaitBits(connection_events, (CONNECT_BIT | DISCONNECT_BIT), pdTRUE, pdFALSE, portMAX_DELAY);
        if (bits&DISCONNECT_BIT) {
            // restart the PPP mode in DTE
            ESP_ERROR_CHECK(esp_modem_stop_ppp(dte));
            ESP_ERROR_CHECK(esp_modem_start_ppp(dte));
        }
    } while ((bits&CONNECT_BIT) == 0);

    return ppp_netif;
}

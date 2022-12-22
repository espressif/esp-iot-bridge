// Copyright 2022 Espressif Systems (Shanghai) PTE LTD
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

#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_netif_ppp.h"

// #include "esp_modem_dce.h"
#include "esp_modem_recov_helper.h"
#include "esp_modem_dce_common_commands.h"
#include "esp_bridge_modem.h"

#include "esp_modem.h"
#include "led_indicator.h"
#include "sdkconfig.h"
#if CONFIG_BRIDGE_MODEM_USB
#include "esp_usbh_cdc.h"
#endif

#define MODULE_BOOT_TIME 8
static const char *TAG = "bridge_modem";

static const int CONNECT_BIT = BIT0;
static const int DISCONNECT_BIT = BIT1;

led_indicator_handle_t led_system_handle = NULL;
static led_indicator_handle_t led_wifi_handle = NULL;
static led_indicator_handle_t led_4g_handle = NULL;
static int active_station_num = 0;
typedef struct {
    esp_modem_dte_t *dte;
    EventGroupHandle_t events_handle;
} ip_event_arg_t;

extern esp_modem_dce_t *sim7600_board_create(esp_modem_dce_config_t *config);
extern esp_modem_dce_t *usb_modem_board_create(esp_modem_dce_config_t *config);

static void on_modem_event(void *arg, esp_event_base_t event_base,
                           int32_t event_id, void *event_data)
{
    if (event_base == IP_EVENT) {
        ip_event_arg_t *p_ip_event_arg = (ip_event_arg_t *)arg;
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

            if (led_4g_handle) {
                led_indicator_start(led_4g_handle, BLINK_CONNECTED);
            }

            xEventGroupClearBits(p_ip_event_arg->events_handle, DISCONNECT_BIT);
            xEventGroupSetBits(p_ip_event_arg->events_handle, CONNECT_BIT);
        } else if (event_id == IP_EVENT_PPP_LOST_IP) {
            ESP_LOGI(TAG, "Modem Disconnect from PPP Server");

            if (led_4g_handle) {
                led_indicator_stop(led_4g_handle, BLINK_CONNECTED);
            }

            if (led_4g_handle) {
                led_indicator_start(led_4g_handle, BLINK_CONNECTING);
            }

            xEventGroupClearBits(p_ip_event_arg->events_handle, CONNECT_BIT);
            xEventGroupSetBits(p_ip_event_arg->events_handle, DISCONNECT_BIT);
            esp_modem_stop_ppp(p_ip_event_arg->dte);
            esp_modem_start_ppp(p_ip_event_arg->dte);
            ESP_LOGW(TAG, "Lost IP, Restart PPP");
        } else if (event_id == IP_EVENT_GOT_IP6) {
            ESP_LOGI(TAG, "GOT IPv6 event!");
            ip_event_got_ip6_t *event = (ip_event_got_ip6_t *)event_data;
            ESP_LOGI(TAG, "Got IPv6 address " IPV6STR, IPV62STR(event->ip6_info.ip));
        }
    } else if (event_base == ESP_MODEM_EVENT) {
        switch (event_id) {
        case ESP_MODEM_EVENT_PPP_START:
            ESP_LOGI(TAG, "Modem PPP Started");
            break;

        case ESP_MODEM_EVENT_PPP_STOP:
            ESP_LOGI(TAG, "Modem PPP Stopped");
            break;

        default:
            ESP_LOGW(TAG, "Modem event! %d", event_id);
            break;
        }
    } else if (event_base == WIFI_EVENT) {
        //wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        switch (event_id) {
        case WIFI_EVENT_AP_STACONNECTED:
            if (++active_station_num > 0) {
                if (led_wifi_handle) {
                    led_indicator_start(led_wifi_handle, BLINK_CONNECTED);
                }
            }

            //ESP_LOGI(TAG, "station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
            break;

        case WIFI_EVENT_AP_STADISCONNECTED:
            if (--active_station_num == 0) {
                if (led_wifi_handle) {
                    led_indicator_stop(led_wifi_handle, BLINK_CONNECTED);
                }

                if (led_wifi_handle) {
                    led_indicator_start(led_wifi_handle, BLINK_CONNECTING);
                }
            }

            //ESP_LOGI(TAG, "station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
            break;

        default:
            break;
        }
    } else if (event_base == NETIF_PPP_STATUS) {
        if (event_id < NETIF_PP_PHASE_OFFSET) {
            ESP_LOGE(TAG, "PPP netif event = %d", event_id);
            ESP_LOGE(TAG, "Just Force restart!");
            esp_restart();
        } else {
            ESP_LOGW(TAG, "PPP netif event = %d", event_id);
        }
    }
}

#if CONFIG_BRIDGE_MODEM_UART
esp_netif_t *esp_bridge_modem_init(modem_config_t *config)
{
    EventGroupHandle_t connection_events = xEventGroupCreate();

    // init the DTE
    esp_modem_dte_config_t dte_config = ESP_MODEM_DTE_DEFAULT_CONFIG();
    dte_config.tx_io_num = CONFIG_ESP_BRIDGE_MODEM_TX_GPIO;
    dte_config.rx_io_num = CONFIG_ESP_BRIDGE_MODEM_RX_GPIO;
    dte_config.baud_rate = CONFIG_ESP_BRIDGE_MODEM_BAUD_RATE;
    dte_config.event_task_stack_size = 4096;
    dte_config.rx_buffer_size = 16384;
    dte_config.tx_buffer_size = 16384;
    dte_config.event_queue_size = 256;
    dte_config.event_task_priority = 15;
    esp_modem_dce_config_t dce_config = ESP_MODEM_DCE_DEFAULT_CONFIG(CONFIG_BRIDGE_MODEM_PPP_APN);
    dce_config.populate_command_list = true;
    esp_netif_config_t ppp_netif_config = ESP_NETIF_DEFAULT_PPP();

    // Initialize esp-modem units, DTE, DCE, ppp-netif
    esp_modem_dte_t *dte = esp_modem_dte_new(&dte_config);
#if defined(CONFIG_BRIDGE_MODEM_CUSTOM_BOARD)
    esp_modem_dce_t *dce = sim7600_board_create(&dce_config);
#else
    esp_modem_dce_t *dce = esp_modem_dce_new(&dce_config);
#endif
    esp_netif_t *ppp_netif = esp_netif_new(&ppp_netif_config);
    assert(ppp_netif);

    ip_event_arg_t ip_event_arg;
    ip_event_arg.dte = dte;
    ip_event_arg.events_handle = connection_events;

    ESP_ERROR_CHECK(esp_modem_set_event_handler(dte, on_modem_event, ESP_EVENT_ANY_ID, &ip_event_arg));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, on_modem_event, &ip_event_arg));
    ESP_ERROR_CHECK(esp_event_handler_register(NETIF_PPP_STATUS, ESP_EVENT_ANY_ID, on_modem_event, NULL));

    ESP_ERROR_CHECK(esp_modem_default_attach(dte, dce, ppp_netif));

    ESP_ERROR_CHECK(esp_modem_default_start(dte)); // use retry
    ESP_ERROR_CHECK(esp_modem_start_ppp(dte));
    /* Wait for the first connection */
    EventBits_t bits;

    do {
        bits = xEventGroupWaitBits(connection_events, (CONNECT_BIT | DISCONNECT_BIT), pdTRUE, pdFALSE, portMAX_DELAY);

        if (bits & DISCONNECT_BIT) {
            // restart the PPP mode in DTE
            ESP_ERROR_CHECK(esp_modem_stop_ppp(dte));
            ESP_ERROR_CHECK(esp_modem_start_ppp(dte));
        }
    } while ((bits & CONNECT_BIT) == 0);

    return ppp_netif;
}
#elif CONFIG_BRIDGE_MODEM_USB
esp_netif_t *esp_bridge_modem_init(modem_config_t *config)
{
    led_indicator_config_t led_config = {
        .off_level = 0,
        .mode = LED_GPIO_MODE,
    };

    if (LED_RED_SYSTEM_GPIO) {
        led_system_handle = led_indicator_create(LED_RED_SYSTEM_GPIO, &led_config);
    }

    if (LED_BLUE_WIFI_GPIO) {
        led_wifi_handle = led_indicator_create(LED_BLUE_WIFI_GPIO, &led_config);
    }

    if (LED_GREEN_4GMODEM_GPIO) {
        led_4g_handle = led_indicator_create(LED_GREEN_4GMODEM_GPIO, &led_config);
    }

    EventGroupHandle_t connection_events = xEventGroupCreate();

    // init the USB DTE
    esp_modem_dte_config_t dte_config = ESP_MODEM_DTE_DEFAULT_CONFIG();
    dte_config.rx_buffer_size = config->rx_buffer_size; //rx ringbuffer for usb transfer
    dte_config.tx_buffer_size = config->tx_buffer_size; //tx ringbuffer for usb transfer
    dte_config.line_buffer_size = config->line_buffer_size;
    dte_config.event_task_stack_size = config->event_task_stack_size; //task to handle usb rx data
    dte_config.event_task_priority = config->event_task_priority; //task to handle usb rx data
    esp_modem_dce_config_t dce_config = ESP_MODEM_DCE_DEFAULT_CONFIG(CONFIG_BRIDGE_MODEM_PPP_APN);
    dce_config.populate_command_list = true;
    esp_netif_config_t ppp_netif_config = ESP_NETIF_DEFAULT_PPP();

    // Initialize esp-modem units, DTE, DCE, ppp-netif
    esp_modem_dte_t *dte = esp_modem_dte_new(&dte_config);
    assert(dte != NULL);
    esp_modem_dce_t *dce = usb_modem_board_create(&dce_config);
    assert(dce != NULL);
    esp_netif_t *ppp_netif = esp_netif_new(&ppp_netif_config);
    assert(ppp_netif != NULL);

    ip_event_arg_t ip_event_arg;
    ip_event_arg.dte = dte;
    ip_event_arg.events_handle = connection_events;

    ESP_ERROR_CHECK(esp_modem_set_event_handler(dte, on_modem_event, ESP_EVENT_ANY_ID, &ip_event_arg));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, on_modem_event, &ip_event_arg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, on_modem_event, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(NETIF_PPP_STATUS, ESP_EVENT_ANY_ID, on_modem_event, NULL));

    if (led_4g_handle) {
        led_indicator_stop(led_4g_handle, BLINK_CONNECTED);
    }

    if (led_4g_handle) {
        led_indicator_start(led_4g_handle, BLINK_CONNECTING);
    }

    if (led_wifi_handle) {
        led_indicator_stop(led_wifi_handle, BLINK_CONNECTED);
    }

    if (led_wifi_handle) {
        led_indicator_start(led_wifi_handle, BLINK_CONNECTING);
    }

    ESP_ERROR_CHECK(esp_modem_default_attach(dte, dce, ppp_netif));
    ESP_ERROR_CHECK(esp_modem_default_start(dte));

    esp_err_t ret = ESP_OK;
    vTaskDelay(pdMS_TO_TICKS(2000));
    ret = esp_modem_start_ppp(dte);
    int dial_retry_times = CONFIG_MODEM_DIAL_RERTY_TIMES;

    while (ret != ESP_OK && --dial_retry_times > 0) {
        ret = esp_modem_stop_ppp(dte);
        vTaskDelay(pdMS_TO_TICKS(2000));
        ret = esp_modem_start_ppp(dte);
        ESP_LOGI(TAG, "re-start ppp, retry=%d", dial_retry_times);
    };

    if (ret == ESP_OK) {
        if (led_4g_handle) {
            led_indicator_start(led_4g_handle, BLINK_CONNECTED);
        }
    } else {
        if (led_system_handle) {
            led_indicator_start(led_system_handle, BLINK_CONNECTED);    //solid red, internal error
        }

        ESP_LOGE(TAG, "4G modem dial failed");
        ESP_LOGE(TAG, "Force restart!");
        esp_restart();
    }

    /* Wait for the first connection */
    xEventGroupWaitBits(connection_events, CONNECT_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    return ppp_netif;
}
#endif

esp_timer_handle_t modem_powerup_timer;
static void modem_powerup_timer_callback(void *arg)
{
    esp_netif_t *ppp_netif = (esp_netif_t *)arg;
    modem_config_t modem_config = MODEM_DEFAULT_CONFIG();
    ESP_LOGI(TAG, "====================================");
    ESP_LOGI(TAG, "     ESP 4G Cat.1 Wi-Fi Router");
    ESP_LOGI(TAG, "====================================");

    /* Initialize modem board. Dial-up internet */
    ppp_netif = esp_bridge_modem_init(&modem_config);
    assert(ppp_netif != NULL);

    ESP_ERROR_CHECK(esp_timer_delete(modem_powerup_timer));
}

esp_netif_t *esp_bridge_create_modem_netif(esp_netif_ip_info_t *custom_ip_info, uint8_t custom_mac[6], bool data_forwarding, bool enable_dhcps)
{
    esp_netif_t *netif = NULL;

    if (data_forwarding || enable_dhcps) {
        return netif;
    }

    ESP_LOGW(TAG, "Force reset 4g board");
    gpio_config_t io_config = {
        .pin_bit_mask = BIT64(MODEM_RESET_GPIO),
        .mode = GPIO_MODE_OUTPUT
    };
    gpio_config(&io_config);
    gpio_set_level(MODEM_RESET_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(500));
    gpio_set_level(MODEM_RESET_GPIO, 1);

    /* Waitting for modem powerup */
    const esp_timer_create_args_t modem_powerup_timer_args = {
        .callback = &modem_powerup_timer_callback,
        .name = "modem-powerup",
        .arg = netif
    };

    ESP_ERROR_CHECK(esp_timer_create(&modem_powerup_timer_args, &modem_powerup_timer));
    ESP_ERROR_CHECK(esp_timer_start_once(modem_powerup_timer, MODULE_BOOT_TIME * 1000000));
    return netif;
}

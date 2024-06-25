/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <inttypes.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_netif_ppp.h"
#include "esp_modem_api.h"
#include "esp_log.h"
#include "esp_bridge.h"
#include "esp_bridge_internal.h"

#define MODULE_BOOT_TIME_MS     5000
#if defined(CONFIG_BRIDGE_FLOW_CONTROL_NONE)
#define BRIDGE_FLOW_CONTROL ESP_MODEM_FLOW_CONTROL_NONE
#elif defined(CONFIG_BRIDGE_FLOW_CONTROL_SW)
#define BRIDGE_FLOW_CONTROL ESP_MODEM_FLOW_CONTROL_SW
#elif defined(CONFIG_BRIDGE_FLOW_CONTROL_HW)
#define BRIDGE_FLOW_CONTROL ESP_MODEM_FLOW_CONTROL_HW
#endif

#if CONFIG_BRIDGE_MODEM_DEVICE_BG96 == 1
#define ESP_BRIDGE_MODEM_DEVICE         ESP_MODEM_DCE_BG96
#elif CONFIG_BRIDGE_MODEM_DEVICE_SIM800 == 1
#define ESP_BRIDGE_MODEM_DEVICE         ESP_MODEM_DCE_SIM800
#elif CONFIG_BRIDGE_MODEM_DEVICE_SIM7000 == 1
#define ESP_BRIDGE_MODEM_DEVICE         ESP_MODEM_DCE_SIM7000
#elif CONFIG_BRIDGE_MODEM_DEVICE_SIM7070 == 1
#define ESP_BRIDGE_MODEM_DEVICE         ESP_MODEM_DCE_SIM7070
#elif CONFIG_BRIDGE_MODEM_DEVICE_SIM7600 == 1
#define ESP_BRIDGE_MODEM_DEVICE         ESP_MODEM_DCE_SIM7600
#endif

static const char *TAG = "bridge_modem";
static EventGroupHandle_t event_group = NULL;
static const int CONNECT_BIT = BIT0;
static const int USB_DISCONNECTED_BIT = BIT3; // Used only with USB DTE but we define it unconditionally, to avoid too many #ifdefs in the code

#if (defined(CONFIG_BRIDGE_SERIAL_VIA_USB)) && (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 0))
#include "esp_modem_usb_c_api.h"
#include "esp_modem_usb_config.h"
#include "freertos/task.h"
static void usb_terminal_error_handler(esp_modem_terminal_error_t err)
{
    if (err == ESP_MODEM_TERMINAL_DEVICE_GONE) {
        ESP_LOGI(TAG, "USB modem disconnected");
        assert(event_group);
        xEventGroupSetBits(event_group, USB_DISCONNECTED_BIT);
    }
}
#endif

static void on_ppp_changed(void *arg, esp_event_base_t event_base,
                           int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "PPP state changed event %"PRId32"", event_id);
    if (event_id == NETIF_PPP_ERRORUSER) {
        /* User interrupted event from esp-netif */
        esp_netif_t *netif = event_data;
        ESP_LOGI(TAG, "User interrupted event from netif:%p", netif);
    }
}

static void on_ip_event(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "IP event! %"PRId32"", event_id);
    if (event_id == IP_EVENT_PPP_GOT_IP) {
        esp_netif_dns_info_t dns_info;

        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        esp_netif_t *netif = event->esp_netif;

        ESP_LOGI(TAG, "Modem Connect to PPP Server");
        ESP_LOGI(TAG, "~~~~~~~~~~~~~~");
        ESP_LOGI(TAG, "IP          : " IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "Netmask     : " IPSTR, IP2STR(&event->ip_info.netmask));
        ESP_LOGI(TAG, "Gateway     : " IPSTR, IP2STR(&event->ip_info.gw));
        esp_netif_get_dns_info(netif, 0, &dns_info);
        ESP_LOGI(TAG, "Name Server1: " IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
        esp_netif_get_dns_info(netif, 1, &dns_info);
        ESP_LOGI(TAG, "Name Server2: " IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
        ESP_LOGI(TAG, "~~~~~~~~~~~~~~");
        xEventGroupSetBits(event_group, CONNECT_BIT);
        esp_bridge_update_dns_info(event->esp_netif, NULL);
        ESP_LOGI(TAG, "GOT ip event!!!");
    } else if (event_id == IP_EVENT_PPP_LOST_IP) {
        ESP_LOGI(TAG, "Modem Disconnect from PPP Server");
        IOT_BRIDGE_NAPT_TABLE_CLEAR();
    } else if (event_id == IP_EVENT_GOT_IP6) {
        ESP_LOGI(TAG, "GOT IPv6 event!");

        ip_event_got_ip6_t *event = (ip_event_got_ip6_t *)event_data;
        ESP_LOGI(TAG, "Got IPv6 address " IPV6STR, IPV62STR(event->ip6_info.ip));
    }
}

esp_netif_t *esp_bridge_create_modem_netif(esp_netif_ip_info_t *custom_ip_info, uint8_t custom_mac[6], bool data_forwarding, bool enable_dhcps)
{
    esp_netif_t *netif = NULL;

    if (data_forwarding || enable_dhcps) {
        return netif;
    }

    ESP_LOGW(TAG, "Force reset 4g board");
    gpio_config_t io_config = {
        .pin_bit_mask = BIT64(CONFIG_MODEM_RESET_GPIO),
        .mode = GPIO_MODE_OUTPUT
    };
    gpio_config(&io_config);
    gpio_set_level(CONFIG_MODEM_RESET_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(500));
    gpio_set_level(CONFIG_MODEM_RESET_GPIO, 1);

    vTaskDelay(pdMS_TO_TICKS(MODULE_BOOT_TIME_MS));

    event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &on_ip_event, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(NETIF_PPP_STATUS, ESP_EVENT_ANY_ID, &on_ppp_changed, NULL));

    /* Configure the PPP netif */
    esp_modem_dce_config_t dce_config = ESP_MODEM_DCE_DEFAULT_CONFIG(CONFIG_BRIDGE_MODEM_PPP_APN);
    esp_netif_config_t netif_ppp_config = ESP_NETIF_DEFAULT_PPP();
    esp_netif_t *esp_netif = esp_netif_new(&netif_ppp_config);
    assert(esp_netif);

    /* Configure the DTE */
#if defined(CONFIG_BRIDGE_SERIAL_VIA_UART)
    esp_modem_dte_config_t dte_config = ESP_MODEM_DTE_DEFAULT_CONFIG();
    /* setup UART specific configuration based on kconfig options */
    dte_config.uart_config.tx_io_num = CONFIG_BRIDGE_MODEM_UART_TX_PIN;
    dte_config.uart_config.rx_io_num = CONFIG_BRIDGE_MODEM_UART_RX_PIN;
    dte_config.uart_config.rts_io_num = CONFIG_BRIDGE_MODEM_UART_RTS_PIN;
    dte_config.uart_config.cts_io_num = CONFIG_BRIDGE_MODEM_UART_CTS_PIN;
    dte_config.uart_config.baud_rate = CONFIG_BRIDGE_MODEM_BAUD_RATE;
    dte_config.uart_config.flow_control = BRIDGE_FLOW_CONTROL;
    dte_config.uart_config.rx_buffer_size = CONFIG_BRIDGE_MODEM_UART_RX_BUFFER_SIZE;
    dte_config.uart_config.tx_buffer_size = CONFIG_BRIDGE_MODEM_UART_TX_BUFFER_SIZE;
    dte_config.uart_config.event_queue_size = CONFIG_BRIDGE_MODEM_UART_EVENT_QUEUE_SIZE;
    dte_config.task_stack_size = CONFIG_BRIDGE_MODEM_UART_EVENT_TASK_STACK_SIZE;
    dte_config.task_priority = CONFIG_BRIDGE_MODEM_UART_EVENT_TASK_PRIORITY;
    dte_config.dte_buffer_size = CONFIG_BRIDGE_MODEM_UART_RX_BUFFER_SIZE / 2;

#ifdef ESP_BRIDGE_MODEM_DEVICE
    esp_modem_dce_t *dce = esp_modem_new_dev(ESP_BRIDGE_MODEM_DEVICE, &dte_config, &dce_config, esp_netif);
#else
    ESP_LOGI(TAG, "Initializing esp_modem for a generic module...");
    esp_modem_dce_t *dce = esp_modem_new(&dte_config, &dce_config, esp_netif);
#endif

    assert(dce);
    if (dte_config.uart_config.flow_control == ESP_MODEM_FLOW_CONTROL_HW) {
        esp_err_t err = esp_modem_set_flow_control(dce, 2, 2);  //2/2 means HW Flow Control.
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set the set_flow_control mode");
            esp_modem_destroy(dce);
            esp_netif_destroy(esp_netif);
            vEventGroupDelete(event_group);
            event_group = NULL;
            return NULL;
        }
        ESP_LOGI(TAG, "HW set_flow_control OK");
    }

#elif (defined(CONFIG_BRIDGE_SERIAL_VIA_USB)) && (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 0))

    ESP_LOGI(TAG, "Initializing esp_modem for the BG96 module...");
    struct esp_modem_usb_term_config usb_config = ESP_MODEM_DEFAULT_USB_CONFIG(CONFIG_BRIDGE_MODEM_USB_VID, CONFIG_BRIDGE_MODEM_USB_PID, CONFIG_BRIDGE_MODEM_USB_INTERFACE_NUMBER); // VID, PID and interface num of 4G modem
    const esp_modem_dte_config_t dte_usb_config = ESP_MODEM_DTE_DEFAULT_USB_CONFIG(usb_config);
    ESP_LOGI(TAG, "Waiting for USB device connection...");
    esp_modem_dce_t *dce = esp_modem_new_dev_usb(ESP_MODEM_DCE_BG96, &dte_usb_config, &dce_config, esp_netif);
    assert(dce);
    esp_modem_set_error_cb(dce, usb_terminal_error_handler);
    vTaskDelay(pdMS_TO_TICKS(1000)); // Although the DTE should be ready after USB enumeration, sometimes it fails to respond without this delay

#else
#error Invalid serial connection to modem.
#endif

    xEventGroupClearBits(event_group, CONNECT_BIT | USB_DISCONNECTED_BIT);

#if CONFIG_BRIDGE_NEED_SIM_PIN == 1
    // check if PIN needed
    bool pin_ok = false;
    if (esp_modem_read_pin(dce, &pin_ok) == ESP_OK && pin_ok == false) {
        if (esp_modem_set_pin(dce, CONFIG_BRIDGE_SIM_PIN) == ESP_OK) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        } else {
            abort();
        }
    }
#endif

    int rssi, ber;
    esp_err_t err = esp_modem_get_signal_quality(dce, &rssi, &ber);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_modem_get_signal_quality failed with %d %s", err, esp_err_to_name(err));
        esp_modem_destroy(dce);
        esp_netif_destroy(esp_netif);
        vEventGroupDelete(event_group);
        event_group = NULL;
        return NULL;
    }
    ESP_LOGI(TAG, "Signal quality: rssi=%d, ber=%d", rssi, ber);

    err = esp_modem_set_mode(dce, ESP_MODEM_MODE_DATA);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_modem_set_mode(ESP_MODEM_MODE_DATA) failed with %d", err);
        esp_modem_destroy(dce);
        esp_netif_destroy(esp_netif);
        vEventGroupDelete(event_group);
        event_group = NULL;
        return NULL;
    }
    /* Wait for IP address */
    ESP_LOGI(TAG, "Waiting for IP address");
    xEventGroupWaitBits(event_group, CONNECT_BIT | USB_DISCONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    return esp_netif;
}

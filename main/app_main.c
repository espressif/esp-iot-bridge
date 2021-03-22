#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_log.h"
#include "esp_spi_flash.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/gpio.h"

#include "esp_netif.h"

#include "esp_eth.h"
#include "esp_bt.h"

#include "esp_storage.h"
#include "esp_gateway_wifi.h"
#include "esp_gateway_eth.h"
#include "esp_gateway_modem.h"

#include "led_gpio.h"
#include "button.h"

#define GPIO_LED_BLE   GPIO_NUM_15
#define GPIO_LED_WIFI  GPIO_NUM_2
#define GPIO_LED_MODEM GPIO_NUM_4
#define GPIO_LED_ETH   GPIO_NUM_16

#define GPIO_BUTTON_SW1 GPIO_NUM_34

#define MODEM_IS_OPEN CONFIG_MODEM_IS_OPEN

typedef enum {
    FEAT_TYPE_WIFI,
    FEAT_TYPE_MODEM,
    FEAT_TYPE_ETH,
    FEAT_TYPE_MAX,
} feat_type_t;

static feat_type_t g_feat_type = FEAT_TYPE_WIFI;
static led_handle_t g_led_handle_list[3] = {NULL};

static const char *TAG = "main";

void button_tap_cb(void *arg)
{
    led_gpio_state_write(g_led_handle_list[g_feat_type], LED_GPIO_OFF);
    g_feat_type = (g_feat_type + 1) % FEAT_TYPE_MAX;
    ESP_LOGI(TAG, "button tap, g_feat_type: %d", g_feat_type);
    led_gpio_state_write(g_led_handle_list[g_feat_type], LED_GPIO_ON);
}


static void button_press_3sec_cb(void *arg)
{
    ESP_LOGI(TAG, "button_press_3sec_cb");

    esp_storage_set("feat_type", &g_feat_type, sizeof(feat_type_t));
    led_gpio_state_write(g_led_handle_list[g_feat_type], LED_GPIO_QUICK_BLINK);

    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_restart();
}

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);

    esp_storage_init();
    esp_storage_get("feat_type", &g_feat_type, sizeof(feat_type_t));

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_LOGI(TAG, "feat_type: %d", g_feat_type);

    g_led_handle_list[FEAT_TYPE_WIFI] = led_gpio_create(GPIO_LED_WIFI, LED_GPIO_DARK_HIGH);
    g_led_handle_list[FEAT_TYPE_ETH]  = led_gpio_create(GPIO_LED_ETH, LED_GPIO_DARK_HIGH);
    g_led_handle_list[FEAT_TYPE_MODEM] = led_gpio_create(GPIO_LED_MODEM, LED_GPIO_DARK_HIGH);

    led_gpio_state_write(g_led_handle_list[g_feat_type], LED_GPIO_ON);

    button_handle_t button_handle = button_create(GPIO_BUTTON_SW1, BUTTON_ACTIVE_LOW);

    button_add_tap_cb(button_handle, BUTTON_CB_TAP, button_tap_cb, NULL);
    button_add_press_cb(button_handle, 3, button_press_3sec_cb, NULL);

    switch (g_feat_type) {
        case FEAT_TYPE_WIFI:
            ESP_LOGI(TAG, "============================");
            ESP_LOGI(TAG, "Wi-Fi Repeater");
            ESP_LOGI(TAG, "============================");
            esp_bt_mem_release(ESP_BT_MODE_BTDM);

            esp_gateway_wifi_init(WIFI_MODE_APSTA);
            esp_gateway_wifi_set(WIFI_MODE_STA, "esp-guest", "esp-guest");
            esp_gateway_wifi_set(WIFI_MODE_AP, "wifi_router", "espressif");
            esp_gateway_wifi_sta_connected(portMAX_DELAY);
            esp_gateway_wifi_napt_enable();
            break;

        case FEAT_TYPE_MODEM: {
#if MODEM_IS_OPEN
            ESP_LOGI(TAG, "============================");
            ESP_LOGI(TAG, "4G Router");
            ESP_LOGI(TAG, "============================");
            esp_bt_mem_release(ESP_BT_MODE_BTDM);

            esp_netif_t *ppp_netif = esp_gateway_modem_init();
            esp_netif_t *ap_netif  = esp_gateway_wifi_init(WIFI_MODE_AP);

            esp_netif_dns_info_t dns;
            ESP_ERROR_CHECK(esp_netif_get_dns_info(ppp_netif, ESP_NETIF_DNS_MAIN, &dns));
            ESP_ERROR_CHECK(esp_gateway_wifi_set_dhcps(ap_netif, dns.ip.u_addr.ip4.addr));

            esp_gateway_wifi_set(WIFI_MODE_AP, "4g_router", "espressif");
            vTaskDelay(pdMS_TO_TICKS(100));

            esp_gateway_wifi_napt_enable();
#endif
            break;
        }

        case FEAT_TYPE_ETH:
            ESP_LOGI(TAG, "============================");
            ESP_LOGI(TAG, "ETH Router");
            ESP_LOGI(TAG, "============================");
            esp_bt_mem_release(ESP_BT_MODE_BTDM);

            if (gpio_get_level(GPIO_BUTTON_SW1)) {
                esp_gateway_wifi_init(WIFI_MODE_AP);
                esp_gateway_wifi_set(WIFI_MODE_AP, "eth_router", "espressif");
            } else {
                esp_gateway_wifi_init(WIFI_MODE_STA);
                esp_gateway_wifi_set(WIFI_MODE_STA, "esp-guest", "esp-guest");
            }

            esp_gateway_eth_init();

            break;

        default:
            break;
    }
}

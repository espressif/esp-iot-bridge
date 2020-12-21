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

#include "esp_storage.h"

#include "led_gpio.h"
#include "button.h"

#define GPIO_LED_BLE   GPIO_NUM_15
#define GPIO_LED_WIFI  GPIO_NUM_2
#define GPIO_LED_MODEM GPIO_NUM_4
#define GPIO_LED_ETH   GPIO_NUM_16

#define GPIO_BUTTON_SW1 GPIO_NUM_34

typedef enum {
    FEAT_TYPE_BLE,
    FEAT_TYPE_WIFI,
    FEAT_TYPE_MODEM,
    FEAT_TYPE_ETH,
    FEAT_TYPE_MAX,
} feat_type_t;

static feat_type_t g_feat_type = FEAT_TYPE_WIFI;
static led_handle_t g_led_handle_list[4] = {NULL};

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

    vTaskDelay(pdMS_TO_TICKS(3000));
    esp_restart();
}

void app_main(void)
{
    // feat_type_t feat_type = FEAT_TYPE_WIFI;

    esp_storage_init();
    esp_storage_get("feat_type", &g_feat_type, sizeof(feat_type_t));

    ESP_LOGI(TAG, "feat_type: %d", g_feat_type);

    g_led_handle_list[FEAT_TYPE_WIFI] = led_gpio_create(GPIO_LED_WIFI, LED_GPIO_DARK_HIGH);
    g_led_handle_list[FEAT_TYPE_BLE]  = led_gpio_create(GPIO_LED_BLE, LED_GPIO_DARK_HIGH);
    g_led_handle_list[FEAT_TYPE_ETH]  = led_gpio_create(GPIO_LED_ETH, LED_GPIO_DARK_HIGH);
    g_led_handle_list[FEAT_TYPE_MODEM] = led_gpio_create(GPIO_LED_MODEM, LED_GPIO_DARK_HIGH);

    led_gpio_state_write(g_led_handle_list[g_feat_type], LED_GPIO_ON);

    button_handle_t button_handle = button_create(GPIO_BUTTON_SW1, BUTTON_ACTIVE_LOW);
    button_add_tap_cb(button_handle, BUTTON_CB_TAP, button_tap_cb, NULL);
    button_add_press_cb(button_handle, 3, button_press_3sec_cb, NULL);

    switch (g_feat_type) {
        case FEAT_TYPE_BLE:

            break;

        case FEAT_TYPE_WIFI:

            break;

        case FEAT_TYPE_MODEM:

            break;

        case FEAT_TYPE_ETH:

            break;

        default:
            break;
    }
}

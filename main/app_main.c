#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_log.h"
#include "esp_spi_flash.h"

#include "led_gpio.h"

#define LED_IO_NUM_0    25
#define LED_IO_NUM_1    26


static const char *TAG = "main";

void app_main(void)
{
    static led_handle_t led_0, led_1;

    led_0 = led_gpio_create(LED_IO_NUM_0, LED_GPIO_DARK_LOW);
    led_1 = led_gpio_create(LED_IO_NUM_1, LED_GPIO_DARK_LOW);
    led_gpio_state_write(led_0, LED_GPIO_ON);
    led_gpio_state_write(led_1, LED_GPIO_OFF);
    ESP_LOGI(TAG, "led0 state:%d", led_gpio_state_read(led_0));
    vTaskDelay(pdMS_TO_TICKS(5000));
    led_gpio_state_write(led_0, LED_GPIO_QUICK_BLINK);
    led_gpio_state_write(led_1, LED_GPIO_OFF);
    ESP_LOGI(TAG, "led0 state:%d", led_gpio_state_read(led_0));
    ESP_LOGI(TAG, "led0 mode:%d", led_gpio_mode_read(led_0));
    vTaskDelay(pdMS_TO_TICKS(5000));
    led_gpio_state_write(led_0, LED_GPIO_ON);
    led_gpio_state_write(led_1, LED_GPIO_ON);
    vTaskDelay(pdMS_TO_TICKS(5000));
    led_gpio_night_duty_write(20);
    led_gpio_mode_write(led_1, LED_GPIO_NIGHT_MODE);
    ESP_LOGI(TAG, "led0 state:%d", led_gpio_state_read(led_0));
    ESP_LOGI(TAG, "led0 mode:%d", led_gpio_mode_read(led_0));
    vTaskDelay(pdMS_TO_TICKS(5000));
    led_gpio_state_write(led_1, LED_GPIO_SLOW_BLINK);
    vTaskDelay(pdMS_TO_TICKS(5000));
    led_gpio_mode_write(led_0, LED_GPIO_NORMAL_MODE);
}

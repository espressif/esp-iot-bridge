// Copyright 2021-2022 Espressif Systems (Shanghai) PTE LTD
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_event.h"

#include "esp_gateway.h"

#include "button.h"
#include "web_server.h"

#define GPIO_BUTTON_SW1    CONFIG_GPIO_BUTTON_SW1

static const char *TAG = "main";

static esp_err_t esp_storage_init(void)
{
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    return ret;
}

static void button_press_3sec_cb(void *arg)
{
    ESP_LOGI(TAG, "Restore factory settings");
    nvs_flash_erase();
    esp_restart();
}

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);

    esp_storage_init();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    button_handle_t button_handle = button_create((gpio_num_t)GPIO_BUTTON_SW1, BUTTON_ACTIVE_LOW);
    button_add_press_cb(button_handle, 3, button_press_3sec_cb, NULL);

    esp_gateway_create_all_netif();

#if defined(CONFIG_GATEWAY_EXTERNAL_NETIF_STATION)
	StartWebServer();
#endif
}

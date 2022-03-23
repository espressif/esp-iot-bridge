#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_event.h"
#include "esp_system.h"

#include "esp_netif.h"
#include "esp_gateway.h"

#include <app_wifi.h>
#include <app_light.h>
#include <app_rainmaker.h>

static const char *TAG = "app_gateway";

static void ip_event_handler(void *arg, esp_event_base_t event_base,
                      int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    ESP_LOGI(TAG, "<IP_EVENT_STA_GOT_IP>IP:" IPSTR, IP2STR(&event->ip_info.ip));
    session_cost_information(TAG, __func__, __LINE__, "GATEWAY_EVENT_PARENT_CONNECTED");
}

esp_err_t app_gateway_enable(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

#if defined(CONFIG_GATEWAY_EXTERNAL_NETIF_STATION)
    esp_gateway_create_station_netif(NULL, NULL, false, false);
#endif

    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));

    app_rainmaker_start();

    /* Start wifi provisioning */
    app_wifi_start(POP_TYPE_RANDOM);

    return ESP_OK;
}
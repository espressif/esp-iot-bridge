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
#include "esp_litemesh.h"

#include <app_wifi.h>
#include <app_light.h>
#include <app_rainmaker.h>

static const char *TAG = "app_gateway";

#define NODE_ID_REPORT_MSG_MAX_RETRY  10

static char Node_ID[13];
static cJSON* node_id_report_process(cJSON *payload, uint32_t seq);
static cJSON* node_id_rsq_process(cJSON *payload, uint32_t seq);

static const esp_litemesh_msg_action_t node_id_action[] = {
    {"node_id_report", "node_id_rsq", node_id_report_process},
    {"node_id_rsq", NULL, node_id_rsq_process},

    {NULL, NULL, NULL} /* Must be NULL terminated */
};

static esp_err_t esp_litemesh_node_id_report(void)
{
    esp_err_t ret = ESP_FAIL;
    cJSON *item = cJSON_CreateObject();

    if (item) {
        cJSON_AddStringToObject(item, "node_id", "Node_ID");
        ret = esp_litemesh_try_sending_msg("node_id_report", "node_id_rsq", NODE_ID_REPORT_MSG_MAX_RETRY, item, &esp_litemesh_send_msg_to_root);
    }

    return ret;
}

static cJSON* node_id_report_process(cJSON *payload, uint32_t seq)
{
    cJSON *found = cJSON_GetObjectItem(payload, "node_id");

    if (found) {
        memset(Node_ID, 0, sizeof(Node_ID));
        memcpy(Node_ID, found->valuestring, sizeof(Node_ID));
    }

    return NULL;
}

static cJSON* node_id_rsq_process(cJSON *payload, uint32_t seq)
{
    esp_litemesh_msg_action_list_unregister(node_id_action);

    return NULL;
}

static void ip_event_handler(void *arg, esp_event_base_t event_base,
                             int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    ESP_LOGI(TAG, "<IP_EVENT_STA_GOT_IP>IP:" IPSTR, IP2STR(&event->ip_info.ip));
    session_cost_information(TAG, __func__, __LINE__, "GATEWAY_EVENT_PARENT_CONNECTED");

    esp_litemesh_msg_action_list_register(node_id_action);

    if (esp_litemesh_get_level() > 1) {
        esp_litemesh_node_id_report();
    }
}

esp_err_t app_gateway_enable(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_gateway_create_all_netif();

    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));

    app_rainmaker_start();

    /* Start wifi provisioning */
    app_wifi_start(POP_TYPE_RANDOM);

    return ESP_OK;
}
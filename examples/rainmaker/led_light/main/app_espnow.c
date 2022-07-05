// Copyright 2022 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "cJSON.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "app_espnow.h"

#define ESPNOW_MAXDELAY 512
#define GROUP_CONTROL_PAYLOAD_LEN  200

extern void esp_rmaker_control_light_by_user(char* data);
extern char group_control_payload[GROUP_CONTROL_PAYLOAD_LEN];

static const char *TAG = "app_espnow";
static QueueHandle_t s_espnow_queue;
static SemaphoreHandle_t sent_msgs_mutex;
static uint8_t s_broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

typedef struct esp_now_msg_send {
    uint32_t retry_times;
    uint32_t max_retry;
    uint32_t msg_len;
    void* sent_msg;
} esp_now_msg_send_t;

static esp_now_msg_send_t* sent_msgs;

/* Parse received ESPNOW data. */
int espnow_data_parse(uint8_t *data, uint16_t data_len, uint32_t *seq, uint8_t *payload)
{
    app_espnow_data_t *buf = (app_espnow_data_t *)data;

    if (data_len < sizeof(app_espnow_data_t)) {
        ESP_LOGE(TAG, "Receive ESPNOW data too short, len:%d", data_len);
        return -1;
    }

    *seq = buf->seq;
    memcpy(payload, buf->payload, data_len - 4);

    return -1;
}

/* Prepare ESPNOW data to be sent. */
void espnow_data_prepare(uint8_t *buf, uint8_t* payload, size_t payload_len)
{
    static uint32_t seq_num = 0;
    app_espnow_data_t *temp = (app_espnow_data_t*)buf;
    temp->seq = ++seq_num;
    memcpy(temp->payload, payload, payload_len);
}

static void esp_now_send_timer_cb(TimerHandle_t timer)
{
    if (esp_mesh_lite_get_level() == 1) {
        xSemaphoreTake(sent_msgs_mutex, portMAX_DELAY);
        if (sent_msgs->max_retry > sent_msgs->retry_times) {
            sent_msgs->retry_times++;
            if (sent_msgs->sent_msg) {
                if (esp_now_send(s_broadcast_mac, sent_msgs->sent_msg, sent_msgs->msg_len) != ESP_OK) {
                    ESP_LOGE(TAG, "Send error");
                }
            }
        } else {
            if (sent_msgs->max_retry) {
                sent_msgs->retry_times = 0;
                sent_msgs->max_retry = 0;
                sent_msgs->msg_len = 0;
                if (sent_msgs->sent_msg) {
                    free(sent_msgs->sent_msg);
                    sent_msgs->sent_msg = NULL;
                }
            }
        }
        xSemaphoreGive(sent_msgs_mutex);
    }
}

void esp_now_send_group_control(uint8_t* payload)
{
    size_t payload_len = strlen((char*)payload);
    uint8_t *buf = calloc(1, payload_len + 4);
    espnow_data_prepare(buf, payload, payload_len);
    if (esp_now_send(s_broadcast_mac, buf, payload_len + 4) != ESP_OK) {
        ESP_LOGE(TAG, "Send error");
    }

    xSemaphoreTake(sent_msgs_mutex, portMAX_DELAY);
    sent_msgs->retry_times = 0;
    sent_msgs->max_retry = 2;
    sent_msgs->msg_len = payload_len + 4;
    sent_msgs->sent_msg = buf;
    xSemaphoreGive(sent_msgs_mutex);
}

void esp_now_remove_send_msgs(void)
{
    xSemaphoreTake(sent_msgs_mutex, portMAX_DELAY);
    if (sent_msgs->max_retry) {
        sent_msgs->retry_times = 0;
        sent_msgs->max_retry = 0;
        sent_msgs->msg_len = 0;
        if (sent_msgs->sent_msg) {
            free(sent_msgs->sent_msg);
            sent_msgs->sent_msg = NULL;
        }
    }
    xSemaphoreGive(sent_msgs_mutex);
}

/* ESPNOW sending or receiving callback function is called in WiFi task.
 * Users should not do lengthy operations from this task. Instead, post
 * necessary data to a queue and handle it from a lower priority task. */
static void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    app_espnow_event_t evt;
    app_espnow_event_send_cb_t *send_cb = &evt.info.send_cb;

    if (mac_addr == NULL) {
        ESP_LOGE(TAG, "Send cb arg error");
        return;
    }

    evt.id = APP_ESPNOW_SEND_CB;
    memcpy(send_cb->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    send_cb->status = status;
}

static void espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len)
{
    app_espnow_event_t evt;
    app_espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;

    if (mac_addr == NULL || data == NULL || len <= 0) {
        ESP_LOGE(TAG, "Receive cb arg error");
        return;
    }

    evt.id = APP_ESPNOW_RECV_CB;
    memcpy(recv_cb->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    recv_cb->data = malloc(len);
    if (recv_cb->data == NULL) {
        ESP_LOGE(TAG, "Malloc receive data fail");
        return;
    }
    memcpy(recv_cb->data, data, len);
    recv_cb->data_len = len;
    if (xQueueSend(s_espnow_queue, &evt, ESPNOW_MAXDELAY) != pdTRUE) {
        ESP_LOGW(TAG, "Send receive queue fail");
        free(recv_cb->data);
    }
}

static void espnow_task(void *pvParameter)
{
    app_espnow_event_t evt;
    static uint32_t recv_seq = 0;

    ESP_LOGI(TAG, "Start espnow task");

    while (xQueueReceive(s_espnow_queue, &evt, portMAX_DELAY) == pdTRUE) {
        switch (evt.id) {
            case APP_ESPNOW_RECV_CB:
            {
                app_espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;
                uint32_t seq_temp;
                memset(group_control_payload, 0x0, GROUP_CONTROL_PAYLOAD_LEN);
                espnow_data_parse(recv_cb->data, recv_cb->data_len, &seq_temp, (uint8_t*)group_control_payload);
                ESP_LOGI(TAG, "Receive broadcast data from: "MACSTR", len: %d, seq: %u", MAC2STR(recv_cb->mac_addr), recv_cb->data_len, seq_temp);

                if (esp_mesh_lite_get_level() == 1) {
                    if (seq_temp == recv_seq) {
                        // cancel resend
                        esp_now_remove_send_msgs();
                    } else {
                        // repeat
                        recv_seq = seq_temp;
                    }
                    free(recv_cb->data);
                    break;
                }

                if (seq_temp > recv_seq) {
                    recv_seq = seq_temp;

                    // Data Forward
                    if (esp_now_send(s_broadcast_mac, recv_cb->data, recv_cb->data_len) != ESP_OK) {
                        ESP_LOGE(TAG, "Send error");
                    }
                    ESP_LOGI(TAG, "Receive broadcast data from: "MACSTR", len: %d, seq: %u", MAC2STR(recv_cb->mac_addr), recv_cb->data_len, recv_seq);

                    cJSON *rmaker_data_js = cJSON_Parse((const char*)group_control_payload);
                    cJSON *light_js = cJSON_GetObjectItem(rmaker_data_js, "Light");
                    if (light_js) {
                        cJSON *group_id_js = cJSON_GetObjectItem(light_js, "group_id");
                        if (group_id_js) {
                            if (group_id_js->valueint && (esp_rmaker_is_my_group_id(group_id_js->valueint) == false)) {
                                ESP_LOGW(TAG, "The Group_id[%d] does not belong to the device, and control information is ignored", group_id_js->valueint);
                            } else {
                                esp_rmaker_control_light_by_user(group_control_payload);
                            }
                            cJSON_Delete(rmaker_data_js);
                        }
                    }
                } else {
                    ESP_LOGI(TAG, "repeat");
                }

                free(recv_cb->data);
                break;
            }
            default:
                ESP_LOGE(TAG, "Callback type error: %d", evt.id);
                break;
        }
    }
}

esp_err_t app_espnow_init(void)
{
    app_espnow_send_param_t *send_param = NULL;

    s_espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(app_espnow_event_t));
    if (s_espnow_queue == NULL) {
        ESP_LOGE(TAG, "Create mutex fail");
        return ESP_FAIL;
    }

    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_send_cb(espnow_send_cb) );
    ESP_ERROR_CHECK( esp_now_register_recv_cb(espnow_recv_cb) );

    /* Set primary master key. */
    ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) );

    /* Add broadcast peer information to peer list. */
    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    if (peer == NULL) {
        ESP_LOGE(TAG, "Malloc peer information fail");
        vSemaphoreDelete(s_espnow_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = 0;
    if (esp_mesh_lite_get_level() > 1) {
        peer->ifidx = ESP_IF_WIFI_STA;
        // esp_wifi_config_espnow_rate(ESP_IF_WIFI_STA, WIFI_PHY_RATE_11M_L);
    } else {
        peer->ifidx = ESP_IF_WIFI_AP;
        // esp_wifi_config_espnow_rate(ESP_IF_WIFI_AP, WIFI_PHY_RATE_11M_L);
    }
    peer->encrypt = false;
    memcpy(peer->peer_addr, s_broadcast_mac, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK( esp_now_add_peer(peer) );
    free(peer);

    xTaskCreate(espnow_task, "espnow_task", 2048, send_param, 4, NULL);

    sent_msgs = (esp_now_msg_send_t*)malloc(sizeof(esp_now_msg_send_t));
    sent_msgs->max_retry = 0;
    sent_msgs->msg_len = 0;
    sent_msgs->retry_times = 0;
    sent_msgs->sent_msg = NULL;
    sent_msgs_mutex = xSemaphoreCreateMutex();

    TimerHandle_t esp_now_send_timer = xTimerCreate("esp_now_send_timer", 100 / portTICK_PERIOD_MS, pdTRUE,
            NULL, esp_now_send_timer_cb);
    xTimerStart(esp_now_send_timer, portMAX_DELAY);

    return ESP_OK;
}

void espnow_deinit(app_espnow_send_param_t *send_param)
{
    free(send_param->buffer);
    free(send_param);
    vSemaphoreDelete(s_espnow_queue);
    esp_now_deinit();
}
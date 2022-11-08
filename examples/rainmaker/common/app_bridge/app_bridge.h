/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma once
#include <stdint.h>
#include <esp_err.h>

#include "esp_netif.h"
#include "esp_mesh_lite.h"

typedef enum {
    ESP_MESH_LITE_EVENT_CHILD_NODE_JOIN = ESP_MESH_LITE_EVENT_MAX,
    ESP_MESH_LITE_EVENT_CHILD_NODE_LEAVE,
} mesh_lite_event_child_node_info_t;

/* Enable ESP Bridge in the application
 *
 * @return ESP_OK on success.
 * @return error in case of failure.
 */
esp_err_t app_bridge_enable(void);

void app_rmaker_mesh_lite_service_creat(void);

esp_err_t esp_mesh_lite_report_child_info(void);

void app_mesh_lite_comm_report_info_to_parent(void);

void esp_rmaker_mesh_lite_level_update_and_report(uint8_t level);

void esp_rmaker_mesh_lite_self_ip_update_and_report(esp_netif_ip_info_t* ap_ip_info, esp_netif_ip_info_t* sta_ip_info);

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

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "esp_mesh_lite_core.h"

#define ROOT    (1)

typedef enum {
    ESP_MESH_LITE_EVENT_NODE_JOIN = ESP_MESH_LITE_EVENT_CORE_MAX,
    ESP_MESH_LITE_EVENT_NODE_LEAVE,
    ESP_MESH_LITE_EVENT_NODE_CHANGE,
    ESP_MESH_LITE_EVENT_MAX,
} esp_mesh_lite_event_node_info_t;

/**
 * @brief Initialization Mesh-Lite.
 * 
 */
void esp_mesh_lite_init(esp_mesh_lite_config_t* config);

typedef struct  esp_mesh_lite_node_info {
    uint8_t level;
    char mac[MAC_MAX_LEN];
    char ip[IP_MAX_LEN];
} esp_mesh_lite_node_info_t;

#if CONFIG_MESH_LITE_NODE_INFO_REPORT
/**
 * @brief child nodes report mac and level information to the root node.
 * 
 */
esp_err_t esp_mesh_lite_report_info(void);
#endif /* CONFIG_MESH_LITE_NODE_INFO_REPORT */

esp_err_t esp_mesh_lite_nvs_get_str(const char* name, char* str);
esp_err_t esp_mesh_lite_nvs_set_str(const char* name, const char* str);

#ifdef __cplusplus
}
#endif



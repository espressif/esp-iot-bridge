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

#include "esp_wifi_types.h"
#include "cJSON.h"

extern const char* ESP_MESH_LITE_EVENT;

#define MAC_MAX_LEN                   (18)
#define IP_MAX_LEN                    (16)

/* Definitions for error constants. */
#define ESP_ERR_DUPLICATE_ADDITION    0x110   /*!< Duplicate addition */

#ifdef CONFIG_MESH_LITE_SOFTAP_SSID_END_WITH_THE_MAC
#define ESP_MESH_LITE_SOFTAP_SSID_END_WITH_THE_MAC CONFIG_MESH_LITE_SOFTAP_SSID_END_WITH_THE_MAC
#else
#define ESP_MESH_LITE_SOFTAP_SSID_END_WITH_THE_MAC 0
#endif

#ifdef CONFIG_JOIN_MESH_WITHOUT_CONFIGURED_WIFI_INFO
#define JOIN_MESH_WITHOUT_CONFIGURED_WIFI_INFO CONFIG_JOIN_MESH_WITHOUT_CONFIGURED_WIFI_INFO
#else
#define JOIN_MESH_WITHOUT_CONFIGURED_WIFI_INFO 0
#endif

#ifdef CONFIG_JOIN_MESH_IGNORE_ROUTER_STATUS
#define JOIN_MESH_IGNORE_ROUTER_STATUS CONFIG_JOIN_MESH_IGNORE_ROUTER_STATUS
#else
#define JOIN_MESH_IGNORE_ROUTER_STATUS 0
#endif

#if ESP_MESH_LITE_SOFTAP_SSID_END_WITH_THE_MAC
#define SSID_MAC_LEN    7  // _XXYYZZ
#else
#define SSID_MAC_LEN    0
#endif

#define STATIC_ASSERT(condition) typedef char p__LINE__[ (condition) ? 1 : -1];
STATIC_ASSERT((sizeof(CONFIG_MESH_LITE_SOFTAP_SSID) + SSID_MAC_LEN) < (32 + 2))
STATIC_ASSERT(sizeof(CONFIG_MESH_LITE_SOFTAP_PASSWORD) < (63 + 2))

#define ESP_MESH_LITE_DEFAULT_INIT()                                                           \
    {                                                                                          \
        .vendor_id = {CONFIG_MESH_LITE_VENDOR_ID_0, CONFIG_MESH_LITE_VENDOR_ID_1},             \
        .mesh_id = CONFIG_MESH_LITE_ID,                                                        \
        .max_connect_number = CONFIG_MESH_LITE_MAX_CONNECT_NUMBER,                             \
        .max_router_number = CONFIG_MESH_LITE_MAX_ROUTER_NUMBER,                               \
        .max_level = CONFIG_MESH_LITE_MAXIMUM_LEVEL_ALLOWED,                                   \
        .end_with_mac = ESP_MESH_LITE_SOFTAP_SSID_END_WITH_THE_MAC,                            \
        .join_mesh_ignore_router_status = JOIN_MESH_IGNORE_ROUTER_STATUS,                      \
        .join_mesh_without_configured_wifi = JOIN_MESH_WITHOUT_CONFIGURED_WIFI_INFO,           \
        .softap_ssid = CONFIG_MESH_LITE_SOFTAP_SSID,                                           \
        .softap_password = CONFIG_MESH_LITE_SOFTAP_PASSWORD,                                   \
    }

typedef enum {
    ESP_MESH_LITE_EVENT_CORE_STARTED,
    ESP_MESH_LITE_EVENT_CORE_INHERITED_NET_SEGMENT_CHANGED,
    ESP_MESH_LITE_EVENT_CORE_ROUTER_INFO_CHANGED,
    ESP_MESH_LITE_EVENT_CORE_MAX,
} esp_mesh_lite_event_core_t;

typedef struct {
    uint8_t vendor_id[2];
    uint8_t mesh_id;
    uint8_t max_connect_number;
    uint8_t max_router_number;
    uint8_t max_level;
    bool end_with_mac;
    bool join_mesh_ignore_router_status;
    bool join_mesh_without_configured_wifi;
    const char* softap_ssid;
    const char* softap_password;
} esp_mesh_lite_config_t;

typedef cJSON* (*msg_process_cb_t)(cJSON *payload, uint32_t seq);

typedef struct esp_mesh_lite_msg_action {
    const char* type;
    const char* rsp_type;
    msg_process_cb_t process;
} esp_mesh_lite_msg_action_t;


/*****************************************************/
/**************** ESP Wi-Fi Mesh Lite ****************/
/*****************************************************/

/**
  * @brief Check if the network segment is used to avoid conflicts.
  * 
  * @return
  *     - true :be used
  *     - false:not used
  */
bool esp_mesh_lite_network_segment_is_used(uint32_t ip);

/**
 * @brief Initialization Mesh-Lite.
 * 
 * @return
 *     - OK   : successful
 *     - Other: fail
 */
esp_err_t esp_mesh_lite_core_init(esp_mesh_lite_config_t* config);

/**
 * @brief Scan to find a matched node and connect.
 * 
 */
void esp_mesh_lite_connect(void);

/**
 * @brief Set the mesh_lite_id
 *
 * @param[in] mesh_id: Each mesh network should have a different and unique ID.
 * 
 * @param[in] force_update_nvs: Whether to force update the value of mesh_id in nvs.
 * 
 */
void esp_mesh_lite_set_mesh_id(uint8_t mesh_id, bool force_update_nvs);

/**
 * @brief  Set which level this node is only allowed to be
 * 
 * @attention  Please make sure `esp_mesh_lite_init` is called before calling this function.
 *
 * @param[in]  level: 1 ~ CONFIG_MESH_LITE_MAXIMUM_LEVEL_ALLOWED, 0 is invalid.
 * 
 */
esp_err_t esp_mesh_lite_set_allowed_level(uint8_t level);

/**
 * @brief  Set which level this node is not allowed to be used as
 * 
 * @attention  Please make sure `esp_mesh_lite_init` is called before calling this function.
 *
 * @param[in]  level: 1 ~ CONFIG_MESH_LITE_MAXIMUM_LEVEL_ALLOWED, 0 is invalid.
 *
 */
esp_err_t esp_mesh_lite_set_disallowed_level(uint8_t level);

/**
 * @brief  Set router information
 * 
 * @attention  Please make sure `esp_mesh_lite_init` is called before calling this function.
 *
 * @param[in]  conf
 *
 */
esp_err_t esp_mesh_lite_set_router_config(wifi_sta_config_t *conf);

/**
 * @brief  Whether to allow other nodes to join the mesh network.
 * 
 * @attention  Please make sure `esp_mesh_lite_init` is called before calling this function.
 *
 * @param[in]  enable: true -> allow; false -> disallow
 *
 */
esp_err_t esp_mesh_lite_allow_others_to_join(bool enable);

/**
 * @brief  Set argot number.
 * 
 * @attention  Please make sure `esp_mesh_lite_init` is called before calling this function.
 *
 * @param[in]  argot
 *
 */
esp_err_t esp_mesh_lite_set_argot(uint32_t argot);

/**
 * @brief  Set SoftAP information.
 *
 * @param[in]  softap_ssid
 * @param[in]  softap_password
 * @param[in]  end_with_mac: Whether to add mac information at the end of ssid. eg:SSID_XXYYZZ
 *
 */
esp_err_t esp_mesh_lite_set_softap_info(const char* softap_ssid, const char* softap_password, bool end_with_mac);

/**
 * @brief  Get the mesh_lite_id
 * 
 * @return
 *      - mesh_lite_id
 */
uint8_t esp_mesh_lite_get_mesh_id(void);

/**
 * @brief  Get the node level
 *
 * @return
 *      - level
 */
uint8_t esp_mesh_lite_get_level(void);

/**
 * @brief  Erase rtc_memory store information
 *
 * @attention  Must be called before node factory reset.
 *
 */
void esp_mesh_lite_erase_rtc_store(void);

/**
 * @brief  Get the argot number
 *
 * @return
 *      - argot
 */
uint32_t esp_mesh_lite_get_argot(void);

/**
 * @brief  Get the allowed level
 *
 * @return
 *      - allowed_level
 */
uint8_t esp_mesh_lite_get_allowed_level(void);

/**
 * @brief  Get the disallowed level
 *
 * @return
 *      - disallowed_level
 */
uint8_t esp_mesh_lite_get_disallowed_level(void);


/*****************************************************/
/********* ESP Wi-Fi Mesh Lite Communication *********/
/*****************************************************/

/**
 * @brief  Send broadcast message to child nodes.
 *
 * @param[in]  payload
 * 
 */
esp_err_t esp_mesh_lite_send_broadcast_msg_to_child(const char* payload);

/**
 * @brief  Send broadcast message to parent node.
 * 
 * @attention For non-root nodes, Please choose `esp_mesh_lite_send_msg_to_parent(const char* payload)`
 *
 * @param[in]  payload
 * 
 */
esp_err_t esp_mesh_lite_send_broadcast_msg_to_parent(const char* payload);

/**
 * @brief  Send message to root node.
 *
 * @param[in]  payload
 * 
 */
esp_err_t esp_mesh_lite_send_msg_to_root(const char* payload);

/**
 * @brief  Send message to parent node.
 *
 * @param[in]  payload
 * 
 */
esp_err_t esp_mesh_lite_send_msg_to_parent(const char* payload);

/**
 * @brief Send a specific type of message and set the number of retransmissions.
 *
 * @param[in] send_msg:    Send message type.
 * @param[in] expect_msg:  The type of message expected to be received,
 *                         if the corresponding message type is received, stop retransmission.
 * @param[in] max_retry:   Maximum number of retransmissions.
 * @param[in] req_payload: Message payload, req_payload will not be free in this API.
 * @param[in] resend:      Send msg function pointer.
 *                         - esp_mesh_lite_send_broadcast_msg_to_child()
 *                         - esp_mesh_lite_send_broadcast_msg_to_parent()
 *                         - esp_mesh_lite_send_msg_to_root()
 *                         - esp_mesh_lite_send_msg_to_parent()
 * 
 */
esp_err_t esp_mesh_lite_try_sending_msg(char* send_msg,
                                       char* expect_msg,
                                       uint32_t max_retry,
                                       cJSON* req_payload,
                                       esp_err_t (*resend)(const char* payload));

/**
 * @brief Register custom message reception and recovery logic
 * 
 * @attention  Please refer to examples/rainmaker/common/app_bridge/app_bridge.c
 *
 * @param[in] msg_action
 * 
 */
esp_err_t esp_mesh_lite_msg_action_list_register(const esp_mesh_lite_msg_action_t* msg_action);

/**
 * @brief Register custom message reception and recovery logic
 * 
 * @attention  Please refer to examples/rainmaker/common/app_bridge/app_bridge.c
 *
 * @param[in] msg_action
 * 
 */
esp_err_t esp_mesh_lite_msg_action_list_unregister(const esp_mesh_lite_msg_action_t* msg_action);

/**
  * @brief This function initialize AES context and set key schedule (encryption or decryption).
  * 
  * @param[in]  key      encryption key
  * @param[in]  keybits  currently only supports 128
  * 
  * @attention this function must be called before Mesh-Lite initialization.
  * 
  * @return
  *     - ESP_OK : successful
  *     - Other  : fail
  */
esp_err_t esp_mesh_lite_aes_set_key(const unsigned char* key, unsigned int keybits);
#ifdef __cplusplus
}
#endif

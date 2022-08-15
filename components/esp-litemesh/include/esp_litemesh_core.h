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

extern const char* LITEMESH_EVENT;

#define MAC_MAX_LEN                   (18)

/* Definitions for error constants. */
#define ESP_ERR_DUPLICATE_ADDITION    0x110   /*!< Duplicate addition */

#ifdef CONFIG_LITEMESH_SOFTAP_SSID_END_WITH_THE_MAC 
#define ESP_LITEMESH_SOFTAP_SSID_END_WITH_THE_MAC CONFIG_LITEMESH_SOFTAP_SSID_END_WITH_THE_MAC
#else
#define ESP_LITEMESH_SOFTAP_SSID_END_WITH_THE_MAC 0
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

#if ESP_LITEMESH_SOFTAP_SSID_END_WITH_THE_MAC
#define SSID_MAC_LEN    7  // _XXYYZZ
#else
#define SSID_MAC_LEN    0
#endif

#define STATIC_ASSERT(condition) typedef char p__LINE__[ (condition) ? 1 : -1];
STATIC_ASSERT((sizeof(CONFIG_LITEMESH_SOFTAP_SSID) + SSID_MAC_LEN) < (32 + 2))
STATIC_ASSERT(sizeof(CONFIG_LITEMESH_SOFTAP_PASSWORD) < (63 + 2))

#define ESP_LITEMESH_DEFAULT_INIT()                                                           \
    {                                                                                         \
        .vendor_id = {CONFIG_LITEMESH_VENDOR_ID_0, CONFIG_LITEMESH_VENDOR_ID_1},              \
        .mesh_id = CONFIG_MESH_ID,                                                            \
        .max_connect_number = CONFIG_LITEMESH_MAX_CONNECT_NUMBER,                             \
        .max_router_number = CONFIG_LITEMESH_MAX_ROUTER_NUMBER,                               \
        .max_level = CONFIG_LITEMESH_MAXIMUM_LEVEL_ALLOWED,                                   \
        .end_with_mac = ESP_LITEMESH_SOFTAP_SSID_END_WITH_THE_MAC,                            \
        .join_mesh_ignore_router_status = JOIN_MESH_IGNORE_ROUTER_STATUS,                     \
        .join_mesh_without_configured_wifi = JOIN_MESH_WITHOUT_CONFIGURED_WIFI_INFO,          \
        .softap_ssid = CONFIG_LITEMESH_SOFTAP_SSID,                                           \
        .softap_password = CONFIG_LITEMESH_SOFTAP_PASSWORD,                                   \
    }

typedef enum {
    LITEMESH_EVENT_CORE_STARTED,
    LITEMESH_EVENT_CORE_INHERITED_NET_SEGMENT_CHANGED,
    LITEMESH_EVENT_CORE_ROUTER_INFO_CHANGED,
    LITEMESH_EVENT_CORE_MAX,
} litemesh_event_core_t;

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
} esp_litemesh_config_t;

typedef cJSON* (*msg_process_cb_t)(cJSON *payload, uint32_t seq);

typedef struct esp_litemesh_msg_action {
    const char* type;
    const char* rsp_type;
    msg_process_cb_t process;
} esp_litemesh_msg_action_t;


/**********************************************/
/**************** ESP-LiteMesh ****************/
/**********************************************/

/**
  * @brief Check if the network segment is used to avoid conflicts.
  * 
  * @return
  *     - true :be used
  *     - false:not used
  */
bool esp_litemesh_network_segment_is_used(uint32_t ip);

/**
 * @brief Initialization LiteMesh.
 * 
 * @return
 *     - OK   : successful
 *     - Other: fail
 */
esp_err_t esp_litemesh_core_init(esp_litemesh_config_t* config);

/**
 * @brief Scan to find a matched node and connect.
 * 
 */
void esp_litemesh_connect(void);

/**
 * @brief Set the mesh_id
 *
 * @param[in] mesh_id: Each mesh network should have a different and unique ID.
 * 
 */
void esp_litemesh_set_mesh_id(uint8_t mesh_id);

/**
 * @brief  Set which level this node is only allowed to be
 *
 * @param[in]  level: 1 ~ CONFIG_LITEMESH_MAXIMUM_LEVEL_ALLOWED, 0 is invalid.
 * 
 */
esp_err_t esp_litemesh_set_allowed_level(uint8_t level);

/**
 * @brief  Set which level this node is not allowed to be used as
 *
 * @param[in]  level: 1 ~ CONFIG_LITEMESH_MAXIMUM_LEVEL_ALLOWED, 0 is invalid.
 *
 */
esp_err_t esp_litemesh_set_disallowed_level(uint8_t level);

/**
 * @brief  Set router information
 *
 * @param[in]  conf
 *
 */
esp_err_t esp_litemesh_set_router_config(wifi_sta_config_t *conf);

/**
 * @brief  Whether to allow other nodes to join the mesh network.
 *
 * @param[in]  enable: true -> allow; false -> disallow
 *
 */
esp_err_t esp_litemesh_allow_others_to_join(bool enable);

/**
 * @brief  Set SoftAP information.
 *
 * @param[in]  softap_ssid
 * @param[in]  softap_password
 * @param[in]  end_with_mac: Whether to add mac information at the end of ssid. eg:SSID_XXYYZZ
 *
 */
esp_err_t esp_litemesh_set_softap_info(const char* softap_ssid, const char* softap_password, bool end_with_mac);

/**
 * @brief  Get the mesh_id
 * 
 * @return
 *      - mesh_id
 */
uint8_t esp_litemesh_get_mesh_id(void);

/**
 * @brief  Get the node level
 *
 * @return
 *      - level
 */
uint8_t esp_litemesh_get_level(void);


/**********************************************/
/********* ESP-LiteMesh Communication *********/
/**********************************************/

/**
 * @brief  Send broadcast message to child nodes.
 *
 * @param[in]  payload
 * 
 */
esp_err_t esp_litemesh_send_broadcast_msg_to_child(const char* payload);

/**
 * @brief  Send broadcast message to parent node.
 * 
 * @attention For non-root nodes, Please choose `esp_litemesh_send_msg_to_parent(const char* payload)`
 *
 * @param[in]  payload
 * 
 */
esp_err_t esp_litemesh_send_broadcast_msg_to_parent(const char* payload);

/**
 * @brief  Send message to root node.
 *
 * @param[in]  payload
 * 
 */
esp_err_t esp_litemesh_send_msg_to_root(const char* payload);

/**
 * @brief  Send message to parent node.
 *
 * @param[in]  payload
 * 
 */
esp_err_t esp_litemesh_send_msg_to_parent(const char* payload);

/**
 * @brief Send a specific type of message and set the number of retransmissions.
 *
 * @param[in] send_msg:    Send message type.
 * @param[in] expect_msg:  The type of message expected to be received,
 *                         if the corresponding message type is received, stop retransmission.
 * @param[in] max_retry:   Maximum number of retransmissions.
 * @param[in] req_payload: Message payload, req_payload will not be free in this API.
 * @param[in] resend:      Send msg function pointer.
 *                         - esp_litemesh_send_broadcast_msg_to_child()
 *                         - esp_litemesh_send_broadcast_msg_to_parent()
 *                         - esp_litemesh_send_msg_to_root()
 *                         - esp_litemesh_send_msg_to_parent()
 * 
 */
esp_err_t esp_litemesh_try_sending_msg(char* send_msg,
                                       char* expect_msg,
                                       uint32_t max_retry,
                                       cJSON* req_payload,
                                       esp_err_t (*resend)(const char* payload));

/**
 * @brief Register custom message reception and recovery logic
 * 
 * @attention  Please refer to esp-gateway/examples/rainmaker/common/app_gateway/app_gateway.c
 *
 * @param[in] msg_action
 * 
 */
esp_err_t esp_litemesh_msg_action_list_register(const esp_litemesh_msg_action_t* msg_action);

/**
 * @brief Register custom message reception and recovery logic
 * 
 * @attention  Please refer to esp-gateway/examples/rainmaker/common/app_gateway/app_gateway.c
 *
 * @param[in] msg_action
 * 
 */
esp_err_t esp_litemesh_msg_action_list_unregister(const esp_litemesh_msg_action_t* msg_action);

/**
  * @brief This function initialize AES context and set key schedule (encryption or decryption).
  * 
  * @param[in]  key      encryption key
  * @param[in]  keybits  currently only supports 128
  * 
  * @attention this function must be called before LiteMesh initialization.
  * 
  * @return
  *     - ESP_OK : successful
  *     - Other  : fail
  */
esp_err_t esp_litemesh_aes_set_key(const unsigned char* key, unsigned int keybits);
#ifdef __cplusplus
}
#endif



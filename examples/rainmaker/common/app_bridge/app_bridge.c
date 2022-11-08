#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_netif.h"

#include "esp_bridge.h"
#include "esp_mesh_lite.h"

#include <app_light.h>
#include <app_bridge.h>
#include <app_rainmaker.h>

#include "esp_rmaker_core.h"
#include "esp_rmaker_common_events.h"

static const char *TAG = "app_bridge";

#define ESP_RMAKER_MESH_LITE_SERVICE            "mesh"
#define ESP_RMAKER_MESH_LITE_SERVICE_LEVEL      "level"
#define ESP_RMAKER_MESH_LITE_SERVICE_MESH_ID    "mesh_id"
#define ESP_RMAKER_MESH_LITE_SERVICE_RANDOM     "random"
#define ESP_RMAKER_MESH_LITE_SERVICE_SSID       "ssid"
#define ESP_RMAKER_MESH_LITE_SERVICE_PASSWORD   "password"
#define ESP_RMAKER_MESH_LITE_SERVICE_SELF_IP    "self_ip"
#define ESP_RMAKER_MESH_LITE_SERVICE_SELF_MAC   "self_mac"
#define ESP_RMAKER_MESH_LITE_SERVICE_CHILD_IP   "child_ip"
#define ESP_RMAKER_MESH_LITE_SERVICE_CHILD_MAC  "child_mac"
#define ESP_RMAKER_MESH_LITE_SERVICE_MESH_GROUP "mesh_group"

#define MAX_STATION CONFIG_MESH_LITE_MAX_CONNECT_NUMBER

static esp_rmaker_param_t *level_param;
static esp_rmaker_param_t *mesh_id_param;
static esp_rmaker_param_t *random_param;
static esp_rmaker_param_t *ssid_param;
static esp_rmaker_param_t *password_param;
static esp_rmaker_param_t *self_ip_param;
static esp_rmaker_param_t *self_mac_param;
static esp_rmaker_param_t *child_ip_param;
static esp_rmaker_param_t *child_mac_param;
static esp_rmaker_param_t *mesh_group_param;

typedef struct esp_mesh_lite_child_info {
    char mac[MAC_MAX_LEN];
    char ip[IP_MAX_LEN];
    struct esp_mesh_lite_child_info* next;
} esp_mesh_lite_child_info_t;

static SemaphoreHandle_t child_info_mutex;
static esp_mesh_lite_child_info_t* child_info = NULL;
static char* mac_strings[MAX_STATION];
static char* ip_strings[MAX_STATION];

esp_err_t esp_rmaker_mesh_lite_add_child_info(char* mac, char* ip)
{
    xSemaphoreTake(child_info_mutex, portMAX_DELAY);
    esp_mesh_lite_child_info_t* new = child_info;

    while (new) {
        if (!strncmp(new->mac, mac, (MAC_MAX_LEN - 1))) {
            xSemaphoreGive(child_info_mutex);
            return ESP_ERR_DUPLICATE_ADDITION;
        }
        new = new->next;
    }

    /* not found, create a new */
    new = (esp_mesh_lite_child_info_t*)malloc(sizeof(esp_mesh_lite_child_info_t));
    if (new == NULL) {
        ESP_LOGE(TAG, "child info add fail");
        xSemaphoreGive(child_info_mutex);
        return ESP_ERR_NO_MEM;
    }

    memcpy(new->mac, mac, MAC_MAX_LEN);
    memcpy(new->ip, ip, IP_MAX_LEN);

    new->next = child_info;
    child_info = new;

    xSemaphoreGive(child_info_mutex);  
    return ESP_OK;
}

void esp_rmaker_mesh_lite_remove_child_info(char* mac)
{
    xSemaphoreTake(child_info_mutex, portMAX_DELAY);

    esp_mesh_lite_child_info_t* current = child_info;
    esp_mesh_lite_child_info_t* prev = NULL;

    while (current) {
        if (!strncmp(current->mac, mac, (MAC_MAX_LEN - 1))) {
            if (prev == NULL) {
                child_info = child_info->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            break;
        }

        prev = current;
        current = current->next;
    }

    xSemaphoreGive(child_info_mutex);
}

void esp_rmaker_mesh_lite_child_info_update_and_report(void)
{
    xSemaphoreTake(child_info_mutex, portMAX_DELAY);
    uint8_t loop = 0;
    esp_mesh_lite_child_info_t* p = child_info;

    while (p) {
        if (loop < MAX_STATION) {
            memcpy(mac_strings[loop], p->mac, MAC_MAX_LEN);
            memcpy(ip_strings[loop], p->ip, IP_MAX_LEN);
        } else {
            break;
        }
        p = p->next;
        loop++;
    }

    cJSON *child_ip_item = cJSON_CreateStringArray((const char* const*)ip_strings, loop);
    cJSON *child_mac_item = cJSON_CreateStringArray((const char* const*)mac_strings, loop);
    char* child_ip_string = cJSON_PrintUnformatted(child_ip_item);
    char* child_mac_string = cJSON_PrintUnformatted(child_mac_item);
    cJSON_Delete(child_ip_item);
    cJSON_Delete(child_mac_item);

    esp_rmaker_param_update_and_report(child_ip_param, esp_rmaker_array(child_ip_string));
    esp_rmaker_param_update_and_report(child_mac_param, esp_rmaker_array(child_mac_string));

    xSemaphoreGive(child_info_mutex);
}

static esp_err_t esp_rmaker_mesh_service_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
        const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx)
{
    esp_err_t err = ESP_FAIL;
    if (strcmp(esp_rmaker_param_get_type(param), ESP_RMAKER_MESH_LITE_SERVICE_MESH_GROUP) == 0) {
        ESP_LOGI(TAG, "The room is %s\r\n", val.val.s);
    }
    esp_rmaker_param_update_and_report(param, val);
    return err;
}

static void ip_event_handler(void *arg, esp_event_base_t event_base,
                             int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;

    if (esp_mesh_lite_get_level() > 1) {
        esp_mesh_lite_report_child_info();
#if CONFIG_MESH_LITE_NODE_INFO_REPORT
        esp_mesh_lite_report_info();
#endif
    }
    esp_rmaker_mesh_lite_level_update_and_report(esp_mesh_lite_get_level());
    esp_rmaker_mesh_lite_self_ip_update_and_report(NULL, &event->ip_info);
}

static void esp_mesh_lite_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
    esp_mesh_lite_node_info_t *event = (esp_mesh_lite_node_info_t *) event_data;
    switch (event_id) {
        case ESP_MESH_LITE_EVENT_CORE_INHERITED_NET_SEGMENT_CHANGED:
            esp_rmaker_mesh_lite_self_ip_update_and_report(NULL, NULL);
            break;
        case ESP_MESH_LITE_EVENT_CHILD_NODE_JOIN:
            ESP_LOGI(TAG, "[Child Info Join]  Level:%d  Mac:%s IP %s\r\n", event->level, event->mac, event->ip);
            esp_rmaker_mesh_lite_add_child_info(event->mac, event->ip);
            esp_rmaker_mesh_lite_child_info_update_and_report();
            break;
        case ESP_MESH_LITE_EVENT_CHILD_NODE_LEAVE:
            ESP_LOGI(TAG, "[Child Info Leave]  Level:%d  Mac:%s IP %s\r\n", event->level, event->mac, event->ip);
            esp_rmaker_mesh_lite_remove_child_info(event->mac);
            esp_rmaker_mesh_lite_child_info_update_and_report();
            break;
#if CONFIG_MESH_LITE_NODE_INFO_REPORT
        case ESP_MESH_LITE_EVENT_NODE_JOIN:
            ESP_LOGI(TAG, "[Node Info Join]  Level:%d  Mac:%s\r\n", event->level, event->mac);
            break;
        case ESP_MESH_LITE_EVENT_NODE_LEAVE:
            ESP_LOGI(TAG, "[Node Info Leave]  Level:%d  Mac:%s\r\n", event->level, event->mac);
            break;
        case ESP_MESH_LITE_EVENT_NODE_CHANGE:
            ESP_LOGI(TAG, "[Node Info change]  Level:%d  Mac:%s\r\n", event->level, event->mac);
            break;
#endif
    }
}

static char* esp_rmaker_mesh_lite_self_ip_format(esp_netif_ip_info_t* ap_ip_info, esp_netif_ip_info_t* sta_ip_info)
{
    char* self_ip_obj[2];
    self_ip_obj[0] = (char*)calloc(IP_MAX_LEN, 1);
    self_ip_obj[1] = (char*)calloc(IP_MAX_LEN, 1);

    if (!ap_ip_info) {
        esp_netif_ip_info_t ap_ip_info_old;
        memset(&ap_ip_info_old, 0x0, sizeof(esp_netif_ip_info_t));
        esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ap_ip_info_old);
        sprintf(self_ip_obj[0], IPSTR, IP2STR(&ap_ip_info_old.ip));
    } else {
        sprintf(self_ip_obj[0], IPSTR, IP2STR(&ap_ip_info->ip));
    }

    if (!sta_ip_info) {
        esp_netif_ip_info_t sta_ip_info_old;
        memset(&sta_ip_info_old, 0x0, sizeof(esp_netif_ip_info_t));
        esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &sta_ip_info_old);
        sprintf(self_ip_obj[1], IPSTR, IP2STR(&sta_ip_info_old.ip));
    } else {
        sprintf(self_ip_obj[1], IPSTR, IP2STR(&sta_ip_info->ip));
    }

    cJSON *slef_ip_item = cJSON_CreateStringArray((const char* const*)self_ip_obj, 2);
    char* self_ip_string = cJSON_PrintUnformatted(slef_ip_item);
    cJSON_Delete(slef_ip_item);
    free(self_ip_obj[0]);
    free(self_ip_obj[1]);

    return self_ip_string;
}

static char* esp_rmaker_mesh_lite_self_mac_format(void)
{
    char* self_mac_obj[2];
    self_mac_obj[0] = (char*)calloc(MAC_MAX_LEN, 1);
    self_mac_obj[1] = (char*)calloc(MAC_MAX_LEN, 1);
    uint8_t mac_temp[6];
    esp_wifi_get_mac(WIFI_IF_AP, mac_temp);
    snprintf(self_mac_obj[0], MAC_MAX_LEN, MACSTR, MAC2STR(mac_temp));
    esp_wifi_get_mac(WIFI_IF_STA, mac_temp);
    snprintf(self_mac_obj[1], MAC_MAX_LEN, MACSTR, MAC2STR(mac_temp));

    cJSON *self_mac_item = cJSON_CreateStringArray((const char* const*)self_mac_obj, 2);
    char* self_mac_string = cJSON_PrintUnformatted(self_mac_item);
    cJSON_Delete(self_mac_item);
    free(self_mac_obj[0]);
    free(self_mac_obj[1]);

    return self_mac_string;
}

void esp_rmaker_mesh_lite_level_update_and_report(uint8_t level)
{
    esp_rmaker_param_update_and_report(level_param, esp_rmaker_int(level));
}

void esp_rmaker_mesh_lite_self_ip_update_and_report(esp_netif_ip_info_t* ap_ip_info, esp_netif_ip_info_t* sta_ip_info)
{
    char* self_ip_string = esp_rmaker_mesh_lite_self_ip_format(ap_ip_info, sta_ip_info);
    esp_rmaker_param_update_and_report(self_ip_param, esp_rmaker_array(self_ip_string));
}

void app_rmaker_mesh_lite_service_creat(void)
{
    esp_rmaker_device_t *service = esp_rmaker_service_create(ESP_RMAKER_MESH_LITE_SERVICE, "esp.service.mesh", NULL);
    if (service) {
        level_param = esp_rmaker_param_create(ESP_RMAKER_MESH_LITE_SERVICE_LEVEL, "esp.param.level", esp_rmaker_int(esp_mesh_lite_get_level()), PROP_FLAG_READ);
        esp_rmaker_device_add_param(service, level_param);

        mesh_id_param = esp_rmaker_param_create(ESP_RMAKER_MESH_LITE_SERVICE_MESH_ID, "esp.param.mesh_id", esp_rmaker_int(esp_mesh_lite_get_mesh_id()), PROP_FLAG_READ);
        esp_rmaker_device_add_param(service, mesh_id_param);

        random_param = esp_rmaker_param_create(ESP_RMAKER_MESH_LITE_SERVICE_RANDOM, "esp.param.random", esp_rmaker_int(esp_mesh_lite_get_argot()), PROP_FLAG_READ);
        esp_rmaker_device_add_param(service, random_param);

        /* Get Wi-Fi Station configuration */
        char ssid_temp[32];
        char psw_temp[64];
        memset(ssid_temp, 0x0, 32);
        memset(psw_temp, 0x0, 64);

        esp_mesh_lite_nvs_get_str("softap_ssid", ssid_temp);
        ssid_param = esp_rmaker_param_create(ESP_RMAKER_MESH_LITE_SERVICE_SSID, "esp.param.ssid", esp_rmaker_str((const char*)ssid_temp), PROP_FLAG_READ);
        esp_rmaker_device_add_param(service, ssid_param);

        esp_mesh_lite_nvs_get_str("softap_psw", psw_temp);
        password_param = esp_rmaker_param_create(ESP_RMAKER_MESH_LITE_SERVICE_PASSWORD, "esp.param.password", esp_rmaker_str((const char*)psw_temp), PROP_FLAG_READ);
        esp_rmaker_device_add_param(service, password_param);

        char* self_ip_string = esp_rmaker_mesh_lite_self_ip_format(NULL, NULL);
        self_ip_param = esp_rmaker_param_create(ESP_RMAKER_MESH_LITE_SERVICE_SELF_IP, "esp.param.self_ip", esp_rmaker_array(self_ip_string), PROP_FLAG_READ);
        esp_rmaker_device_add_param(service, self_ip_param);

        char* self_mac_string = esp_rmaker_mesh_lite_self_mac_format();
        self_mac_param = esp_rmaker_param_create(ESP_RMAKER_MESH_LITE_SERVICE_SELF_MAC, "esp.param.self_mac", esp_rmaker_array(self_mac_string), PROP_FLAG_READ);
        esp_rmaker_device_add_param(service, self_mac_param);

        child_ip_param = esp_rmaker_param_create(ESP_RMAKER_MESH_LITE_SERVICE_CHILD_IP, "esp.param.child_ip", esp_rmaker_array("[]"), PROP_FLAG_READ);
        esp_rmaker_device_add_param(service, child_ip_param);

        child_mac_param = esp_rmaker_param_create(ESP_RMAKER_MESH_LITE_SERVICE_CHILD_MAC, "esp.param.child_mac", esp_rmaker_array("[]"), PROP_FLAG_READ);
        esp_rmaker_device_add_param(service, child_mac_param);

        mesh_group_param = esp_rmaker_param_create(ESP_RMAKER_MESH_LITE_SERVICE_MESH_GROUP, "esp.param.mesh_group", esp_rmaker_str(""), PROP_FLAG_READ | PROP_FLAG_WRITE);
        esp_rmaker_device_add_param(service, mesh_group_param);
    }
    esp_rmaker_device_add_cb(service, esp_rmaker_mesh_service_cb, NULL);
    esp_err_t err = esp_rmaker_node_add_device(esp_rmaker_get_node(), service);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "mesh service enabled");
    } else {
        esp_rmaker_device_delete(service);
    }
}

esp_err_t app_bridge_enable(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_bridge_create_all_netif();
    // esp_mesh_lite_set_allowed_level(1);
    // esp_mesh_lite_set_disallowed_level(1);

    child_info_mutex = xSemaphoreCreateMutex();

    for (uint8_t i = 0; i < MAX_STATION; i++) {
        mac_strings[i] = (char*)malloc(MAC_MAX_LEN);
        memset(mac_strings[i], 0x0, MAC_MAX_LEN);
    }

    for (uint8_t i = 0; i < MAX_STATION; i++) {
        ip_strings[i] = (char*)malloc(IP_MAX_LEN);
        memset(ip_strings[i], 0x0, IP_MAX_LEN);
    }

    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(ESP_MESH_LITE_EVENT, ESP_EVENT_ANY_ID, &esp_mesh_lite_handler, NULL, NULL));

    app_mesh_lite_comm_report_info_to_parent();
    return ESP_OK;
}
/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_idf_version.h>
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 1, 0)
// Features supported in 4.1+
#define ESP_NETIF_SUPPORTED
#endif

#ifdef ESP_NETIF_SUPPORTED
#include <esp_netif.h>
#else
#include <tcpip_adapter.h>
#endif

#include <wifi_provisioning/manager.h>
#ifdef CONFIG_APP_WIFI_PROV_TRANSPORT_BLE
#include <wifi_provisioning/scheme_ble.h>
#else /* CONFIG_APP_WIFI_PROV_TRANSPORT_SOFTAP */
#include <wifi_provisioning/scheme_softap.h>
#endif /* CONFIG_APP_WIFI_PROV_TRANSPORT_BLE */

#include <qrcode.h>
#include <nvs.h>
#include <nvs_flash.h>
#include "app_wifi.h"

#include "esp_bridge.h"

static const char *TAG = "app_wifi";
static const int WIFI_CONNECTED_EVENT = BIT0;
static EventGroupHandle_t wifi_event_group;

#define PROV_QR_VERSION "v1"

#define PROV_TRANSPORT_SOFTAP   "softap"
#define PROV_TRANSPORT_BLE      "ble"
#define QRCODE_BASE_URL     "https://rainmaker.espressif.com/qrcode.html"

#define CREDENTIALS_NAMESPACE   "rmaker_creds"
#define RANDOM_NVS_KEY          "random"

#ifdef CONFIG_APP_WIFI_SHOW_DEMO_INTRO_TEXT

#define ESP_RAINMAKER_GITHUB_EXAMPLES_PATH  "https://github.com/espressif/esp-rainmaker/blob/master/examples"
#define ESP_RAINMAKER_INTRO_LINK    "https://rainmaker.espressif.com"
#define ESP_RMAKER_PHONE_APP_LINK   "http://bit.ly/esp-rmaker"
char esp_rainmaker_ascii_art[] = \
"  ______  _____ _____    _____            _____ _   _ __  __          _  ________ _____\n"\
" |  ____|/ ____|  __ \\  |  __ \\     /\\   |_   _| \\ | |  \\/  |   /\\   | |/ /  ____|  __ \\\n"\
" | |__  | (___ | |__) | | |__) |   /  \\    | | |  \\| | \\  / |  /  \\  | ' /| |__  | |__) |\n"\
" |  __|  \\___ \\|  ___/  |  _  /   / /\\ \\   | | | . ` | |\\/| | / /\\ \\ |  < |  __| |  _  /\n"\
" | |____ ____) | |      | | \\ \\  / ____ \\ _| |_| |\\  | |  | |/ ____ \\| . \\| |____| | \\ \\\n"\
" |______|_____/|_|      |_|  \\_\\/_/    \\_\\_____|_| \\_|_|  |_/_/    \\_\\_|\\_\\______|_|  \\_\\\n";

static void intro_print(bool provisioned)
{
    printf("####################################################################################################\n");
    printf("%s\n", esp_rainmaker_ascii_art);
    printf("Welcome to ESP RainMaker %s demo application!\n", RMAKER_DEMO_PROJECT_NAME);
    if (!provisioned) {
        printf("Follow these steps to get started:\n");
        printf("1. Download the ESP RainMaker phone app by visiting this link from your phone's browser:\n\n");
        printf("   %s\n\n", ESP_RMAKER_PHONE_APP_LINK);
        printf("2. Sign up and follow the steps on screen to add the device to your Wi-Fi network.\n");
        printf("3. You are now ready to use the device and control it locally as well as remotely.\n");
        printf("   You can also use the Boot button on the board to control your device.\n");
    }
    printf("\nIf you want to reset Wi-Fi credentials, or reset to factory, press and hold the Boot button.\n");
    printf("\nThis application uses ESP RainMaker, which is based on ESP IDF.\n");
    printf("Check out the source code for this application here:\n   %s/%s\n",
            ESP_RAINMAKER_GITHUB_EXAMPLES_PATH, RMAKER_DEMO_PROJECT_NAME);
    printf("\nPlease visit %s for additional information.\n\n", ESP_RAINMAKER_INTRO_LINK);
    printf("####################################################################################################\n");
}

#else

static void intro_print(bool provisioned)
{
    /* Do nothing */
}

#endif /* !APP_WIFI_SHOW_DEMO_INTRO_TEXT */

static void app_wifi_print_qr(const char *name, const char *pop, const char *transport)
{
    if (!name || !pop || !transport) {
        ESP_LOGW(TAG, "Cannot generate QR code payload. Data missing.");
        return;
    }
    char payload[150];
    snprintf(payload, sizeof(payload), "{\"ver\":\"%s\",\"name\":\"%s\"" \
                    ",\"pop\":\"%s\",\"transport\":\"%s\"}",
                    PROV_QR_VERSION, name, pop, transport);
#ifdef CONFIG_APP_WIFI_PROV_SHOW_QR
    ESP_LOGI(TAG, "Scan this QR code from the ESP RainMaker phone app for Provisioning.");
    qrcode_display(payload);
#endif /* CONFIG_APP_WIFI_PROV_SHOW_QR */
    ESP_LOGI(TAG, "If QR code is not visible, copy paste the below URL in a browser.\n%s?data=%s", QRCODE_BASE_URL, payload);
}

#define KEYSTORE_NAMESPACE    "app_wifi_nw"
#define KEY_STA_SSID          "ssid"
#define KEY_STA_PASSWORD      "pswd"

static esp_err_t app_wifi_keystore_get(const char *name_space, const char *key, uint8_t *val, size_t *val_size)
{
    nvs_handle handle;
    esp_err_t err = nvs_open(name_space, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        return -1;
    } else {
        err = nvs_get_blob(handle, key, val, val_size);
        nvs_close(handle);
    }

    return err;
}

static esp_err_t app_wifi_keystore_set(const char *name_space, const char *key, const uint8_t *val, const size_t val_len)
{
    nvs_handle handle;
    esp_err_t err = nvs_open(name_space, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%d) opening NVS handle!", err);
    } else {
        err = nvs_set_blob(handle, key, val, val_len);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to write %s", key);
        } else {
            nvs_commit(handle);
        }
        nvs_close(handle);
    }
    
    return err;
}

static esp_err_t app_wifi_network_connect(void)
{
    uint8_t ssid[64] = {0};
    uint8_t password[64] = {0};
    unsigned int ssid_len = sizeof(ssid);
    unsigned int password_len = sizeof(password);
    int ret = app_wifi_keystore_get(KEYSTORE_NAMESPACE, KEY_STA_SSID, ssid, &ssid_len);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get stored SSID");
        return ESP_FAIL;
    }

    /* Even if a password is not found, it is not an error, as it could be an open network */
    ret = app_wifi_keystore_get(KEYSTORE_NAMESPACE, KEY_STA_PASSWORD, password, &password_len);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get stored Password");
        password_len = 0;
    }

    wifi_config_t wifi_cfg_old;
    memset(&wifi_cfg_old, 0, sizeof(wifi_config_t));
    memcpy(wifi_cfg_old.sta.ssid, ssid, ssid_len);
    if (password_len)
        memcpy(wifi_cfg_old.sta.password, password, password_len);

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_cfg_old));
    ESP_ERROR_CHECK(esp_wifi_start());

    return ESP_OK;
}

/* Event handler for catching system events */
static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == WIFI_PROV_EVENT) {
        switch (event_id) {
            case WIFI_PROV_START:
                ESP_LOGI(TAG, "Provisioning started");
                session_cost_information(TAG, __func__, __LINE__, "WIFI_PROV_START");
                break;
            case WIFI_PROV_CRED_RECV: {
                wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
                ESP_LOGI(TAG, "Received Wi-Fi credentials"
                         "\n\tSSID     : %s\n\tPassword : %s",
                         (const char *) wifi_sta_cfg->ssid,
                         (const char *) wifi_sta_cfg->password);
                session_cost_information(TAG, __func__, __LINE__, "WIFI_PROV_CRED_RECV");
                app_wifi_keystore_set(KEYSTORE_NAMESPACE, KEY_STA_SSID, wifi_sta_cfg->ssid, strlen((const char *) wifi_sta_cfg->ssid));
                if (strlen((const char *) wifi_sta_cfg->password)) {
                    app_wifi_keystore_set(KEYSTORE_NAMESPACE, KEY_STA_PASSWORD, wifi_sta_cfg->password, strlen((const char *) wifi_sta_cfg->password));
                }
#if CONFIG_MESH_LITE_ENABLE
                esp_mesh_lite_set_router_config(wifi_sta_cfg);
                esp_mesh_lite_connect();
#else
                esp_wifi_set_storage(WIFI_STORAGE_FLASH);
                esp_wifi_set_config(ESP_IF_WIFI_STA, (wifi_config_t*)wifi_sta_cfg);
                esp_wifi_set_storage(WIFI_STORAGE_RAM);

                esp_wifi_disconnect();
                esp_wifi_connect();
#endif /* CONFIG_MESH_LITE_ENABLE */
                xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_EVENT);
                break;
            }
            case WIFI_PROV_CRED_FAIL: {
                wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
                ESP_LOGE(TAG, "Provisioning failed!\n\tReason : %s"
                         "\n\tPlease reset to factory and retry provisioning",
                         (*reason == WIFI_PROV_STA_AUTH_ERROR) ?
                         "Wi-Fi station authentication failed" : "Wi-Fi access-point not found");
                xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_EVENT);
                break;
            }
            case WIFI_PROV_CRED_SUCCESS:
                ESP_LOGI(TAG, "Provisioning successful");
                session_cost_information(TAG, __func__, __LINE__, "WIFI_PROV_CRED_SUCCESS");
                break;
            case WIFI_PROV_END:
                /* De-initialize manager once provisioning is finished */
                wifi_prov_mgr_deinit();
                break;
            default:
                break;
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        session_cost_information(TAG, __func__, __LINE__, "WIFI_EVENT_STA_START");
        // esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
        session_cost_information(TAG, __func__, __LINE__, "IP_EVENT_STA_GOT_IP");
        /* Signal main application to continue execution */
        // xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_EVENT);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Disconnected. Connecting to the AP again...");
        session_cost_information(TAG, __func__, __LINE__, "WIFI_EVENT_STA_DISCONNECTED");
        // esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        session_cost_information(TAG, __func__, __LINE__, "WIFI_EVENT_STA_CONNECTED");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        session_cost_information(TAG, __func__, __LINE__, "WIFI_EVENT_AP_STACONNECTED");
    }
}

static esp_err_t rainmaker_mesh_lite_handler(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen,
                                             uint8_t **outbuf, ssize_t *outlen, void *priv_data)
{
    if (inbuf == NULL) {
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Received data from APP: %.*s", inlen, (char *)inbuf);
    cJSON *root = cJSON_Parse((const char*)inbuf);
    cJSON *item = NULL;
    uint8_t mesh_id = 0;
    uint32_t argot = 0;
    wifi_ap_config_t config;
    memset(&config, 0x0, sizeof(config));

    item = cJSON_GetObjectItem(root, "meshId");
    if (item) {
        mesh_id = item->valueint;
        esp_mesh_lite_set_mesh_id(mesh_id, true);
        ESP_LOGI(TAG, "[MeshID]: %d", mesh_id);
    }

    item = cJSON_GetObjectItem(root, "random");
    if (item) {
        argot = item->valuedouble;
        esp_mesh_lite_set_argot(argot);
        ESP_LOGI(TAG, "[random]: %u", argot);
    }

    item = cJSON_GetObjectItem(root, "password");
    if (item) {
        strlcpy((char *)config.password, item->valuestring, sizeof(config.password));
        esp_mesh_lite_nvs_set_str("softap_psw", (char *)config.password);
        ESP_LOGI(TAG, "[SoftAP psw]: %s", config.password);
    }

    item = cJSON_GetObjectItem(root, "ssid");
    if (item) {
        esp_mesh_lite_nvs_set_str("softap_ssid", item->valuestring);
        esp_mesh_lite_set_softap_info(item->valuestring, (char*)config.password, true);

        char softap_ssid[33];
        uint8_t mac[6];
        esp_wifi_get_mac(WIFI_IF_AP, mac);
        snprintf(softap_ssid, sizeof(softap_ssid), "%.25s_%02x%02x%02x", item->valuestring, mac[3], mac[4], mac[5]);
        memcpy((char *)config.ssid, softap_ssid, sizeof(config.ssid));
        ESP_LOGI(TAG, "[SoftAP ssid]: %s", (char *)config.ssid);
    }

    config.max_connection = CONFIG_MESH_LITE_MAX_CONNECT_NUMBER;
    config.authmode = strlen((char*)config.password) < 8 ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, (wifi_config_t*)&config));

    char response[] = "SUCCESS";
    *outbuf = (uint8_t *)strdup(response);
    if (*outbuf == NULL) {
        ESP_LOGE(TAG, "System out of memory");
        return ESP_ERR_NO_MEM;
    }
    *outlen = strlen(response) + 1; /* +1 for NULL terminating byte */

    ESP_LOGW("heap", "free heap %d, minimum %d", esp_get_free_heap_size(), esp_get_minimum_free_heap_size());

    return ESP_OK;
}

/* Free random_bytes after use only if function returns ESP_OK */
static esp_err_t read_random_bytes_from_nvs(uint8_t **random_bytes, size_t *len)
{
    nvs_handle handle;
    esp_err_t err;
    *len = 0;

    if ((err = nvs_open_from_partition(CONFIG_ESP_RMAKER_FACTORY_PARTITION_NAME, CREDENTIALS_NAMESPACE,
                                NVS_READONLY, &handle)) != ESP_OK) {
        ESP_LOGD(TAG, "NVS open for %s %s %s failed with error %d", CONFIG_ESP_RMAKER_FACTORY_PARTITION_NAME, CREDENTIALS_NAMESPACE, RANDOM_NVS_KEY, err);
        return ESP_FAIL;
    }

    if ((err = nvs_get_blob(handle, RANDOM_NVS_KEY, NULL, len)) != ESP_OK) {
        ESP_LOGD(TAG, "Error %d. Failed to read key %s.", err, RANDOM_NVS_KEY);
        nvs_close(handle);
        return ESP_ERR_NOT_FOUND;
    }

    *random_bytes = calloc(*len, 1);
    if (*random_bytes) {
        nvs_get_blob(handle, RANDOM_NVS_KEY, *random_bytes, len);
        nvs_close(handle);
        return ESP_OK;
    }
    nvs_close(handle);
    return ESP_ERR_NO_MEM;
}

static esp_err_t get_device_service_name(char *service_name, size_t max)
{
    uint8_t *nvs_random = NULL;
    const char *ssid_prefix = "BOX_";
    size_t nvs_random_size = 0;
    if ((read_random_bytes_from_nvs(&nvs_random, &nvs_random_size) != ESP_OK) || nvs_random_size < 3) {
        uint8_t eth_mac[6];
        esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
        snprintf(service_name, max, "%s%02x%02x%02x", ssid_prefix, eth_mac[3], eth_mac[4], eth_mac[5]);
    } else {
        snprintf(service_name, max, "%s%02x%02x%02x", ssid_prefix, nvs_random[nvs_random_size - 3],
                nvs_random[nvs_random_size - 2], nvs_random[nvs_random_size - 1]);
    }
    if (nvs_random) {
        free(nvs_random);
    }

    // Nova Home
    uint8_t mfg[13] = { 0xe5, 0x02, 'N', 'o', 'v', 'c', 0x00, 0x01, 0x00, 0x05, 0x01, 0x00 };
#if CONFIG_MESH_LITE_ENABLE
    mfg[12] |= 0x08;
    uint8_t allowed_level = esp_mesh_lite_get_allowed_level();
    uint8_t disallowed_level = esp_mesh_lite_get_disallowed_level();

    if (allowed_level == 1) {
        mfg[12] |= 0x04;
    } else if (disallowed_level == 1) {
        mfg[12] |= 0x02;
    } else {
        mfg[12] |= 0x06;
    }
#else
    mfg[12] &= 0xF7;
#endif
    ESP_LOGI(TAG, "reserve0 %d [Mesh-Lite Enable]:%d [root]:%d [child]:%d", mfg[12], ((mfg[12] >> 3) & 0x01), ((mfg[12] >> 2) & 0x01), ((mfg[12] >> 1) & 0x01));
    esp_err_t err = wifi_prov_scheme_ble_set_mfg_data(mfg, sizeof(mfg));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "wifi_prov_scheme_ble_set_mfg_data failed %d", err);
        return err;
    }
    return ESP_OK;
}

static esp_err_t get_device_pop(char *pop, size_t max, app_wifi_pop_type_t pop_type)
{
    if (!pop || !max) {
        return ESP_ERR_INVALID_ARG;
    }

    if (pop_type == POP_TYPE_MAC) {
        uint8_t eth_mac[6];
        esp_err_t err = esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
        if (err == ESP_OK) {
            snprintf(pop, max, "%02x%02x%02x%02x", eth_mac[2], eth_mac[3], eth_mac[4], eth_mac[5]);
            return ESP_OK;
        } else {
            return err;
        }
    } else if (pop_type == POP_TYPE_RANDOM) {
        uint8_t *nvs_random;
        size_t nvs_random_size = 0;
        if ((read_random_bytes_from_nvs(&nvs_random, &nvs_random_size) != ESP_OK) || nvs_random_size < 4) {
            return ESP_ERR_NOT_FOUND;
        } else {
            snprintf(pop, max, "%02x%02x%02x%02x", nvs_random[0], nvs_random[1], nvs_random[2], nvs_random[3]);
            free(nvs_random);
            return ESP_OK;
        }
    } else {
        return ESP_ERR_INVALID_ARG;
    }
}

esp_err_t app_wifi_start(app_wifi_pop_type_t pop_type)
{
    wifi_event_group = xEventGroupCreate();

    /* Register our event handler for Wi-Fi, IP and Provisioning related events */
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    /* Configuration for the provisioning manager */
    wifi_prov_mgr_config_t config = {
        /* What is the Provisioning Scheme that we want ?
         * wifi_prov_scheme_softap or wifi_prov_scheme_ble */
#ifdef CONFIG_APP_WIFI_PROV_TRANSPORT_BLE
        .scheme = wifi_prov_scheme_ble,
#else /* CONFIG_APP_WIFI_PROV_TRANSPORT_SOFTAP */
        .scheme = wifi_prov_scheme_softap,
#endif /* CONFIG_APP_WIFI_PROV_TRANSPORT_BLE */

        /* Any default scheme specific event handler that you would
         * like to choose. Since our example application requires
         * neither BT nor BLE, we can choose to release the associated
         * memory once provisioning is complete, or not needed
         * (in case when device is already provisioned). Choosing
         * appropriate scheme specific event handler allows the manager
         * to take care of this automatically. This can be set to
         * WIFI_PROV_EVENT_HANDLER_NONE when using wifi_prov_scheme_softap*/
#ifdef CONFIG_APP_WIFI_PROV_TRANSPORT_BLE
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM
#else /* CONFIG_APP_WIFI_PROV_TRANSPORT_SOFTAP */
        .scheme_event_handler = WIFI_PROV_EVENT_HANDLER_NONE,
#endif /* CONFIG_APP_WIFI_PROV_TRANSPORT_BLE */
    };

    /* Initialize provisioning manager with the
     * configuration parameters set above */
    ESP_ERROR_CHECK(wifi_prov_mgr_init(config));

    bool provisioned = false;
    /* Let's find out if the device is provisioned */
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));
    /* If device is not yet provisioned start provisioning service */
    if (!provisioned) {
        ESP_LOGI(TAG, "Starting provisioning");
#ifdef ESP_NETIF_SUPPORTED
        // esp_netif_create_default_wifi_ap();
#endif

        /* What is the Device Service Name that we want
         * This translates to :
         *     - Wi-Fi SSID when scheme is wifi_prov_scheme_softap
         *     - device name when scheme is wifi_prov_scheme_ble
         */
        char service_name[12];
        get_device_service_name(service_name, sizeof(service_name));

        /* What is the security level that we want (0 or 1):
         *      - WIFI_PROV_SECURITY_0 is simply plain text communication.
         *      - WIFI_PROV_SECURITY_1 is secure communication which consists of secure handshake
         *          using X25519 key exchange and proof of possession (pop) and AES-CTR
         *          for encryption/decryption of messages.
         */
        wifi_prov_security_t security = WIFI_PROV_SECURITY_1;

        /* Do we want a proof-of-possession (ignored if Security 0 is selected):
         *      - this should be a string with length > 0
         *      - NULL if not used
         */
        char pop[9];
        esp_err_t err = get_device_pop(pop, sizeof(pop), pop_type);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error: %d. Failed to get PoP from NVS, Please perform Claiming.", err);
            return err;
        }

        /* What is the service key (Wi-Fi password)
         * NULL = Open network
         * This is ignored when scheme is wifi_prov_scheme_ble
         */
        const char *service_key = NULL;

#ifdef CONFIG_APP_WIFI_PROV_TRANSPORT_BLE
        /* This step is only useful when scheme is wifi_prov_scheme_ble. This will
         * set a custom 128 bit UUID which will be included in the BLE advertisement
         * and will correspond to the primary GATT service that provides provisioning
         * endpoints as GATT characteristics. Each GATT characteristic will be
         * formed using the primary service UUID as base, with different auto assigned
         * 12th and 13th bytes (assume counting starts from 0th byte). The client side
         * applications must identify the endpoints by reading the User Characteristic
         * Description descriptor (0x2901) for each characteristic, which contains the
         * endpoint name of the characteristic */
        uint8_t custom_service_uuid[] = {
            /* This is a random uuid. This can be modified if you want to change the BLE uuid. */
            /* 12th and 13th bit will be replaced by internal bits. */
            0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
            0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02,
        };
        err = wifi_prov_scheme_ble_set_service_uuid(custom_service_uuid);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "wifi_prov_scheme_ble_set_service_uuid failed %d", err);
            return err;
        }
#endif /* CONFIG_APP_WIFI_PROV_TRANSPORT_BLE */
        wifi_prov_mgr_endpoint_create("rainmaker-litemesh");
        /* Start provisioning service */
        ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security, NULL, service_name, service_key));
        wifi_prov_mgr_endpoint_register("rainmaker-litemesh", rainmaker_mesh_lite_handler, NULL);
        /* Print QR code for provisioning */
#ifdef CONFIG_APP_WIFI_PROV_TRANSPORT_BLE
        app_wifi_print_qr(service_name, pop, PROV_TRANSPORT_BLE);
#else /* CONFIG_APP_WIFI_PROV_TRANSPORT_SOFTAP */
        app_wifi_print_qr(service_name, pop, PROV_TRANSPORT_SOFTAP);
#endif /* CONFIG_APP_WIFI_PROV_TRANSPORT_BLE */
        intro_print(provisioned);
        ESP_LOGI(TAG, "Provisioning Started. Name : %s, POP : %s", service_name, pop);
    } else {
        ESP_LOGI(TAG, "Already provisioned, starting Wi-Fi STA");
        intro_print(provisioned);
        /* We don't need the manager as device is already provisioned,
         * so let's release it's resources */
        wifi_prov_mgr_deinit();

        /* Start Wi-Fi station */
        app_wifi_network_connect();
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_EVENT);
    }
    /* Wait for Wi-Fi connection */
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_EVENT, false, true, portMAX_DELAY);

    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));

    return ESP_OK;
}

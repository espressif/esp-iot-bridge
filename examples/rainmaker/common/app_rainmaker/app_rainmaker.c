/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_devices.h>
#include <esp_rmaker_ota.h>
#include <esp_rmaker_schedule.h>
#include <esp_rmaker_common_events.h>
#include <esp_rmaker_user_mapping.h>

#include <app_wifi.h>
#include <app_insights.h>
#include <app_light.h>

static const char *TAG = "app_rainmaker";

esp_rmaker_device_t *light_device;

extern const char ota_server_cert[] asm("_binary_server_crt_start");

/* Callback to handle commands received from the RainMaker cloud */
static esp_err_t write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
            const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx)
{
    session_cost_information(TAG, __func__, __LINE__, "write");
    if (ctx) {
        ESP_LOGI(TAG, "Received write request via : %s", esp_rmaker_device_cb_src_to_str(ctx->src));
    }
    const char *device_name = esp_rmaker_device_get_name(device);
    const char *param_name = esp_rmaker_param_get_name(param);
    if (strcmp(param_name, ESP_RMAKER_DEF_POWER_NAME) == 0) {
        ESP_LOGI(TAG, "Received value = %s for %s - %s",
                val.val.b? "true" : "false", device_name, param_name);
        app_light_set_power(val.val.b);
    } else if (strcmp(param_name, ESP_RMAKER_DEF_BRIGHTNESS_NAME) == 0) {
        ESP_LOGI(TAG, "Received value = %d for %s - %s",
                val.val.i, device_name, param_name);
        app_light_set_brightness(val.val.i);
    } else if (strcmp(param_name, ESP_RMAKER_DEF_HUE_NAME) == 0) {
        ESP_LOGI(TAG, "Received value = %d for %s - %s",
                val.val.i, device_name, param_name);
        app_light_set_hue(val.val.i);
    } else if (strcmp(param_name, ESP_RMAKER_DEF_SATURATION_NAME) == 0) {
        ESP_LOGI(TAG, "Received value = %d for %s - %s",
                val.val.i, device_name, param_name);
        app_light_set_saturation(val.val.i);
    } else {
        /* Silently ignoring invalid params */
        return ESP_OK;
    }
    esp_rmaker_param_update_and_report(param, val);
    return ESP_OK;
}

/* Event handler for catching RainMaker events */
static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == RMAKER_EVENT) {
        switch (event_id) {
            case RMAKER_EVENT_INIT_DONE:
                ESP_LOGI(TAG, "RainMaker Initialised.");
                break;
            case RMAKER_EVENT_CLAIM_STARTED:
                ESP_LOGI(TAG, "RainMaker Claim Started.");
                break;
            case RMAKER_EVENT_CLAIM_SUCCESSFUL:
                ESP_LOGI(TAG, "RainMaker Claim Successful.");
                break;
            case RMAKER_EVENT_CLAIM_FAILED:
                ESP_LOGI(TAG, "RainMaker Claim Failed.");
                break;
            default:
                ESP_LOGW(TAG, "Unhandled RainMaker Event: %d", event_id);
        }
    } else if (event_base == RMAKER_COMMON_EVENT) {
        switch (event_id) {
            case RMAKER_EVENT_REBOOT:
                ESP_LOGI(TAG, "Rebooting in %d seconds.", *((uint8_t *)event_data));
                break;
            case RMAKER_EVENT_WIFI_RESET:
                ESP_LOGI(TAG, "Wi-Fi credentials reset.");
                break;
            case RMAKER_EVENT_FACTORY_RESET:
                ESP_LOGI(TAG, "Node reset to factory defaults.");
                break;
            default:
                ESP_LOGW(TAG, "Unhandled RainMaker Common Event: %d", event_id);
        }
    } else {
        ESP_LOGW(TAG, "Invalid event received!");
    }
}

void app_rainmaker_start(void)
{
    /* Initialize Application specific hardware drivers and
     * set initial state.
     */
    app_driver_init();

    /* Register an event handler to catch RainMaker events */
    ESP_ERROR_CHECK(esp_event_handler_register(RMAKER_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));

    /* Initialize the ESP RainMaker Agent.
     * Note that this should be called after app_wifi_init() but before app_wifi_start()
     * */
    esp_rmaker_config_t rainmaker_cfg = {
        .enable_time_sync = false,
    };
    esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg, "ESP RainMaker Device", "Lightbulb");
    if (!node) {
        ESP_LOGE(TAG, "Could not initialise node. Aborting!!!");
        vTaskDelay(5000/portTICK_PERIOD_MS);
        abort();
    }
    esp_rmaker_node_add_attribute(node, "LiteMeshDevice", "1");

    /* Create a device and add the relevant parameters to it */
    light_device = esp_rmaker_lightbulb_device_create("Light", NULL, DEFAULT_POWER);
    esp_rmaker_device_add_cb(light_device, write_cb, NULL);

    esp_rmaker_device_add_param(light_device, esp_rmaker_brightness_param_create(ESP_RMAKER_DEF_BRIGHTNESS_NAME, DEFAULT_BRIGHTNESS));
    esp_rmaker_device_add_param(light_device, esp_rmaker_hue_param_create(ESP_RMAKER_DEF_HUE_NAME, DEFAULT_HUE));
    esp_rmaker_device_add_param(light_device, esp_rmaker_saturation_param_create(ESP_RMAKER_DEF_SATURATION_NAME, DEFAULT_SATURATION));

    esp_rmaker_node_add_device(node, light_device);

    esp_rmaker_system_serv_config_t serv_config = {
        .flags = SYSTEM_SERV_FLAGS_ALL,
        .reset_reboot_seconds = 2,
    };

    esp_rmaker_system_service_enable(&serv_config);

    /* Enable OTA */
    esp_rmaker_ota_config_t ota_config = {
        .server_cert = ota_server_cert,
    };
    esp_rmaker_ota_enable(&ota_config, OTA_USING_TOPICS);

    /* Enable timezone service which will be require for setting appropriate timezone
     * from the phone apps for scheduling to work correctly.
     * For more information on the various ways of setting timezone, please check
     * https://rainmaker.espressif.com/docs/time-service.html.
     */
    esp_rmaker_timezone_service_enable();

    /* Enable scheduling. */
    esp_rmaker_schedule_enable();

    /* Enable Insights. Requires CONFIG_ESP_INSIGHTS_ENABLED=y */
    app_insights_enable();

    /* Start the ESP RainMaker Agent */
    esp_rmaker_start();
}

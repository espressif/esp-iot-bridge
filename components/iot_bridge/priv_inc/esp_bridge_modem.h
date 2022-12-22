// Copyright 2022 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "esp_err.h"
#include "esp_log.h"
#include "esp_netif.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define ESP_MODEM_CHECK(a, str, goto_tag, ...)                                        \
    do                                                                                \
    {                                                                                 \
        if (!(a))                                                                     \
        {                                                                             \
            ESP_LOGE(TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__);     \
            goto goto_tag;                                                            \
        }                                                                             \
    } while (0)

#define MODEM_POWER_GPIO_INACTIVE_LEVEL     (1)
#define MODEM_POWER_GPIO_ACTIVE_MS          (500)
#define MODEM_POWER_GPIO_INACTIVE_MS        (8000)
#define MODEM_RESET_GPIO_INACTIVE_LEVEL     (1)
#define MODEM_RESET_GPIO_ACTIVE_MS          (200)
#define MODEM_RESET_GPIO_INACTIVE_MS        (5000)

#define LED_RED_SYSTEM_GPIO                 CONFIG_LED_RED_SYSTEM_GPIO
#define LED_BLUE_WIFI_GPIO                  CONFIG_LED_BLUE_WIFI_GPIO
#define LED_GREEN_4GMODEM_GPIO              CONFIG_LED_GREEN_4GMODEM_GPIO
#define MODEM_POWER_GPIO                    CONFIG_MODEM_POWER_GPIO
#define MODEM_RESET_GPIO                    CONFIG_MODEM_RESET_GPIO

#define MODEM_DEFAULT_CONFIG()                                   \
    {                                                            \
        .rx_buffer_size = 1024*15,                               \
                          .tx_buffer_size = 1024*15,                               \
                                            .line_buffer_size = 1600,                                \
                                                    .event_task_priority = CONFIG_USB_TASK_BASE_PRIORITY + 3,\
                                                            .event_task_stack_size = 3072                            \
    }

typedef struct {
    int rx_buffer_size;             /*!< USB RX Buffer Size */
    int tx_buffer_size;             /*!< USB TX Buffer Size */
    int line_buffer_size;           /*!< Line buffer size for command mode */
    int event_task_priority;        /*!< USB Event/Data Handler Task Priority*/
    int event_task_stack_size;      /*!< USB Event/Data Handler Task Stack Size*/
} modem_config_t;

/**
* @brief Initialize esp-modem units, DTE, DCE, ppp-netif and start ppp.
*
* @param[in] config: modem configuration
*
* @return
*     - instance: create modem netif instance successfully
*     - NULL: create modem netif instance failed because some error occurred
*/
esp_netif_t *esp_bridge_modem_init(modem_config_t *config);

/**
* @brief Force reset 4g module and development board.
*
* @return
 *    - ESP_OK: request mac successfully
*/
esp_err_t esp_modem_board_force_reset(void);

#ifdef __cplusplus
}
#endif

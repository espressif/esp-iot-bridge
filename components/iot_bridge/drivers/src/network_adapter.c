/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "soc/lldesc.h"
#include "sys/queue.h"
#include "soc/soc.h"
#include "sdkconfig.h"
#include <unistd.h>
#ifndef CONFIG_IDF_TARGET_ARCH_RISCV
#include "xtensa/core-macros.h"
#endif
#include "esp_private/wifi.h"
#include "interface.h"
#include "network_adapter.h"

#include "freertos/task.h"
#include "freertos/queue.h"
#ifdef CONFIG_BRIDGE_BT_ENABLED
#include "esp_bt.h"
#ifdef CONFIG_BT_HCI_UART_NO
#include "driver/uart.h"
#endif
#endif
#include "endian.h"

#ifdef CONFIG_BRIDGE_BT_ENABLED
#include "slave_bt.h"
#endif

extern esp_netif_t* network_adapter_netif;
static bool init_done = false;

static const char TAG[] = "NETWORK_ADAPTER";

#if CONFIG_ESP_WLAN_DEBUG
static const char TAG_RX[] = "H -> S";
static const char TAG_TX[] = "S -> H";
#endif

#ifdef CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS
#define STATS_TICKS                      pdMS_TO_TICKS(1000*2)
#define ARRAY_SIZE_OFFSET                5
#endif

#if defined(CONFIG_BRIDGE_EXTERNAL_NETIF_SPI) || defined(CONFIG_BRIDGE_DATA_FORWARDING_NETIF_SPI)
  #ifdef CONFIG_IDF_TARGET_ESP32S2
    #define TO_HOST_QUEUE_SIZE           5
  #else
    #define TO_HOST_QUEUE_SIZE           20
  #endif
#else
  #define TO_HOST_QUEUE_SIZE             100
#endif

#define MAC_LEN                 6

volatile uint8_t datapath = 0;
volatile uint8_t station_connected = 0;
volatile uint8_t softap_started = 0;
volatile uint8_t ota_ongoing = 0;

interface_context_t *if_context = NULL;
interface_handle_t *if_handle = NULL;

static QueueHandle_t meta_to_host_queue = NULL;
static QueueHandle_t to_host_queue[MAX_PRIORITY_QUEUES] = {NULL};

static void print_firmware_version()
{
	ESP_LOGI(TAG, "*********************************************************************");
#if defined(CONFIG_BRIDGE_EXTERNAL_NETIF_SPI) || defined(CONFIG_BRIDGE_DATA_FORWARDING_NETIF_SPI)
  #if BLUETOOTH_UART
	ESP_LOGI(TAG, "                Transport used :: SPI + UART                    ");
  #else
	ESP_LOGI(TAG, "                Transport used :: SPI only                      ");
  #endif
#else
  #if BLUETOOTH_UART
	ESP_LOGI(TAG, "                Transport used :: SDIO + UART                   ");
  #else
	ESP_LOGI(TAG, "                Transport used :: SDIO only                     ");
  #endif
#endif
	ESP_LOGI(TAG, "*********************************************************************");
}

static uint8_t get_capabilities()
{
	uint8_t cap = 0;

	ESP_LOGI(TAG, "Supported features are:");
#if defined(CONFIG_BRIDGE_EXTERNAL_NETIF_SPI) || defined(CONFIG_BRIDGE_DATA_FORWARDING_NETIF_SPI)
	ESP_LOGI(TAG, "- WLAN over SPI");
	cap |= ESP_WLAN_SPI_SUPPORT;
#else
	ESP_LOGI(TAG, "- WLAN over SDIO");
	cap |= ESP_WLAN_SDIO_SUPPORT;
#endif

#if CONFIG_ESP_SPI_CHECKSUM || CONFIG_ESP_SDIO_CHECKSUM
	cap |= ESP_CHECKSUM_ENABLED;
#endif

#ifdef CONFIG_BRIDGE_BT_ENABLED
	cap |= get_bluetooth_capabilities();
#endif
	ESP_LOGI(TAG, "capabilities: 0x%x", cap);

	return cap;
}

static esp_err_t compose_sta_if_pkt(interface_buffer_handle_t *buf_handle, void *buffer, uint16_t len, uint8_t flag)
{
	uint8_t *netif_buf = NULL;

	netif_buf = (uint8_t *) malloc(len);

	if (!netif_buf) {
		ESP_LOGE(TAG, "Netif Send packet: memory allocation failed");
		return ESP_FAIL;
	}

	memcpy(netif_buf, buffer, len);

	buf_handle->if_type = ESP_STA_IF;
	buf_handle->if_num = 0;
	buf_handle->payload_len = len;
	buf_handle->payload = netif_buf;
	buf_handle->flag = flag;
	buf_handle->priv_buffer_handle = netif_buf;
	buf_handle->free_buf_handle = free;

	return ESP_OK;
}

esp_err_t pkt_netif2driver(void *buffer, uint16_t len)
{
	if (!init_done) {
		return ESP_FAIL;
	}

	esp_err_t ret = ESP_OK;
	interface_buffer_handle_t buf_handle;
	memset(&buf_handle, 0x0, sizeof(interface_buffer_handle_t));

	if (!buffer || !datapath) {
		return ESP_OK;
	}

	ret = compose_sta_if_pkt(&buf_handle, buffer, len, 0);
	if (ret != ESP_OK) {
		return ESP_FAIL;
	}

	ret = send_to_host_queue(&buf_handle, PRIO_Q_OTHERS);

	return ret;
}

esp_err_t pkt_dhcp_status_change(void *buffer, uint16_t len)
{
	if (!init_done) {
		return ESP_FAIL;
	}

	esp_err_t ret = ESP_OK;
	interface_buffer_handle_t buf_handle;
	memset(&buf_handle, 0x0, sizeof(interface_buffer_handle_t));

	if (!buffer || !datapath) {
		return ESP_OK;
	}

	ret = compose_sta_if_pkt(&buf_handle, buffer, len, DHCPS_CHANGED);
	if (ret != ESP_OK) {
		return ESP_FAIL;
	}

	ret = send_to_host_queue(&buf_handle, PRIO_Q_OTHERS);

	return ret;
}

void process_tx_pkt(interface_buffer_handle_t *buf_handle)
{
	/* Check if data path is not yet open */
	if (!datapath) {
		/* Post processing */
		if (buf_handle->free_buf_handle && buf_handle->priv_buffer_handle) {
			buf_handle->free_buf_handle(buf_handle->priv_buffer_handle);
			buf_handle->priv_buffer_handle = NULL;
		}
		ESP_LOGD(TAG, "Data path stopped");
		usleep(100*1000);
		return;
	}
	if (if_context && if_context->if_ops && if_context->if_ops->write) {
		if_context->if_ops->write(if_handle, buf_handle);
	}
	/* Post processing */
	if (buf_handle->free_buf_handle && buf_handle->priv_buffer_handle) {
		buf_handle->free_buf_handle(buf_handle->priv_buffer_handle);
		buf_handle->priv_buffer_handle = NULL;
	}
}

/* Send data to host */
void send_task(void* pvParameters)
{
	uint8_t queue_type = 0;
	interface_buffer_handle_t buf_handle = {0};

	while (1) {

		if (!datapath) {
			usleep(100*1000);
			continue;
		}

		if (xQueueReceive(meta_to_host_queue, &queue_type, portMAX_DELAY))
			if (xQueueReceive(to_host_queue[queue_type], &buf_handle, portMAX_DELAY))
				process_tx_pkt(&buf_handle);
	}
}

void process_rx_pkt(interface_buffer_handle_t *buf_handle)
{
	struct esp_payload_header *header = NULL;
	uint8_t *payload = NULL;
	uint16_t payload_len = 0;

	header = (struct esp_payload_header *) buf_handle->payload;
	payload = buf_handle->payload + le16toh(header->offset);
	payload_len = le16toh(header->len);

#if CONFIG_ESP_WLAN_DEBUG
	ESP_LOG_BUFFER_HEXDUMP(TAG_RX, payload, 8, ESP_LOG_INFO);
#endif

	if ((buf_handle->if_type == ESP_STA_IF) && network_adapter_netif) {
		/* Forward data to lwip */
		esp_netif_receive(network_adapter_netif, payload, payload_len, NULL);
		// ESP_LOG_BUFFER_HEXDUMP("host -> slave", payload, payload_len, ESP_LOG_INFO);
	} else if (buf_handle->if_type == ESP_AP_IF && softap_started) {
		/* Forward data to wlan driver */
		esp_wifi_internal_tx(ESP_IF_WIFI_AP, payload, payload_len);
    }
#if defined(CONFIG_BRIDGE_BT_ENABLED) && BLUETOOTH_HCI
    else if (buf_handle->if_type == ESP_HCI_IF) {
        process_hci_rx_pkt(payload, payload_len);
    }
#endif

	/* Free buffer handle */
	if (buf_handle->free_buf_handle && buf_handle->priv_buffer_handle) {
		buf_handle->free_buf_handle(buf_handle->priv_buffer_handle);
		buf_handle->priv_buffer_handle = NULL;
	}
}

/* Get data from host */
void recv_task(void* pvParameters)
{
	interface_buffer_handle_t buf_handle = {0};

	for (;;) {

		if (!datapath) {
			/* Datapath is not enabled by host yet*/
			usleep(100*1000);
			continue;
		}

		/* receive data from transport layer */
		if (if_context && if_context->if_ops && if_context->if_ops->read) {
			int len = if_context->if_ops->read(if_handle, &buf_handle);
			if (len <= 0) {
				usleep(10*1000);
				continue;
			}
		}

		process_rx_pkt(&buf_handle);
	}
}

esp_err_t send_to_host_queue(interface_buffer_handle_t *buf_handle, uint8_t queue_type)
{
	int ret = xQueueSend(to_host_queue[queue_type], buf_handle, portMAX_DELAY);
	if (ret != pdTRUE) {
		ESP_LOGE(TAG, "Failed to send buffer into queue[%u]\n",queue_type);
		return ESP_FAIL;
	}

	ret = xQueueSend(meta_to_host_queue, &queue_type, portMAX_DELAY);
	if (ret != pdTRUE) {
		ESP_LOGE(TAG, "Failed to send buffer into meta queue[%u]\n",queue_type);
		return ESP_FAIL;
	}

	return ESP_OK;
}

int event_handler(uint8_t val)
{
	switch(val) {
		case ESP_OPEN_DATA_PATH:
			if (if_handle) {
				if_handle->state = ACTIVE;
				datapath = 1;
				ESP_EARLY_LOGI(TAG, "Start Data Path");
			} else {
				ESP_EARLY_LOGI(TAG, "Failed to Start Data Path");
			}
			break;

		case ESP_CLOSE_DATA_PATH:
			datapath = 0;
			if (if_handle) {
				ESP_EARLY_LOGI(TAG, "Stop Data Path");
				if_handle->state = DEACTIVE;
			} else {
				ESP_EARLY_LOGI(TAG, "Failed to Stop Data Path");
			}
			break;
	}
	return 0;
}

#ifdef CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS
/* These functions are only for debugging purpose
 * Please do not enable in production environments
 */
static esp_err_t print_real_time_stats(TickType_t xTicksToWait)
{
    TaskStatus_t *start_array = NULL, *end_array = NULL;
    UBaseType_t start_array_size, end_array_size;
    uint32_t start_run_time, end_run_time;
    esp_err_t ret;

    //Allocate array to store current task states
    start_array_size = uxTaskGetNumberOfTasks() + ARRAY_SIZE_OFFSET;
    start_array = malloc(sizeof(TaskStatus_t) * start_array_size);
    if (start_array == NULL) {
        ret = ESP_ERR_NO_MEM;
        goto exit;
    }
    //Get current task states
    start_array_size = uxTaskGetSystemState(start_array, start_array_size, &start_run_time);
    if (start_array_size == 0) {
        ret = ESP_ERR_INVALID_SIZE;
        goto exit;
    }

    vTaskDelay(xTicksToWait);

    //Allocate array to store tasks states post delay
    end_array_size = uxTaskGetNumberOfTasks() + ARRAY_SIZE_OFFSET;
    end_array = malloc(sizeof(TaskStatus_t) * end_array_size);
    if (end_array == NULL) {
        ret = ESP_ERR_NO_MEM;
        goto exit;
    }
    //Get post delay task states
    end_array_size = uxTaskGetSystemState(end_array, end_array_size, &end_run_time);
    if (end_array_size == 0) {
        ret = ESP_ERR_INVALID_SIZE;
        goto exit;
    }

    //Calculate total_elapsed_time in units of run time stats clock period.
    uint32_t total_elapsed_time = (end_run_time - start_run_time);
    if (total_elapsed_time == 0) {
        ret = ESP_ERR_INVALID_STATE;
        goto exit;
    }

    printf("| Task | Run Time | Percentage\n");
    //Match each task in start_array to those in the end_array
    for (int i = 0; i < start_array_size; i++) {
        int k = -1;
        for (int j = 0; j < end_array_size; j++) {
            if (start_array[i].xHandle == end_array[j].xHandle) {
                k = j;
                //Mark that task have been matched by overwriting their handles
                start_array[i].xHandle = NULL;
                end_array[j].xHandle = NULL;
                break;
            }
        }
        //Check if matching task found
        if (k >= 0) {
            uint32_t task_elapsed_time = end_array[k].ulRunTimeCounter - start_array[i].ulRunTimeCounter;
            uint32_t percentage_time = (task_elapsed_time * 100UL) / (total_elapsed_time * portNUM_PROCESSORS);
            printf("| %s | %"PRIu32" | %"PRIu32"%%\n", start_array[i].pcTaskName, task_elapsed_time, percentage_time);
        }
    }

    //Print unmatched tasks
    for (int i = 0; i < start_array_size; i++) {
        if (start_array[i].xHandle != NULL) {
            printf("| %s | Deleted\n", start_array[i].pcTaskName);
        }
    }
    for (int i = 0; i < end_array_size; i++) {
        if (end_array[i].xHandle != NULL) {
            printf("| %s | Created\n", end_array[i].pcTaskName);
        }
    }
    ret = ESP_OK;

exit:    //Common return path
	if (start_array)
		free(start_array);
	if (end_array)
		free(end_array);
    return ret;
}

void task_runtime_stats_task(void* pvParameters)
{
    while (1) {
        printf("\n\nGetting real time stats over %"PRIu32" ticks\n", STATS_TICKS);
        if (print_real_time_stats(STATS_TICKS) == ESP_OK) {
            printf("Real time stats obtained\n");
        } else {
            printf("Error getting real time stats\n");
        }
        vTaskDelay(pdMS_TO_TICKS(1000*2));
    }
}
#endif

void network_adapter_driver_init(void)
{
	uint8_t capa = 0;
	uint8_t prio_q_idx = 0;
	print_firmware_version();

	capa = get_capabilities();

#ifdef CONFIG_BRIDGE_BT_ENABLED
	esp_err_t ret;
	uint8_t mac[MAC_LEN] = {0};

	initialise_bluetooth();

	ret = esp_read_mac(mac, ESP_MAC_BT);
	if (ret) {
		ESP_LOGE(TAG,"Failed to read BT Mac addr\n");
	} else {
		ESP_LOGI(TAG, "ESP Bluetooth MAC addr: %2x-%2x-%2x-%2x-%2x-%2x\n",
				mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	}
#endif

	if_context = interface_insert_driver(event_handler);
#if defined(CONFIG_BRIDGE_EXTERNAL_NETIF_SPI) || defined(CONFIG_BRIDGE_DATA_FORWARDING_NETIF_SPI)
	datapath = 1;
#endif

	if (!if_context || !if_context->if_ops) {
		ESP_LOGE(TAG, "Failed to insert driver\n");
		return;
	}

	if_handle = if_context->if_ops->init();

	if (!if_handle) {
		ESP_LOGE(TAG, "Failed to initialize driver\n");
		return;
	}

	meta_to_host_queue = xQueueCreate(TO_HOST_QUEUE_SIZE*3, sizeof(uint8_t));
	assert(meta_to_host_queue);
	for (prio_q_idx=0; prio_q_idx<MAX_PRIORITY_QUEUES; prio_q_idx++) {
		to_host_queue[prio_q_idx] = xQueueCreate(TO_HOST_QUEUE_SIZE,
				sizeof(interface_buffer_handle_t));
		assert(to_host_queue[prio_q_idx]);
	}

	assert(xTaskCreate(recv_task , "recv_task" ,
			CONFIG_ESP_DEFAULT_TASK_STACK_SIZE, NULL ,
			CONFIG_ESP_DEFAULT_TASK_PRIO, NULL) == pdTRUE);
	assert(xTaskCreate(send_task , "send_task" ,
			CONFIG_ESP_DEFAULT_TASK_STACK_SIZE, NULL ,
			CONFIG_ESP_DEFAULT_TASK_PRIO, NULL) == pdTRUE);

	while (!datapath) {
		sleep(1);
	}

	/* send capabilities to host */
	generate_startup_event(capa);

	ESP_LOGI(TAG,"Initial set up done");

	init_done = true;
}

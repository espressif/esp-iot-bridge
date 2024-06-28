/*
 * SPDX-FileCopyrightText: 2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "lwip/netif.h"
#include "esp_private/wifi.h"

#include "esp_log.h"

#include "tusb_net.h"

extern bool s_wifi_is_connected;
static SemaphoreHandle_t Net_Semphore;

extern esp_netif_t* usb_netif;

bool tud_network_wait_xmit(uint32_t ms)
{
    if (xSemaphoreTake(Net_Semphore, ms / portTICK_PERIOD_MS) == pdTRUE) {
        xSemaphoreGive(Net_Semphore);
        return true;
    }
    return false;
}

esp_err_t pkt_netif2usb(void *buffer, uint16_t len)
{
    if (!tud_ready()) {
        return ERR_USE;
    }

    if (tud_network_wait_xmit(100)) {
        /* if the network driver can accept another packet, we make it happen */
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
        if (tud_network_can_xmit(len)) {
#else
        if (tud_network_can_xmit()) {
#endif /* ESP_IDF_VERSION >= 5.0.0 */
            // ESP_LOG_BUFFER_HEXDUMP(" netif ==> usb", buffer, len, ESP_LOG_INFO);
            tud_network_xmit(buffer, len);
        }
    }

    return ESP_OK;
}

void tusb_net_init(void)
{
    vSemaphoreCreateBinary(Net_Semphore);
}

//--------------------------------------------------------------------+
// tinyusb callbacks
//--------------------------------------------------------------------+

bool tud_network_recv_cb(const uint8_t *src, uint16_t size)
{
    // ESP_LOG_BUFFER_HEXDUMP(" usb ==> netif", src, size, ESP_LOG_INFO);
    esp_netif_receive(usb_netif, src, size, NULL);
    tud_network_recv_renew();
    return true;
}

uint16_t tud_network_xmit_cb(uint8_t *dst, void *ref, uint16_t arg)
{
    uint16_t len = arg;

    /* traverse the "pbuf chain"; see ./lwip/src/core/pbuf.c for more info */
    memcpy(dst, ref, len);
    return len;
}

void tud_network_init_cb(void)
{
    /* TODO */
}

void tud_network_idle_status_change_cb(bool enable)
{
    if (enable == true) {
        xSemaphoreGive(Net_Semphore);
    } else {
        xSemaphoreTake(Net_Semphore, 0);
    }
}

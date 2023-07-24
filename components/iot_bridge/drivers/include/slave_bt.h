/*
 * SPDX-FileCopyrightText: 2015-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifdef CONFIG_BRIDGE_BT_ENABLED

#ifdef CONFIG_IDF_TARGET_ESP32C3
  #if (CONFIG_BT_CTRL_MODE_EFF == 1)
    #define BLUETOOTH_BLE    1
  #elif (CONFIG_BT_CTRL_MODE_EFF == 2)
    #define BLUETOOTH_BT     2
  #elif (CONFIG_BT_CTRL_MODE_EFF == 3)
    #define BLUETOOTH_BT_BLE 3
  #endif

  #if defined(CONFIG_BT_CTRL_HCI_MODE_VHCI)
    #define BLUETOOTH_HCI    4
  #elif CONFIG_BT_CTRL_HCI_MODE_UART_H4
    #define BLUETOOTH_UART   1
  #endif

#else
  #if defined(CONFIG_BTDM_CONTROLLER_MODE_BLE_ONLY)
    #define BLUETOOTH_BLE    1
  #elif defined(CONFIG_BTDM_CONTROLLER_MODE_BR_EDR_ONLY)
    #define BLUETOOTH_BT     2
  #elif defined(CONFIG_BTDM_CONTROLLER_MODE_BTDM)
    #define BLUETOOTH_BT_BLE 3
  #endif

  #if defined(CONFIG_BTDM_CONTROLLER_HCI_MODE_VHCI)
    #define BLUETOOTH_HCI    4
  #elif CONFIG_BT_HCI_UART_NO
    #define BLUETOOTH_UART   CONFIG_BT_HCI_UART_NO
  #elif CONFIG_BTDM_CTRL_HCI_UART_NO
    #define BLUETOOTH_UART   CONFIG_BTDM_CTRL_HCI_UART_NO
  #endif

#endif

#ifdef BLUETOOTH_UART
  #include "driver/uart.h"
  #define BT_TX_PIN	5
  #define BT_RX_PIN	18
  #define BT_RTS_PIN	19
  #ifdef CONFIG_IDF_TARGET_ESP32C3
    #define BT_CTS_PIN	8
    #define GPIO_OUTPUT_PIN_SEL  ((1ULL<<BT_TX_PIN) | (1ULL<<BT_RTS_PIN))
    #define GPIO_INPUT_PIN_SEL   ((1ULL<<BT_RX_PIN) | (1ULL<<BT_CTS_PIN))
    #define UART_RX_THRS       (120)
  #else
    #define BT_CTS_PIN	23
  #endif
#elif BLUETOOTH_HCI
  void process_hci_rx_pkt(uint8_t *payload, uint16_t payload_len);
#endif

void deinitialize_bluetooth(void);
esp_err_t initialise_bluetooth(void);
uint8_t get_bluetooth_capabilities(void);

#endif /* CONFIG_BRIDGE_BT_ENABLED */

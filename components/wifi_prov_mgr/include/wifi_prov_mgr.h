/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @brief   Enable Wi-Fi Provisioning
 *
 */
void esp_bridge_wifi_prov_mgr(void);

/**
 * @brief   Get Wi-Fi Provisioning Status
 *
 * @return
 *        - true: Wi-Fi Provisioning still in progress
 *        - false:End of Wi-Fi Provisioning
 */
bool wifi_provision_in_progress(void);

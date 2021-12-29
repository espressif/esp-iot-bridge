// Copyright 2021 Espressif Systems (Shanghai) PTE LTD
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

#ifndef _LIGHT_NVS_H_
#define _LIGHT_NVS_H_

#include <stdint.h>
#include "esp_err.h"
#include "nvs_flash.h"

esp_err_t light_nvs_open(nvs_handle_t *handle);

esp_err_t light_nvs_store(nvs_handle_t handle, const char *key, const void *data, size_t length);

esp_err_t light_nvs_get_length(nvs_handle_t handle, const char *key, size_t *length);

esp_err_t light_nvs_restore(nvs_handle_t handle, const char *key, void *data, size_t length, bool *exist);

esp_err_t light_nvs_erase(nvs_handle_t handle, const char *key);

#endif /* _LIGHT_EXAMPLE_NVS_H_ */

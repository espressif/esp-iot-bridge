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

/**
  * @brief Check if the network segment is used to avoid conflicts.
  * 
  * @return
  *     - true :be used
  *     - false:not used
  */
bool esp_litemesh_network_segment_is_used(uint32_t ip);

/**
  * @brief Initialization Vendor IE.
  * 
  * @return
  *     - OK   : successful
  *     - Other: fail
  */
esp_err_t esp_litemesh_init(void);

void esp_litemesh_connect(void);

#ifdef __cplusplus
}
#endif

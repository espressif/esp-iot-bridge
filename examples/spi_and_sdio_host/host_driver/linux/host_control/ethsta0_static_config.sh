#!/bin/sh

# Copyright 2015-2021 Espressif Systems (Shanghai) PTE LTD
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

sudo ifconfig ethsta0 down
# sudo ifconfig ethsta0 hw ether e0:e2:e6:b1:f4:36
sudo ifconfig ethsta0 hw ether $1
sudo ifconfig ethsta0 192.168.4.2
sudo ifconfig ethsta0 up

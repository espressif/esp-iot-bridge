- [English Version](./User_Guide.md)

# ESP-IoT-Bridge 方案

本文档将介绍 ESP-IoT-Bridge 方案的配置流程和使用方法。
ESP-IoT-Bridge 方案主要针对 IoT 应用场景下的各种网络接口之间的连接与通信，如 SPI、SDIO、USB、Wi-Fi、以太网等网络接口。在该解决方案中，Bridge 设备既可以为其它设备提供上网功能，又可以作为连接远程服务器的独立设备。

# 目录

- [1 概述](#1-概述)
- [2 硬件准备](#2-硬件准备)
- [3 环境搭建](#3-环境搭建)
- [4 SDK 准备](#4-sdk-准备)
- [5 配置项介绍](#5-配置项介绍)
- [6 编译 烧写 监视输出](#6-编译-烧写-监视输出)
- [7 配网](#7-配网)
- [8 OTA](#8-ota)
- [9 方案优势](#9-方案优势)
- [10 GPIO Map](#10-gpio-map)

## 1 概述

乐鑫 ESP-IoT-Bridge 方案已经适配乐鑫多种芯片：

| 芯片     |  ESP-IDF Release/v5.0  |  ESP-IDF Release/v5.1  |  ESP-IDF Release/v5.2  |  ESP-IDF Release/v5.3  |  ESP-IDF Release/v5.4  |
| :------- | :--------------------: | :--------------------: | :--------------------: | :--------------------: | :--------------------: |
| ESP32    | ![alt text][supported] | ![alt text][supported] | ![alt text][supported] | ![alt text][supported] | ![alt text][supported] |
| ESP32-C3 | ![alt text][supported] | ![alt text][supported] | ![alt text][supported] | ![alt text][supported] | ![alt text][supported] |
| ESP32-S2 | ![alt text][supported] | ![alt text][supported] | ![alt text][supported] | ![alt text][supported] | ![alt text][supported] |
| ESP32-S3 | ![alt text][supported] | ![alt text][supported] | ![alt text][supported] | ![alt text][supported] | ![alt text][supported] |
| ESP32-C2 | ![alt text][supported] | ![alt text][supported] | ![alt text][supported] | ![alt text][supported] | ![alt text][supported] |
| ESP32-C6 |                        | ![alt text][supported] | ![alt text][supported] | ![alt text][supported] | ![alt text][supported] |
| ESP32-C5 |                        |                        |                        |                        | ![alt text][supported] |
| ESP32-C61|                        |                        |                        |                        | ![alt text][supported] |

[supported]: https://img.shields.io/badge/-%E6%94%AF%E6%8C%81-green "supported"

ESP-IoT-Bridge 方案提供多个网络接口，不同的网络接口可以分为两大类：

- 用于连接互联网的接口

- 用于帮助其他设备转发网络数据使其联网的接口

用户可以通过多种不同的网络接口组合来实现个性化的网络接口连接方案，最大程度地发挥乐鑫芯片的网络优势。

<img src="https://raw.githubusercontent.com/espressif/esp-iot-bridge/master/components/iot_bridge/docs/_static/esp_iot_bridge.png" style="zoom:80%;" />

根据接口的不同组合可以实现多种功能，如下表：

|                     | 无线热点     | 以太网接口   | USB 接口 | SPI 接口 | SDIO 接口 | Bluetooth LE 接口       | Thread 接口       |
| ------------------- | ------------ | ------------ | -------- | -------- | --------- | ----------------------- | ----------------- |
| **无线 Wi-Fi**      | Wi-Fi 路由器 | Wi-Fi 路由器 | 无线网卡 | 无线网卡 | 无线网卡  | Bluetooth LE 边界路由器 | Thread 边界路由器 |
| **以太网**          | Wi-Fi 路由器 | 暂不支持     | 有线网卡 | 有线网卡 | 有线网卡  | Bluetooth LE 边界路由器 | Thread 边界路由器 |
| **Cat.1 4G (UART)** | 4G 路由器    | 4G 路由器    | 4G 网卡  | 4G 网卡  | 4G 网卡   | Bluetooth LE 边界路由器 | Thread 边界路由器 |
| **Cat.1 4G (USB)**  | 4G 路由器    | 4G 路由器    | 不支持   | 4G 网卡  | 4G 网卡   | Bluetooth LE 边界路由器 | Thread 边界路由器 |

备注：

- **第一列的无线 Wi-Fi、以太网、Cat.1 4G (UART)、以及 Cat.1 4G (USB) 是连接到互联网的接口。**

- **第一行的无线热点、以太网接口、USB 接口、SPI 接口、SDIO 接口、Bluetooth LE 接口、以及 Thread 接口是为其它设备提供上网功能的接入接口。**


可以总结出上表主要涉及以下几种应用场景：Wi-Fi 路由器、4G 路由器、4G 网卡、无线网卡、有线网卡、Bluetooth LE 边界路由器和 Thread 边界路由器。ESP 芯片对这些场景的支持情况如下表所示：

| ESP 设备 | Wi-Fi 路由器            | 4G 路由器               | 4G 网卡                               | 无线网卡                              | 有线网卡   | Bluetooth LE 边界路由器 | Thread 边界路由器 |
| -------- | ---------------------- | ---------------------- | ------------------------------------ | ------------------------------------ | ------------------------------------ | ----- | ---- |
| ESP32    | ![alt text][supported] | ![alt text][supported] | ![alt text][supported]<br>(SDIO/SPI) | ![alt text][supported]<br>(SDIO/SPI) | ![alt text][supported]<br>(SDIO/SPI) | TODO  | TODO |
| ESP32-C3 | ![alt text][supported] | ![alt text][supported] | ![alt text][supported]<br>(SPI)      | ![alt text][supported]<br>(SPI)      | ![alt text][supported]<br>(SPI)      | TODO  | TODO |
| ESP32-S2 | ![alt text][supported] | ![alt text][supported] | ![alt text][supported]<br>(SPI)      | ![alt text][supported]<br>(USB/SPI)  | ![alt text][supported]<br>(USB/SPI)  | *N/A* | TODO |
| ESP32-S3 | ![alt text][supported] | ![alt text][supported] | ![alt text][supported]<br>(SPI)      | ![alt text][supported]<br>(USB/SPI)  | ![alt text][supported]<br>(USB/SPI)  | TODO  | TODO |
| ESP32-C2 | ![alt text][supported] | ![alt text][supported] | ![alt text][supported]<br>(SPI)      | ![alt text][supported]<br>(SPI)      | ![alt text][supported]<br>(SPI)      | TODO  | TODO |
| ESP32-C6 | ![alt text][supported] | ![alt text][supported] | ![alt text][supported]<br>(SPI)      | ![alt text][supported]<br>(SPI)      | ![alt text][supported]<br>(SPI)      | TODO  | TODO |
| ESP32-C5 | ![alt text][supported] | ![alt text][supported] | ![alt text][supported]<br>(SPI)      | ![alt text][supported]<br>(SPI)      | ![alt text][supported]<br>(SPI)      | TODO  | TODO |
| ESP32-C61| ![alt text][supported] | ![alt text][supported] | TODO                                 | TODO                                 | TODO                                 | TODO  | TODO |

备注：

- **ESP32 没有 USB 接口，ESP32-C3 的 USB 接口无法用于通讯。如需使用 <font color=red>USB 网卡 </font>或 <font color=red>Cat.1 4G(USB)</font> 功能，请选择 ESP32-S2 或 ESP32-S3。**
- **只有 ESP32 支持以太网接口，其它芯片需要外接 SPI 连接以太网芯片。关于 ESP32 MAC & PHY 配置，请参考 [配置 MAC 和 PHY](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_eth.html#configure-mac-and-phy)。**
- **使用 Thread 边界路由器时，需要搭配 802.15.4 芯片，如 ESP32-H2。**
- **对于 ESP32 SDIO 接口，硬件上有管脚上拉需求，具体请参考 [SD 上拉需求](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/sd_pullup_requirements.html)。**

### ​注意事项
- 从 ([a4ab5cc](https://github.com/espressif/esp-iot-bridge/commit/a4ab5ccdbe07329802fff2778c67496b83ecf0dd)) 之后 esp-iot-bridge 只支持 esp-idf/v5.x 版本。
- 自 **iot_bridge v1.0.0** 起，USB 功能需要使用 **ESP-IDF v5.1.4 或更高版本**。对于使用 **ESP-IDF 5.0-5.1.3** 的系统：
    - **推荐方案**：升级 ESP-IDF 至 ≥v5.1.4
    - **旧版兼容方案**：降级 iot_bridge 至 v0.11.9（当前最新版 esp_tinyusb 组件不支持 RNDIS 协议，如需 RNDIS 功能必须采用此特定配置）
        ```yml
        espressif/iot_bridge:
            version: "0.11.9"
        usb_device:
            path: components/usb/usb_device
            git: https://github.com/espressif/esp-iot-bridge.git
            rules:
            - if: "target in [esp32s2, esp32s3]"
            - if: "idf_version < 5.1.4"
        ```
    | 组件版本         | ESP-IDF版本       | USB支持  | RNDIS支持 | 解决方案                  |
    |------------------|-------------------|----------|-----------|---------------------------|
    | **iot_bridge ≥1.0.0** | **ESP-IDF ≥5.1.4** | ✅ 支持 | ❌ 不支持   | 使用 esp_tinyusb 组件 |
    | **iot_bridge ≥1.0.0** | **ESP-IDF 5.0-5.1.3** | ❌ 不支持 | ❌ 不支持   | 升级IDF **或** 采用旧版方案 ↓ |
    | **iot_bridge 0.11.9** | **ESP-IDF 5.0+** | ✅ 支持 | ✅ 支持（idf5.0-5.1.3） | <pre>espressif/iot_bridge:<br>  version: "0.11.9"<br>usb_device:<br>  path: components/usb/usb_device<br>  git: https://github.com/espressif/esp-iot-bridge.git<br>  rules:<br>  - if: "target in [esp32s2, esp32s3]"<br>  - if: "idf_version < 5.1.4"</pre> |

### 1.1 Wi-Fi 路由器

ESP-IoT-Bridge 设备通过 Wi-Fi 或者有线以太网网口连接至路由器，移动设备或者 PC 通过连接至 ESP-IoT-Bridge 设备的 SoftAP 热点或接入网口进行上网。

- 通过在 menuconfig（``Bridge Configuration`` > ``SoftAP Config``）中启用 ``BRIDGE_SOFTAP_SSID_END_WITH_THE_MAC``，可在 SoftAP SSID 末尾增加 MAC 信息。

- 除了 C2，单个设备最多支持 15 个 子设备同时连接（具体参考 [AP 基本配置](https://docs.espressif.com/projects/esp-idf/zh_CN/v5.1.2/esp32/api-guides/wifi.html#id39) 中 max_connection），多个子设备共享带宽。

- 若 ESP-IoT-Bridge 设备通过 Wi-Fi 连接至路由器，则需要进行配网操作，目前支持以下两种配网方式：

    > - [网页配网](#网页配网)
    > - [Wi-Fi Provisioning (Bluetooth LE) 配网](#wi-fi-provisioning-配网)（不支持 ESP32-S2）

<img src="https://raw.githubusercontent.com/espressif/esp-iot-bridge/master/components/iot_bridge/docs/_static/wifi_router.png" alt="wifi_router" style="zoom: 80%;" />

### 1.2 4G 路由器

ESP-IoT-Bridge 设备可搭载插有 SIM 卡的移动网络模块，移动网络模块联网后，PC 或 MCU 通过以太网接口或 SoftAP 接口接入以访问互联网。

以下模块已适配 **4G Cat.1**：

| UART      | USB             |
| --------- | --------------- |
| A7670C    | ML302-DNLM/CNLM |
| EC600N-CN | Air724UG-NFM    |
| SIM76000  | EC600N-CNLA-N05 |
|           | EC600N-CNLC-N06 |
|           | SIMCom A7600C1  |

<img src="https://raw.githubusercontent.com/espressif/esp-iot-bridge/master/components/iot_bridge/docs/_static/4g_router.png" alt="4g_router" style="zoom: 80%;" />

### 1.3 4G 网卡

ESP-IoT-Bridge 设备可搭载插有 SIM 卡的移动网络模块，网络模块联网后，可通过多个网络接口（SDIO/SPI）接入 PC 或 MCU，为设备提供上网能力。

<img src="https://raw.githubusercontent.com/espressif/esp-iot-bridge/master/components/iot_bridge/docs/_static/4g_nic.png" alt="4g_nic" style="zoom: 80%;" />

### 1.4 无线网卡

ESP-IoT-Bridge 设备可通过多个网络接口（USB/SDIO/SPI）接入 PC 或 MCU，在连接成功后，PC 或 MCU 等设备会新增一个网卡。待配网成功后，即可为设备提供上网能力。

- USB 线一端连接至 ESP32-S2/ESP32-S2S3 的 GPIO19/20，一端连接至 MCU

    |             | USB_DP | USB_DM |
    | ----------- | ------ | ------ |
    | ESP32-S2/S3 | GPIO20 | GPIO19 |

- 使用 SPI/SDIO 接口需要对 MCU(Host) 侧进行配置。具体依赖项设置引导，请参考 **[Linux_based_readme](./docs/Linux_based_readme.md)**。

- 关于 SDIO 硬件连线和 MCU(Host) 配置，请参考 **[SDIO_setup](./docs/SDIO_setup.md)**。

- 关于 SPI 硬件连线和 MCU(Host) 配置，请参考 **[SPI_setup](./docs/SPI_setup.md)**。

- 该方案需要进行配网操作，目前支持以下两种配网方式:

    > - [网页配网](#网页配网)
    > - [Wi-Fi Provisioning (Bluetooth LE) 配网](#wi-fi-provisioning-配网)（不支持 ESP32-S2）

<img src="https://raw.githubusercontent.com/espressif/esp-iot-bridge/master/components/iot_bridge/docs/_static/wireless_nic.png" alt="wireless_nic" style="zoom: 80%;" />

### 1.5 有线网卡

ESP-IoT-Bridge 设备可通过将以太网网线插入路由器 LAN 口连接网络，并通过多个网络接口（USB/SDIO/SPI）接入 PC 或 MCU，为设备提供上网能力。

- USB 线一端连接至 ESP32-S2/ESP32-S3 的 GPIO19/20，一端连接至 MCU

    |             | USB_DP | USB_DM |
    | ----------- | ------ | ------ |
    | ESP32-S2/S3 | GPIO20 | GPIO19 |

- 使用 SPI/SDIO 接口需要对 MCU(Host) 侧进行配置，具体依赖项设置引导，请参考 **[Linux_based_readme](./docs/Linux_based_readme.md)**。

- 关于 SDIO 硬件连线和 MCU(Host) 配置，请参考 **[SDIO_setup](./docs/SDIO_setup.md)**。

- 关于 SPI 硬件连线和 MCU(Host) 配置，请参考 **[SPI_setup](./docs/SPI_setup.md)**。

<img src="https://raw.githubusercontent.com/espressif/esp-iot-bridge/master/components/iot_bridge/docs/_static/wired_nic.png" alt="wired_nic" style="zoom: 80%;" />

## 2 硬件准备

- **Linux 环境**

Linux 环境是用于执行编译、烧写、运行等操作的必须环境。

> Windows 用户可安装虚拟机，在虚拟机中安装 Linux。

- **ESP 设备**

ESP 设备包括 ESP 芯片，ESP 模组，ESP 开发板等。

> - 对于**以太网路由器**、**以太网无线网卡**功能：
>     - ESP32 需要额外增加一个以太网 PHY 芯片
>     - 其它 ESP 芯片需要 SPI 转以太网芯片
> - 对于**随身 Wi-Fi** 功能，需要额外增加一个插有 SIM 卡的移动网络模块。

- **USB 线**

USB 线主要是用于连接 PC 和 ESP 设备、烧写/下载程序以及查看 log 等。


## 3 环境搭建

**如果您熟悉 ESP 开发环境，则可以很轻松理解下面步骤; 如果您不熟悉某个部分，比如编译或烧录，请参考官方文档 [ESP-IDF 编程指南](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/index.html)。**


### 3.1 下载和设置工具链

请参考 [Linux 平台工具链的标准设置](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/get-started/linux-macos-setup.html) 下载和设置工具链，用于项目编译。

### 3.2 烧录工具/下载工具获取

- 烧录工具位于 [esp-idf](https://github.com/espressif/esp-idf) 的 `./components/esptool_py/esptool/esptool.py` 中

- esptool 功能参考:

```
$ ./components/esptool_py/esptool/esptool.py --help
```

### 3.3 ESP-IoT-Bridge 仓库获取

```
$ git clone https://github.com/espressif/esp-iot-bridge.git
```

## 4 SDK 准备

- 获取 Espressif SDK [ESP-IDF](https://github.com/espressif/esp-idf)。

- 为确保成功获取了完整的 ESP-IDF，请在终端中输入 `idf.py --version`，如果输出结果类似于 `ESP-IDF v5.0.5-493-ga463942e14`，则代表安装成功。详细的安装和配置说明请参考[快速入门文档](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32s2/get-started/index.html)。

- 成功获取到 ESP-IDF 之后，请将 ESP-IDF 版本切换到 `release/v5.0`  及其以上版本。

- 由于 IoT-Bridge 组件的某些特性以及 ESP-IDF 的某些限制，组件在编译时将会给当前使用的 ESP-IDF 打上 [patch](https://github.com/espressif/esp-iot-bridge/tree/master/components/iot_bridge/patch)，为了避免对其他项目的影响，推荐为 IoT-Bridge 项目单独维护 ESP-IDF。

## 5 配置项介绍

**选择连接外部网络的接口**

<img src="https://raw.githubusercontent.com/espressif/esp-iot-bridge/master/components/iot_bridge/docs/_static/external.png" alt="external" style="zoom: 80%;" />

**选择为其它设备提供网络数据转发的接口**

<img src="https://raw.githubusercontent.com/espressif/esp-iot-bridge/master/components/iot_bridge/docs/_static/data_forwarding.png" alt="data_forwarding" style="zoom: 80%;" />

- 用户可选择不同的接口组合来实现相应的功能。

- 是否支持选择多个网络数据转发接口来给不同设备提供网络功能？

    | IDF Version               |          | 备注                          |
    | ------------------------- | -------- | ---------------------------- |
    | ESP-IDF Release/v5.0-v5.3 | **支持** | 目前不能同时选择 SDIO 和 SPI 接口 |

    ```
                                 +-- USB  <-+->  Computer
                                 |
    RasPi + ethsta0 +-- SPI -- ESP32 --> External WiFi（Router）
                                 |
                                 +-- SoftAP <-+-> Phone
    ```

**ETH 配置项**

<img src="https://raw.githubusercontent.com/espressif/esp-iot-bridge/master/components/iot_bridge/docs/_static/eth.png" alt="eth" style="zoom: 80%;" />

**Modem 配置项**

<img src="https://raw.githubusercontent.com/espressif/esp-iot-bridge/master/components/iot_bridge/docs/_static/modem.png" alt="modem" style="zoom: 80%;" />


## 6 编译 烧写 监视输出

### 6.1 编译

在 esp-iot-bridge 目录下执行：

```
$ idf.py menuconfig
```

根据 [5.配置项介绍](#5-配置项介绍) 选择合适的配置选项，配置完成之后执行以下命令生成 bin。

```
$ idf.py build
```

### 6.2 烧录程序 & 监视输出

使用 USB 线连接 ESP 设备和 PC，确保烧写端口正确。

#### 6.2.1 烧录程序

```
$ idf.py flash
```

#### 6.2.3 监视输出

```
$ idf.py monitor
```

> 也可使用组合命令 `idf.py build flash monitor` 一次性执行构建、烧录和监视过程。

## 7 配网

### 网页配网

PC 或 MCU 通过热点、USB、SPI、SDIO 或以太网连接至 ESP-IoT-Bridge 设备并成功获取到 IP 地址后，可以通过访问网关 IP 来进行网页配网。

<img src="https://raw.githubusercontent.com/espressif/esp-iot-bridge/master/components/iot_bridge/docs/_static/web_server.png" alt="web_server" style="zoom: 67%;" />


### Wi-Fi Provisioning 配网

#### 7.2.1 获取 APP

- Android:
    - [Bluetooth LE Provisioning app on Play Store](https://play.google.com/store/apps/details?id=com.espressif.provble).
    - GitHub 中源码: [esp-idf-provisioning-android](https://github.com/espressif/esp-idf-provisioning-android).
- iOS:
    - [Bluetooth LE Provisioning app on app store](https://apps.apple.com/in/app/esp-ble-provisioning/id1473590141)
    - GitHub 中源码: [esp-idf-provisioning-ios](https://github.com/espressif/esp-idf-provisioning-ios)

#### 7.2.2 扫描二维码

扫描如 log 显示的二维码进行配网操作

```
I (1604) QRCODE: {"ver":"v1","name":"PROV_806314","pop":"abcd1234","transport":"ble"}
I (1607) NimBLE: GAP procedure initiated: advertise;
I (1618) NimBLE: disc_mode=2
I (1622) NimBLE:  adv_channel_map=0 own_addr_type=0 adv_filter_policy=0 adv_itvl_min=256 adv_itvl_max=256
I (1632) NimBLE:


  █▀▀▀▀▀█ ▄█ ▄▄ █▄▀ ▀▀█▀█▀█ █▀▀▀▀▀█
  █ ███ █ ▄█▀█ ▄ ▄▄▄█▀ ▀  ▀ █ ███ █
  █ ▀▀▀ █ ▄ █ ▀▄ ▄█ ▄▀▀▀▀▀█ █ ▀▀▀ █
  ▀▀▀▀▀▀▀ ▀ ▀ █ ▀ █ ▀▄▀▄█ █ ▀▀▀▀▀▀▀
  ▀▀█▀█ ▀█▀█▀ ▀█▄ ▄▀▀▄▄▄▄█▄▀▄▀ ▀▄█▄
  ██▀▄ █▀▄██▄█▀▀ █ ▀█ ▀█▄▄ █▀▄  ▄█
  ▀ █   ▀▀ ▀▄▀▄▀ ▀█▀▀▄▄▄▄▀ ▀▄▀▀ ▄▀▄
  ▀▀█▄█▀▀▀▀▀▄ ▄▀ ▀▀▀▀▄ █▀▄▀█ ▀█ ▄▀▄
  ▀▀▄▄ █▀ ▀█▄ ▀▀▀▀█▀▀▄ ▄    ▄▀▀▀ █▄
  ▄▄█▄▄ ▀█  ▀█▀▀ ▀▄ ▄▄ ▄ ▄  ▄█▀ ▄▀▄
  ▄▀▀█▀ ▀  █▀▀▄█▀ ▄▀██▀  ▀▀▄▄█▀ ▄ ▄
  █  ▄▄█▀▄▄█  ▄ ▀█▀▀█▄ █▀▄█ █▀▄▄▄▄▄
  ▀ ▀▀  ▀▀▄▄█ ▀▀▀▀▄██▄ ▄ ▄█▀▀▀██▄▄█
  █▀▀▀▀▀█ ▀ ██▀ █▀  ▄  ▄ ▄█ ▀ █ ▄▀
  █ ███ █ █▀█▀█▀ ▀█▀█▄█▄█ █▀▀██▀▄▀
  █ ▀▀▀ █ █▀▄  ▀ █▄▀█▄██ ▄█ ▀█▄▀█▀
  ▀▀▀▀▀▀▀ ▀▀ ▀ ▀▀▀▀▀ ▀  ▀▀▀ ▀▀   ▀


I (1798) esp_bridge_wifi_prov_mgr: If QR code is not visible, copy paste the below URL in a browser.
https://espressif.github.io/esp-jumpstart/qrcode.html?data={"ver":"v1","name":"PROV_806314","pop":"abcd1234","transport":"ble"}
```

Note：

- 由于 ESP32-S2 不支持 BLE，故该配网方案不适用于 ESP32-S2
- `PROV_MODE` 默认为 `PROV_SEC2_DEV_MODE`，量产固件建议选为 `PROV_SEC2_PROD_MODE`，并添加自己的 `salt` 和 `verifier`，具体请参考 [wifi_prov_mgr.c](https://github.com/espressif/esp-iot-bridge/blob/master/components/wifi_prov_mgr/src/wifi_prov_mgr.c#L41)。

## 8 OTA

### 使用浏览器进行 OTA 固件升级

#### 简介

PC 或 MCU 通过热点、USB、SPI、SDIO 或以太网连接至 ESP-IoT-Bridge 设备并成功获取到 IP 地址后，可以通过访问网关 IP 来进行网页 OTA。浏览器打开 Web Server 的网页后，可以选择进入 OTA 升级页面，通过网页升级固件。

<img src="https://raw.githubusercontent.com/espressif/esp-iot-bridge/master/components/iot_bridge/docs/_static/web_ota.png" alt="web_ota" style="zoom: 67%;" />

## 9 方案优势

<table>
    <tr> <!-- 第一行数据 -->
        <th colspan="2"> 功能 </th>
        <th> 优势 </th>
        <th> 应用场景 </th>
    </tr>
    <tr> <!-- 第二行数据 -->
        <th rowspan="2"> Wi-Fi 路由器 </th> <!-- 居中显示；合并 2 行 -->
        <td> Ethernet/Station 转 SoftAP </td>
        <td> 减轻路由器负荷、扩大信号覆盖范围 </td>
        <td>
            ● 大户型、多层户型家庭 <br>
            ● 大型办公空间 <br>
            ● 会议现场 <br>
            ● 酒店 <br>
            ● 光伏逆变器、风电 <br>
        </td>
    </tr>
    <tr> <!-- 第三行数据 -->
        <td> Station 转 Ethernet </td>
        <td> 免驱动、可热插拔、使用简便、开发成本低 </td>
        <td> 为不具有 Wi-Fi 功能、需要通过网线联网的设备部署网络。<br>
            ● 考勤机 <br>
            ● 收款机 <br>
        </td>
    </tr>
    <tr> <!-- 第四行数据 -->
        <th rowspan="3"> 网卡 </th> <!-- 居中显示；合并 3 行 -->
        <td> 有线/无线网卡（USB） </td>
        <td> 免驱动、可热插拔、使用简便，开发成本低 </td>
        <td>
            ● 光伏逆变器、风电 <br>
            ● 视频监控、IP Camera <br>
            ● 工业控制面板 <br>
        </td>
    </tr>
    <tr> <!-- 第五行数据 -->
        <td> 有线/无线网卡（SPI/SDIO） </td>
        <td> 稳定、高速率 </td>
        <td>
            ● 光伏逆变器、风电 <br>
            ● 视频监控、IP Camera <br>
            ● 机器人 <br>
            ● 工业控制面板 <br>
        </td>
    </tr>
    <tr> <!-- 第六行数据 -->
        <td> 4G 网卡 </td>
        <td> 即插即用、无需配网、移动性强 </td>
        <td> 配合有线/无线网卡使用，为设备提供更广泛的上网选择。<br>
            ● 充电桩 <br>
            ● 考勤机 <br>
            ● 视频监控 <br>
            ● 环境监测 <br>
        </td>
    </tr>
    <tr> <!-- 第七行数据 -->
        <th colspan="2"> 4G 热点 </th>
        <td> 无线连接、无需配网、移动性强 </td>
        <td>
            ● 共享按摩椅、共享充电宝等共享场景 <br>
            ● 无人便利店 <br>
        </td>
    </tr>
    <tr> <!-- 第八行数据 -->
        <th colspan="2"> BLE 边界路由器 </th>
        <td> 使 BLE 设备与其他设备互联互通 </td>
        <td>
            ● 搭建 BLE 医疗传感生态 <br>
            ● 商超电子价签管理 <br>
            ● 整体智慧家居方案 <br>
        </td>
    </tr>
    <tr> <!-- 第九行数据 -->
        <th colspan="2"> Thread 边界路由器 </th>
        <td> 使 Thread 设备与其他设备互联互通 </td>
        <td> 智能家居设备互联 <br>
        </td>
    </tr>
<table>

**请参考 [ESP-IoT-Bridge 视频](https://www.bilibili.com/video/BV1VN411A7G3)，该视频演示了 ESP-IoT-Bridge 的部分功能。**

## 10 GPIO Map

<img src="https://raw.githubusercontent.com/espressif/esp-iot-bridge/master/components/iot_bridge/docs/_static/gpio_map.png" alt="gpio_map" style="zoom: 67%;" />

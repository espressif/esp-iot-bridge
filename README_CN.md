- [English Version](README.md)

# ESP-IoT-Bridge 方案框架

ESP-IoT-Bridge 方案框架是基于 IoT-Bridge 组件搭建的完整的项目目录结构框架，可供开发者参考使用。

ESP-IoT-Bridge 方案主要是针对 IoT 应用场景实现的各种网络接口之间的连接与通信，如 SPI、SDIO、USB、Wi-Fi、以太网等网络接口之间的相互连接，并可结合其他方案形成更复杂的应用场景，如 Wi-Fi Mesh Lite、Rainmaker 等。本方案广泛应用于无线热点、有线网卡、4G 上网等多种领域，更多方案介绍可参见 [User_Guide_CN.md](components/iot_bridge/User_Guide_CN.md)。

在 [examples](examples) 目录下，实现了一些常见应用场景的 demo，可供用户快速集成到自己的应用项目中。

- [examples/wifi_router](examples/wifi_router)：ESP-IoT-Bridge 设备通过 Wi-Fi 或者有线以太网网口连接至路由器，智能设备通过连接至 ESP-IoT-Bridge 设备的 SoftAP 热点进行上网。
- [examples/wireless_nic](examples/wireless_nic)：ESP-IoT-Bridge 设备可通过多个网络接口（USB/ETH/SPI/SDIO）接入 PC 或 MCU，在连接成功后，PC 或 MCU 等设备会新增一个网卡。待配网成功后，即可为设备提供上网能力。
- [examples/wired_nic](examples/wired_nic)：ESP-IoT-Bridge 设备可通过将以太网网线插入路由器 LAN 口连接网络，并通过多个网络接口（USB/SPI/SDIO）接入 PC 或 MCU，为设备提供上网能力。
- [examples/4g_hotspot](examples/4g_hotspot)：ESP-IoT-Bridge 设备在搭载插有 SIM 卡的移动网络模块后可将蜂窝网络转换为 Wi-Fi 信号，周围的智能设备连接其开启的热点后即可联网。
- [examples/4g_nic](examples/4g_nic)：ESP-IoT-Bridge 设备可搭载插有 SIM 卡的移动网络模块，网络模块联网后，可通过多个网络接口（ETH/SPI/SDIO）接入 PC 或 MCU，为设备提供上网能力。

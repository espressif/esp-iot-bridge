# ESP-IOT-Bridge 方案框架

- [English Version](README.md)

ESP-IOT-Bridge 方案框架是基于 ESP-IOT-Bridge 组件搭建的完整的项目目录结构框架，可供开发者参考使用。

ESP-IOT-Bridge 方案主要是针对 iot 应用场景实现的各种网络接口之间的桥接，如 SPI、SDIO、USB、Wi-Fi、以太网等网络接口之间的相互桥接，并可结合其他方案形成更复杂的应用场景，如 Wi-Fi Mesh Lite、Rainmaker 等。本方案广泛应用于无线热点、有线网卡、4G 上网等多种领域，更多方案介绍可参见 [User_Guide_CN.md](components/iot_bridge/User_Guide_CN.md)。

在 [examples](examples) 目录下，实现了一些常见应用场景的 demo，可供用户快速集成到自己的应用项目中。

- [examples/wifi_router](examples/wifi_router): ESP-IOT-Bridge 设备通过 Wi-Fi 或者有线以太网网口连接至路由器，智能设备通过连接至 ESP-IOT-Bridge 设备的 SoftAP 热点进行上网。
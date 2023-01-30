# ESP-IoT-Bridge Solution

- [中文版](README_CN.md)

The ESP-IoT-Bridge solution framework is a complete project directory structure framework based on the ESP-IoT-Bridge components, which can be used by developers for reference.

The ESP-IoT-Bridge scheme is mainly used to bridge various network interfaces in iot application scenarios, such as SPI, SDIO, USB, Wi Fi, Ethernet and other network interfaces. It can also be combined with other schemes to form more complex application scenarios, such as Wi Fi Mesh Lite, Rainmaker, etc. This solution is widely used in wireless hotspots, wired network cards, 4G Internet access and other fields. For more information, see [User_Guide.md](components/iot_bridge/User_Guide.md).

In the [examples](examples) directory, demos of some common application scenarios are implemented for users to quickly integrate into their own application projects.

- [examples/wifi_router](examples/wifi_router): The device based on the ESP-IoT-Bridge solution connects to the router through Wi-Fi or ethernet, and the smart device such as the phone can access the internet by connecting to the SoftAP hotspot provided by the ESP-IoT-Bridge device.

- [examples/4g_hotspot](examples/4g_hotspot): ESP-IoT-Bridge device can be equipped with a mobile network module with a SIM card and then convert the cellular network into a Wi-Fi signal. The surrounding smart devices can connect to the hotspot from the ESP-IoT-Bridge device to gain Internet access.

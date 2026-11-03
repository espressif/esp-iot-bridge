- [中文版](README_CN.md)

# ESP-IoT-Bridge Solution

The ESP-IoT-Bridge solution framework is a complete project directory structure framework based on the IoT-Bridge components, which can be used by developers for reference.

The ESP-IoT-Bridge solution connects network interfaces used in IoT applications, including SPI, SDIO, USB, Wi-Fi, and Ethernet. It can be combined with other solutions, such as Wi-Fi Mesh Lite and Rainmaker. Typical uses include wireless hotspots, wired network cards, and 4G Internet access. For more information, see [User_Guide.md](components/iot_bridge/User_Guide.md).

In the [examples](examples) directory, demos of some common application scenarios are implemented for users to quickly integrate into their own application projects.

- [examples/wifi_router](examples/wifi_router): The ESP-IoT-Bridge device connects to the router through Wi-Fi or Ethernet. A phone or another station reaches the Internet through the device SoftAP.
- [examples/wireless_nic](examples/wireless_nic): The ESP-IoT-Bridge device connects to a PC or MCU through USB, Ethernet, SPI, or SDIO. The host gains a network interface and can reach the Internet after the device is provisioned.
- [examples/wired_nic](examples/wired_nic): The ESP-IoT-Bridge device connects to the router LAN port with an Ethernet cable. A PC or MCU connects through USB, SPI, or SDIO and reaches the Internet.
- [examples/4g_hotspot](examples/4g_hotspot): The ESP-IoT-Bridge device uses a mobile network module and a SIM card, then converts the cellular network into a Wi-Fi hotspot.
- [examples/4g_nic](examples/4g_nic): The ESP-IoT-Bridge device uses a mobile network module and a SIM card. A PC or MCU connects through Ethernet, SPI, or SDIO and reaches the Internet.
- [examples/iperf/ethernet_wireless_nic](examples/iperf/ethernet_wireless_nic): The ESP-IoT-Bridge device connects to a PC over Ethernet and to the network through a Wi-Fi router, then measures throughput between the PC and the router.

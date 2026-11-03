# ESP-IoT-Bridge Component

[![Component Registry](https://components.espressif.com/components/espressif/iot_bridge/badge.svg)](https://components.espressif.com/components/espressif/iot_bridge)

- [User Guide](https://github.com/espressif/esp-iot-bridge/blob/master/components/iot_bridge/User_Guide.md)

ESP-IoT-Bridge connects network interfaces used in IoT applications, including SPI, SDIO, USB, Wi-Fi, and Ethernet. Espressif provides it as a bridge between those interfaces.

## Feature

- Provide Wi-Fi router function that can provide accessing the Internet for other devices
- Provide Wi-Fi network card function for other devices to access the Internet
- Provide wired ethernet network card function for other devices to access the Internet
- Provide 4G network card function for other devices to access the Internet
- Provide 4G Hotspot function that can provide accessing the Internet for other devices

## API

- Call this function after selecting the external netif and the data-forwarding netif in menuconfig (`Bridge Configuration`).

	```
	esp_bridge_create_all_netif()
	```

    > By default, call this function. To create one interface, call the matching function, such as `esp_bridge_create_station_netif()`.

- Register a callback that checks network-segment conflicts.

	```
	esp_bridge_network_segment_check_register
	```

- Check whether other data forwarding netif IP network segments conflict with incoming netif network segments.

	```
	esp_bridge_netif_network_segment_conflict_update
	```

    > Call it from the event handler when an external netif (Station, Ethernet, or 4G) gets an IP address.

## Add the Component to Your Project

Use `add-dependency` to add `iot_bridge`. CMake downloads the component during the build.

```
idf.py add-dependency "espressif/iot_bridge=*"
```

## Examples

Use `create-project-from-example` to create a project from an example.

```
idf.py create-project-from-example "espressif/iot_bridge=*:wifi_router"
```

The example is downloaded into the current directory. Build and flash it from there.

> Or you can download examples from `esp-iot-bridge` repository: [wifi_router](https://github.com/espressif/esp-iot-bridge/tree/master/examples/wifi_router)

## Q&A

Q1. I encountered the following problems when using the package manager

```
Executing action: create-project-from-example
CMakeLists.txt not found in project directory /home/username
```

A1. The package manager is too old. Run `pip install -U idf-component-manager` in the ESP-IDF environment.

# ESP-IoT-Bridge Component

[![Component Registry](https://components.espressif.com/components/espressif/iot_bridge/badge.svg)](https://components.espressif.com/components/espressif/iot_bridge)

- [User Guide](https://github.com/espressif/esp-iot-bridge/blob/master/components/iot_bridge/User_Guide.md)

ESP-IoT-Bridge solution is mainly aimed at bridging between various network interfaces in IoT application scenarios, such as SPI, SDIO, USB, Wi-Fi, Ethernet and other network interfaces. ESP-IoT-Bridge is a smart bridge solution offered by Espressif. 

## Feature

- Provide Wi-Fi router function that can provide accessing the Internet for other devices
- Provide Wi-Fi network card function for other devices to access the Internet
- Provide wired ethernet network card function for other devices to access the Internet
- Provide 4G network card function for other devices to access the Internet
- Provide 4G Hotspot function that can provide accessing the Internet for other devices

## API

- The application layer only needs to call this API at the code level, and select the corresponding `external netif` and `dataforwarding netif` under menuconfig(Bridge Configuration).

	```
	esp_bridge_create_all_netif()
	```

    > Note: By default, you only need to call this API, or you can manually call the corresponding netif api, such as `esp_bridge_create_xxx_netif`.

- Registered users check network segment conflict interface.

	```
	esp_bridge_network_segment_check_register
	```

- Check whether other data forwarding netif IP network segments conflict with incoming netif network segments.

	```
	esp_bridge_netif_network_segment_conflict_update
	```

    > Note: Generally, it is called in the event callback when external netif (Station, Ethernet, 4G) obtains IP.

## Add component to your project
Please use the component manager command `add-dependency` to add the `iot_bridge` to your project's dependency, during the CMake step the component will be downloaded automatically.

```
idf.py add-dependency "espressif/iot_bridge=*"
```

## Examples

Please use the component manager command `create-project-from-example` to create the project from example template.

```
idf.py create-project-from-example "espressif/iot_bridge=*:wifi_router"
```

Then the example will be downloaded in current folder, you can check into it for build and flash.

> Or you can download examples from `esp-iot-bridge` repository: [wifi_router](https://github.com/espressif/esp-iot-bridge/tree/master/examples/wifi_router)

## Q&A

Q1. I encountered the following problems when using the package manager

```
Executing action: create-project-from-example
CMakeLists.txt not found in project directory /home/username
```

A1. This is because an older version packege manager was used, please run `pip install -U idf-component-manager` in ESP-IDF environment to update.

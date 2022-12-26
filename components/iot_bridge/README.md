# ESP-IOT-Bridge Component

- [User Guide](./User_Guide.md)

ESP-IOT-Bridge solution is mainly aimed at bridging between various network interfaces in IoT application scenarios, such as SPI, SDIO, USB, Wi-Fi, Ethernet and other network interfaces. ESP-IOT-Bridge is a smart bridge solution offered by Espressif. 

## Feature

- Provide Wi-Fi router function that can provide accessing the internet for other devices
- Provide Wi-Fi network card function for other devices to access the internet
- Provide wired ethernet network card function for other devices to access the internet
- Provide 4G network card function for other devices to access the internet
- Provide 4G Hotspot function that can provide accessing the internet for other devices

## API

- The application layer only needs to call this API at the code level, and select the corresponding `external netif` and `dataforwarding netif` under menuconfig(Bridge Configuration).

	```
	esp_bridge_create_all_netif()
	```

​		Note: By default, you only need to call this API, or you can manually call the corresponding netif api, such as `esp_bridge_create_xxx_netif`.

- Registered users check network segment conflict interface.

	```
	esp_bridge_network_segment_check_register
	```

- Check whether other data forwarding netif IP network segments conflict with incoming netif network segments.

	```
	esp_bridge_netif_network_segment_conflict_update
	```

​		Note: Generally, it is called in the event callback when external netif (Station, Ethernet, 4G) obtains IP.

## Example

- [examples/wifi_router](https://github.com/espressif/esp-iot-bridge/blob/master/examples/wifi_router): The device based on the ESP-IOT-Bridge solution connects to the router through Wi-Fi or ethernet, and the smart device such as the phone can access the internet by connecting to the SoftAP hotspot provided by the ESP-IOT-Bridge device.
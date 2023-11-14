# ChangeLog

## v0.10.0 - 2023.11.13

### Enhancements

- Support ESP32C6 SDIO netif
- Add esp_bridge_wifi_set_config API
- Remove modom function on idf4.3

## v0.9.0 - 2023.9.21

### Enhancements

- Support SPI and SDIO drivers as external netif

## v0.8.0 - 2023.9.15

### Enhancements

- Update SPI and SDIO drivers

## v0.7.2 - 2023.8.10

### Enhancements

- Update iot_bridge component yml, the idf component manager now supports uploading components with rules.

## v0.7.1 - 2023.8.1

### Enhancements

- Add limit to the length of the SoftAP SSID and password strings

### Docs

- Update images url

## v0.7.0 - 2023.6.20

### Enhancements

- Add 4g nic, wired nic, wireless nic examples

## v0.6.0 - 2023.6.19

### Supported ESP-IDF Version

- Add v5.1

### Supported Socs

- ESP32-C6
- ESP32-H2

### Enhancements

- feature: Make sure SOFTAP_MAX_CONNECT_NUMBER is not larger than DHCPS_MAX_STATION_NUM

### Docs

- Update images and hyperlinks

## v0.5.0 - 2023.4.3

### Supported Socs

- ESP32-C2

## v0.4.0 - 2023.3.16

### Supported ESP-IDF Version

- Add v4.3

## v0.3.0 - 2023.3.9

### Enhancements

- Add multiple bridging interfaces(Ethernet, USB, SPI, SDIO)

## v0.2.0 - 2023.2.24

### Enhancements

- Provide 4G network card function for other devices to access the internet
- Provide 4G Hotspot function that can provide accessing the internet for other devices

## v0.1.0 - 2022.12.26

This is the first released version for iot-bridge component, more detailed descriptions about the project, please refer to [User_Guide](https://github.com/espressif/esp-iot-bridge/blob/master/components/iot_bridge/User_Guide.md).

### Enhancements

- Provide Wi-Fi router function that can provide accessing the internet for other devices
- Provide Wi-Fi network card function for other devices to access the internet
- Provide wired ethernet network card function for other devices to access the internet

### Supported Socs

- ESP32
- ESP32-C3
- ESP32-S2
- ESP32-S3

### Supported ESP-IDF Version

- v4.4
- v5.0

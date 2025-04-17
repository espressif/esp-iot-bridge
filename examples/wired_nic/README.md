| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-C5 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- | -------- |

# Wired NIC example

## Overview

This example focuses on the networking part, implementing packet forwarding between network interfaces. It can be connected to PC or MCU through various network interfaces (USB/SPI/SDIO). ESP32 series chips use NAT to forward data packets in and out of the Ethernet network interface.

<img src="https://raw.githubusercontent.com/espressif/esp-iot-bridge/master/components/iot_bridge/docs/_static/wired_nic_en.png" style="zoom:80%;" />

### How to use example

#### Hardware Required
**Required**
- An Ethernet cable
- An esp32 series development board
- A Micro-USB cable for power supply and programming

**Optional**
- A USB cable for communication
- Some DuPont cables to connect to the SPI or SDIO interface of the MCU

Follow detailed instructions provided specifically for this example.

#### Choose the interface used to provide network data forwarding for other devices

You can select the interface (USB/SPI/SDIO) connected to the PC/MCU in `Component config → Bridge Configuration → The interface used to provide network data forwarding for other devices` of `menuconfig`.

Note: The USB Network Class defaults to CONFIG_TINYUSB_NET_ECM. If you want to recognize the USB NIC device on a Windows computer, you need to select CONFIG_TINYUSB_NET_RNDIS.

#### Build and Flash
Run `idf.py flash monitor` to build, flash and monitor the project.

Once a complete flash process has been performed, you can use `idf.py app-flash monitor` to reduce the flash time.

(To exit the serial monitor, type `Ctrl-]`. Please reset the development board f you cannot exit the monitor.)

For more information, see [User_Guide.md](https://github.com/espressif/esp-iot-bridge/blob/master/components/iot_bridge/User_Guide.md).

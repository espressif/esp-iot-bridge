| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-C5 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- | -------- |

# 4G NIC example

## Overview

This example focuses on the networking part, enabling packet forwarding between network interfaces. It can be connected to PC or MCU through multiple network interfaces (ETH/SPI/SDIO), and ESP32 series chips use NAT to forward data packets in and out of the PPP network interface.

<img src="https://raw.githubusercontent.com/espressif/esp-iot-bridge/master/components/iot_bridge/docs/_static/4g_nic_en.png" style="zoom:80%;" />

### How to use example

#### Hardware Required

**Required**
- A 4G module
- An esp32 series development board
- A Micro-USB cable for power supply and programming

**Optional**
- One Ethernet cable
- A USB cable for communication
- Some DuPont cables to connect to the SPI or SDIO interface of the MCU

Follow detailed instructions provided specifically for this example.

#### Choose the interface of the modem

You can select the interface (UART or USB) connected to the Modem in `Component config → Bridge Configuration → Modem Configuration` of `menuconfig`.

#### Choose the interface used to provide network data forwarding for other devices

You can select the interface (ETH/SPI/SDIO) connected to the PC/MCU in `Component config → Bridge Configuration → The interface used to provide network data forwarding for other devices` of `menuconfig`.

#### Build and Flash
Run `idf.py flash monitor` to build, flash and monitor the project.

Once a complete flash process has been performed, you can use `idf.py app-flash monitor` to reduce the flash time.

(To exit the serial monitor, type `Ctrl-]`. Please reset the development board f you cannot exit the monitor.)

For more information, see [User_Guide.md](https://github.com/espressif/esp-iot-bridge/blob/master/components/iot_bridge/User_Guide.md).

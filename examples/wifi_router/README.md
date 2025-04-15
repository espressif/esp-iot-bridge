| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-C5 | ESP32-S2 | ESP32-S3 | ESP32-C61 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- | -------- | --------- |

# Wi-Fi Router example

## Overview

This example focuses on the networking part, enables forwarding packets between network interfaces. It creates a WiFi soft AP, which uses NAT to forward packets to and from the Station network interface.

<img src="https://raw.githubusercontent.com/espressif/esp-iot-bridge/master/components/iot_bridge/docs/_static/wifi_router_en.png" style="zoom:80%;" />

### How to use example

#### Hardware Required
- An esp32 series development board
- A Micro-USB cable for power supply and programming

Follow detailed instructions provided specifically for this example.

#### Build and Flash
Run `idf.py flash monitor` to build, flash and monitor the project.

Once a complete flash process has been performed, you can use `idf.py app-flash monitor` to reduce the flash time.

(To exit the serial monitor, type `Ctrl-]`. Please reset the development board f you cannot exit the monitor.)

For more information, see [User_Guide.md](https://github.com/espressif/esp-iot-bridge/blob/master/components/iot_bridge/User_Guide.md).

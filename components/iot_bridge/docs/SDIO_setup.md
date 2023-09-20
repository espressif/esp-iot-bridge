# Network Adapter Setup over SDIO

## 1. Setup

### 1.1 Hardware Setup

In this setup, ESP device acts as a SDIO peripheral and provides Wi-Fi capabilities to host. Please connect ESP device to Raspberry-Pi with jumper cables as mentioned below. It may be good to use small length cables to ensure signal integrity. Power ESP32 and Raspberry-Pi separately with a power supply that provide sufficient power. ESP32 can be powered through PC using micro-USB cable.

Raspberry-Pi pinout can be found [here!](https://pinout.xyz/pinout/sdio)

| Raspberry-Pi Pin | ESP32 Pin | ESP32-C6 Pin | Function |
|:-------:|:---------:|:--------:|:--------:|
| 13 | IO13+[pull-up](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/sd_pullup_requirements.html)| IO23+[pull-up](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c6/api-reference/peripherals/sd_pullup_requirements.html) | DAT3 |
| 15 | IO14 | IO19 | CLK |
| 16 | IO15+[pull-up](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/sd_pullup_requirements.html) | IO18+[pull-up](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c6/api-reference/peripherals/sd_pullup_requirements.html) | CMD |
| 18 | IO2+[pull-up](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/sd_pullup_requirements.html)| IO20+[pull-up](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c6/api-reference/peripherals/sd_pullup_requirements.html) | DAT0 |
| 22 | IO4+[pull-up](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/sd_pullup_requirements.html)| IO21+[pull-up](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c6/api-reference/peripherals/sd_pullup_requirements.html) | DAT1 |
| 31 | EN  | ESP Reset |
| 37 | IO12+[pull-up](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/sd_pullup_requirements.html)| IO22+[pull-up](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c6/api-reference/peripherals/sd_pullup_requirements.html) | DAT2 |
| 39 | GND | GND | GND|

Tested on Raspberry-Pi 3B+ and Raspberry-Pi 4B

### 1.2 Raspberry-Pi Software Setup

By default, the SDIO pins of Raspberry-pi are not configured and are internally used for built-in Wi-Fi interface. Please enable SDIO pins by appending following line to */boot/config.txt* file

```
dtoverlay=sdio,poll_once=off
dtoverlay=disable-bt
```

Please reboot Raspberry-Pi after changing this file.

## 2. Load ESP-IoT-Bridge Solution

### 2.1 Host Software

- Execute following commands in root directory of cloned ESP-IoT-Bridge repository on Raspberry-Pi

```
$ cd esp-iot-bridge/examples/spi_and_sdio_host/host_driver/linux/host_control
$ ./rpi_init.sh sdio
```

- This script compiles and loads host driver on Raspberry-Pi. 

### 2.2 Peripheral Firmware for ESP Device

<font color=red>**Note:** </font>

> 1.Please check ESP-IDF Setup and use appropriate ESP-IDF version
>
> 2.Please check menuconfig Setup and use **SDIO Interface**

#### Set up the ESP-IDF environment variables

- In the terminal where you are going to use ESP-IDF

```
$ . $HOME/esp/esp-idf/export.sh
```

#### Source Compilation

- In root directory of ESP-IoT-Bridge repository, execute below command

```
$ cd esp-iot-bridge
```

##### Using cmake

- Execute following command to configure the project

```
$ idf.py menuconfig
```

- This will open project configuration window. To select SDIO transport interface, navigate to `Bridge Driver Configuration -> Transport layer -> SDIO interface -> select` and exit from menuconfig.
- Use below command to compile and flash the project. Replace <serial_port> with ESP peripheral's serial port.

```
$ idf.py -p <serial_port> build flash
```

## 3. Checking the Setup for SDIO

Once ESP device has been flashed a valid firmware and booted successfully, you can see successful enumeration log on Raspberry-Pi side as:

```
$ dmesg
[   14.309956] Bluetooth: Core ver 2.22
[   14.310019] NET: Registered protocol family 31
[   14.310022] Bluetooth: HCI device and connection manager initialized
[   14.310038] Bluetooth: HCI socket layer initialized
[   14.310045] Bluetooth: L2CAP socket layer initialized
[   14.310069] Bluetooth: SCO socket layer initialized
[   14.327776] Bluetooth: HCI UART driver ver 2.3
[   14.327784] Bluetooth: HCI UART protocol H4 registered
[   14.328014] Bluetooth: HCI UART protocol Three-wire (H5) registered
[   14.328138] Bluetooth: HCI UART protocol Broadcom registered
[   14.650789] Bluetooth: BNEP (Ethernet Emulation) ver 1.3
[   14.650796] Bluetooth: BNEP filters: protocol multicast
[   14.650810] Bluetooth: BNEP socket layer initialized
[   14.715079] Bluetooth: RFCOMM TTY layer initialized
[   14.715109] Bluetooth: RFCOMM socket layer initialized
[   14.715137] Bluetooth: RFCOMM ver 1.11
[   20.556381] mmc1: card 0001 removed
[   69.245969] mmc1: queuing unknown CIS tuple 0x01 (3 bytes)
[   69.253368] mmc1: queuing unknown CIS tuple 0x1a (5 bytes)
[   69.256622] mmc1: queuing unknown CIS tuple 0x1b (8 bytes)
[   69.258842] mmc1: queuing unknown CIS tuple 0x80 (1 bytes)
[   69.258939] mmc1: queuing unknown CIS tuple 0x81 (1 bytes)
[   69.259035] mmc1: queuing unknown CIS tuple 0x82 (1 bytes)
[   69.260840] mmc1: queuing unknown CIS tuple 0x80 (1 bytes)
[   69.260939] mmc1: queuing unknown CIS tuple 0x81 (1 bytes)
[   69.261040] mmc1: queuing unknown CIS tuple 0x82 (1 bytes)
[   69.261236] mmc1: new SDIO card at address 0001
[   76.892073] esp32: loading out-of-tree module taints kernel.
[   76.893083] esp32: Resetpin of Host is 6
[   76.893202] esp32: Triggering ESP reset.
[   76.894498] esp_sdio: probe of mmc1:0001:1 failed with error -110
[   76.894566] esp_sdio: probe of mmc1:0001:2 failed with error -110
[   77.596173] mmc1: card 0001 removed
[   77.649578] mmc1: queuing unknown CIS tuple 0x01 (3 bytes)
[   77.657019] mmc1: queuing unknown CIS tuple 0x1a (5 bytes)
[   77.660243] mmc1: queuing unknown CIS tuple 0x1b (8 bytes)
[   77.662448] mmc1: queuing unknown CIS tuple 0x80 (1 bytes)
[   77.662545] mmc1: queuing unknown CIS tuple 0x81 (1 bytes)
[   77.662643] mmc1: queuing unknown CIS tuple 0x82 (1 bytes)
[   77.664445] mmc1: queuing unknown CIS tuple 0x80 (1 bytes)
[   77.664541] mmc1: queuing unknown CIS tuple 0x81 (1 bytes)
[   77.664637] mmc1: queuing unknown CIS tuple 0x82 (1 bytes)
[   77.664832] mmc1: new SDIO card at address 0001
[   77.665287] esp_probe: ESP network device detected
[   77.877892] Features supported are:
[   77.877901] 	 * WLAN
[   77.877906] 	 * BT/BLE
[   77.877911] 	   - HCI over SDIO
[   77.877916] 	   - BT/BLE dual mode
[   78.096179] esp_sdio: probe of mmc1:0001:2 failed with error -22
```

## 4. Configure ethsta0

After the SDIO driver is loaded successfully, `ifconfig -a` command will see ethsta0, use the following command to enable ethsta0

```
$ cd esp-iot-bridge/example/spi_and_sdio_host/host_driver/linux/host_control
$ ./ethsta0_config.sh 12:34:56:78:9a:bc
```

<font color=red>**Note:** </font>

> MAC address can be customized

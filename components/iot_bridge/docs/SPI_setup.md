# Network Adapter Setup over SPI

## 1. Setup

### 1.1 Hardware Setup

In this setup, ESP device acts as a SPI peripheral and provides Wi-Fi capabilities to host. Please connect ESP device to Raspberry-Pi with jumper cables as mentioned below. It may be good to use small length cables to ensure signal integrity. Power ESP32 and Raspberry-Pi separately with a power supply that provide sufficient power. ESP32 can be powered through PC using micro-USB cable.

Raspberry-Pi pinout can be found [here!](https://pinout.xyz/pinout/spi)

#### 1.1.1 Pin connections
| Raspberry-Pi Pin | ESP32 | ESP32-S2/S3 | ESP32-C2/C3/C6 | Function |
|:-------:|:---------:|:--------:|:--------:|:--------:|
| 24 | IO15 | IO10 | IO10 | CS0 |
| 23 | IO14 | IO12 | IO6 | SCLK |
| 21 | IO12 | IO13 | IO2 | MISO |
| 19 | IO13 | IO11 | IO7 | MOSI |
| 25 | GND | GND | GND | Ground |
| 15 | IO2 | IO2 | IO3 | Handshake |
| 13 | IO4 | IO4 | IO4 | Data Ready |
| 31 | EN  | RST | RST | ESP32 Reset |

Tested on Raspberry-Pi 3B+ and Raspberry-Pi 4B

### 1.2 Raspberry-Pi Software Setup

The SPI master driver is disabled by default on Raspberry-Pi OS. To enable it add following commands in */boot/config.txt* file

```
dtparam=spi=on
dtoverlay=disable-bt
```

In addition, below options are set since the SPI clock frequency in analyzer is observed to be smaller than the expected clock. This is Raspberry-Pi specific [issue](https://github.com/raspberrypi/linux/issues/2286).

```
core_freq=250
core_freq_min=250
```

Please reboot Raspberry-Pi after changing this file.

## 2. Load ESP-IoT-Bridge Solution

### 2.1 Host Software

- Execute following commands in root directory of cloned ESP-IoT-Bridge repository on Raspberry-Pi

```
$ cd esp-iot-bridge/examples/spi_and_sdio_host/host_driver/linux/host_control
$ ./rpi_init.sh spi
```

- This script compiles and loads host driver on Raspberry-Pi. It also creates virtual serial interface `/dev/esps0` which is used as a control interface for Wi-Fi on ESP peripheral

### 2.2 Peripheral Firmware for ESP Device

⚠️<font color=red>**Note:** </font>

> 1.Please check ESP-IDF Setup and use appropriate ESP-IDF version
>
> 2.Please check menuconfig Setup and use **SPI Interface**

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

⚠️<font color=red>**Note:** </font>

>Set target if the ESP32-S2, ESP32-S3 or ESP32-C3 is being used. Skip if ESP32 is being used

```
$ idf.py set-target esp32s2
```

or

```
$ idf.py set-target esp32c3
```

- Execute following command to configure project

```
$ idf.py menuconfig
```

- This will open project configuration window. To select SPI transport interface, navigate to `Bridge Driver Configuration -> Transport layer -> SPI interface -> select` and exit from menuconfig.
- Use below command to compile and flash the project. Replace <serial_port> with ESP peripheral's serial port.

```
$ idf.py -p <serial_port> build flash
```

## 3. Checking the Setup for SPI

Once ESP device has been flashed a valid firmware and booted successfully, you can see successful enumeration log on Raspberry-Pi side as:

```
$ dmesg
[   47.150740] OF: overlay: WARNING: memory leak will occur if overlay removed, property: /soc/spi@7e204000/spidev@0/status
[   47.346754] Bluetooth: Core ver 2.22
[   47.346812] NET: Registered protocol family 31
[   47.346815] Bluetooth: HCI device and connection manager initialized
[   47.346830] Bluetooth: HCI socket layer initialized
[   47.346837] Bluetooth: L2CAP socket layer initialized
[   47.346856] Bluetooth: SCO socket layer initialized
[   65.589406] esp32_spi: loading out-of-tree module taints kernel.
[   65.591409] esp32: Resetpin of Host is 6
[   65.591541] esp32: Triggering ESP reset.
[   65.593385] ESP32 device is registered to SPI bus [0],chip select [0]
[   66.201597] Received INIT event from esp32
[   66.201613] ESP32 capabilities: 0x78
[   66.619381] Bluetooth: BNEP (Ethernet Emulation) ver 1.3
[   66.619388] Bluetooth: BNEP filters: protocol multicast
[   66.619404] Bluetooth: BNEP socket layer initialized
```

## 4. Configure ethsta0

After the SPI driver is loaded successfully, `ifconfig -a` command will see ethsta0, use the following command to enable ethsta0

```
$ cd esp-iot-bridge/examples/spi_and_sdio_host/host_driver/linux/host_control
$ ./ethsta0_config.sh 12:34:56:78:9a:bc
```

⚠️<font color=red>**Note:** </font>

> MAC address can be customized

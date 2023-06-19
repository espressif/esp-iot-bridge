# 1. Quick Start Guide

- With the help of this guide, you can easily setup and start using Network Adapter solution with Raspberry-Pi as a host.

### Host Setup

Make sure that Raspberry-Pi is equipped with following:

- Linux Kernel Headers are installed

    ```
     $ sudo apt update
     $ sudo apt install raspberrypi-kernel-headers
    ```

- Following tools are installed on Raspberry-Pi:

    - Git
    - Raspi-gpio utility
    
    ```
     $ sudo apt install git raspi-gpio
    ```
    
- Python Requirement

    - Python 2.x or 3.x

        ```
         $ sudo apt install python
        ```

        or

        ```
         $ sudo apt install python3
        ```

# 2. Comprehensive Guide

### 2.1 Linux Host: Development Environment Setup

- This section list downs environment setup and tools needed to make ESP-IoT-Bridge solution work with Linux based host.
- If you are using Raspberry-Pi as a Linux host, both [section 2.1.1](#raspberry-pi-specific-setup) and [section 2.1.2](#additional-setup) are applicable.
- If you are using other Linux platform, skip to [section 2.1.2](#additional-setup)

#### Raspberry-Pi Specific Setup

This section identifies Raspberry-Pi specific setup requirements.

- Linux Kernel Setup

    - We recommend full version Raspbian install on Raspberry-Pi to ensure easy driver compilation.
    - Please make sure to use kernel version `v4.19` and above. Prior kernel versions may work, but are not tested.
    - Kernel headers are required for driver compilation. Please install them as:

    ```
     $ sudo apt update
     $ sudo apt install raspberrypi-kernel-headers
    ```

    - Verify that kernel headers are installed properly by running following command. Failure of this command indicates that kernel headers are not installed correctly. In such case, follow https://github.com/notro/rpi-source/wiki and run `rpi-source` to get current kernel headers. Alternatively upgrade/downgrade kernel and reinstall kernel headers.

    ```
     $ ls /lib/modules/$(uname -r)/build
    ```

- Additional Tools

    - Raspi-gpio utility:

        ```
         $ sudo apt install raspi-gpio
        ```

    - Bluetooth Stack and utilities:

        ```
         $ sudo apt install pi-bluetooth
        ```

#### Additional Setup

- Linux Kernel setup on non Raspberry-Pi

    - Please make sure to use kernel version `v4.19` and above. Prior kernel versions may work, but are not tested.
    - Please install kernel headers as those are required for driver compilation.
    - Verify that kernel headers are installed properly by running following command. Failure of this command indicates that kernel headers are not installed correctly.

    ```
     $ ls /lib/modules/$(uname -r)/build
    ```

- Install following tools on Linux Host machine.

    - Git
    - Python 2.x or 3.x: We have tested ESP-IoT-Bridge solution with python 2.7.13 and 3.5.3

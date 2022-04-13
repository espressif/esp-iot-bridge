# LED Light Example(Nove Home)

This example uses the Rainmaker cloud platform to demonstrate the ESP-Gateway **Wi-Fi router** function. User can configure the device through the `Nove Home` APP and successfully connect to the Rainmaker cloud. The device is connected to the cloud based on Rainmaker, It can also provide other devices with the ability to surf the Internet wirelessly, and form a network with the LiteMesh function, which greatly reduces the load on the router and expands the wireless communication range.

## Get Start

### 1. Apps

- [Google PlayStore](https://play.google.com/store/apps/details?id=com.espressif.novahome)
- [Apple App Store](https://apps.apple.com/us/app/nova-home/id1563728960)

### 2. Flash Key

Flash the CA&Certificate to the address corresponding to the `fctry` field in partition_table.
The address of different chips is different. For details, please refer to the path `example/rainmaker/led_light/partition_table`

<font color=red>**⚠️Note**</font>：

> Please contact sales@espressif.com for certificates

### 3. IDF environment setup & SDK

Refer to [README](../../../README_EN.md)

### 4. LiteMesh function

- You can choose whether to enable the LiteMesh function in the menuconfig `Gateway Configuration -> The Interface used to provide network data forwarding for other devices -> Enable Lite Mesh`. This example enables this function by default.
- If the LiteMesh function is enabled, the first networked device will connect to the target router and serve as the root node, and subsequent devices will be connected to the root node device and act as child nodes to form a LiteMesh network. For details, please refer to [LiteMesh ](../../../doc/LiteMesh.md).

### 5. Build & Flash

After the ESP-IDF environment is successfully set up, you can execute the following commands to compile and burn the firmware.

```
$ cd esp-gateway/examples/rainmaker/led_light
$ idf.py build
$ idf.py flash
```

### 6. Add devices to Nove Home

- Open `Nove Home`, the APP will automatically search for the device to be configured

<img src="../_static/find_devices.jpg" alt="find_devices" width="25%" div align=center />

- Add devices

<img src="../_static/select_devices.jpg" alt="select_devices" width="25%" div align=center />

- Enter distribution network information

<img src="../_static/select_network.jpg" alt="select_network" width="25%" div align=center />

- Pair and connect to configure the network

<img src="../_static/connect_ble.jpg" alt="connect_ble" width="25%" div align=center />

- Distribution network is successful

<img src="../_static/done.jpg" alt="done" width="25%" div align=center />

- LED control

<img src="../_static/control.jpg" alt="control" width="25%" div align=center />

### 7. Precautions

- Currently `Nove Home` only supports Wi-Fi Provisioning over Bluetooth Low Energy, so this example does not support ESP32-S2 chip currently.

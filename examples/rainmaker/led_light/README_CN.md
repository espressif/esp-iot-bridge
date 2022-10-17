# LED Light 示例(Nove Home)

本示例通过使用 Rainmaker 云平台来展示 ESP-Bridge **Wi-Fi 路由器**功能，用户可通过 `Nove Home` APP 来为设备配网并使设备成功连接至 Rainmaker 云端，设备本身基于 Rainmaker 实现自身连接云端的同时，还可以为其他无线设备提供无线上网的能力，搭配 LiteMesh 功能形成组网，极大程度上减轻路由器承载压力，同时扩大了无线通信范围。

## Get Start

### 1. Apps

- [Google PlayStore](https://play.google.com/store/apps/details?id=com.espressif.novahome)

- [Apple App Store](https://apps.apple.com/us/app/nova-home/id1563728960)

### 2. 获取 Key

目前设备获取证书的方式有两种：

- Self Claiming：配网后设备直接发 http 请求从服务器拉证书，只能在 ESP32-S3 和 ESP32-C3 上应用，由于与 MAC 绑定，每一台设备每次拉取的证书是一样的
- Assisted Claiming：配网时，手机 APP 跟服务器请求证书，随后通过蓝牙发送给设备，不和 MAC 绑定，默认一个账户只有 5 次申请的限制

目前 ESP32-S3 和 ESP32-C3 默认采用 Self-Claiming 的方式获取证书，ESP32 仅能使用 Assisted Claiming 方式获取证书。

>Nova Home 目前 Assisted Claiming 方式不稳定，若使用 ESP32 获取证书建议使用 [ESP Rainmaker](https://github.com/espressif/esp-rainmaker#phone-apps) APP 进行 Claiming

### 3. 编译环境搭建 & SDK 准备

参考 [README](../../../README.md)

### 4. LiteMesh 功能

- 可以在 menuconfig 配置 `Bridge Configuration -> The Interface used to provide network data forwarding for other devices -> Enable Lite Mesh` 中选择是否使能 LiteMesh 功能，本示例默认使能该功能。

- 若开启 LiteMesh 功能，第一个配网的设备会连接至目标路由器并作为 Root 根结点，之后的设备均会连接至根结点设备并作为子节点组成一个 LiteMesh 网络，详情请参考 [LiteMesh](../../../doc/LiteMesh.md)。

### 5. 固件编译 & 烧录

ESP-IDF 环境搭建成功后，即可执行以下命令进行固件编译和烧录。

```
$ cd esp-bridge/examples/rainmaker/led_light
$ idf.py build
$ idf.py flash
```

### 6. 在 Nove Home 中加入设备

- 打开 Nove Home，APP 自动搜索到待配网设备

<img src="../_static/find_devices.jpg" alt="find_devices" width="25%" div align=center />

- 添加对应的设备

<img src="../_static/select_devices.jpg" alt="select_devices" width="25%" div align=center />

- 输入配网信息

<img src="../_static/select_network.jpg" alt="select_network" width="25%" div align=center />

- 点击配对和连接进行配网

<img src="../_static/connect_ble.jpg" alt="connect_ble" width="25%" div align=center />

- 配网成功

<img src="../_static/done.jpg" alt="done" width="25%" div align=center />

- LED 控制

<img src="../_static/control.jpg" alt="control" width="25%" div align=center />

### 7. 注意事项

- 目前 Nove Home 仅支持 Wi-Fi Provisioning 配网（BLE），故该方案目前不支持 ESP32-S2 芯片

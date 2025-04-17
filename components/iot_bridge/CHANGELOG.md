# ChangeLog

## v1.0.1 - 2025.4.17

### Supported Socs

- ESP32-C61 ([bb5e179](https://github.com/espressif/esp-iot-bridge/commit/bb5e179672958eb4e59b4439f9cd04a2bf3c778b))

## v1.0.0 - 2025.4.15

### Breaking Change:

- Starting from iot_bridge v1.0.0, USB functionality requires ESP-IDF v5.1.4 or later. For systems using ESP-IDF versions between 5.0 and 5.1.3:
    - Recommended solution: Upgrade ESP-IDF to ≥v5.1.4
    - Legacy solution: Downgrade iot_bridge to v0.11.9 (The current latest version of esp_tinyusb does not support RNDIS. To use RNDIS functionality, you must select this specific implementation.)
        ```yml
        espressif/iot_bridge:
            version: "0.11.9"
        usb_device:
            path: components/usb/usb_device
            git: https://github.com/espressif/esp-iot-bridge.git
            rules:
            - if: "target in [esp32s2, esp32s3]"
            - if: "idf_version < 5.1.4"
        ```

    | Component Version | ESP-IDF Version | USB Support | RNDIS Support | Required Action |
    |-------------------|-----------------|-------------|---------------|-----------------|
    | **iot_bridge ≥1.0.0** | **ESP-IDF ≥5.1.4** | ✅ Full support | ❌ Not supported | Use modern USB stack |
    | **iot_bridge ≥1.0.0** | **ESP-IDF 5.0-5.1.3** | ❌ Not supported | ❌ Not supported | Upgrade IDF **or** see legacy solution ↓ |
    | **iot_bridge 0.11.9** | **ESP-IDF 5.0+** | ✅ Full support | ✅ Supported (ESP-IDF 5.0-5.1.3) | <pre>espressif/iot_bridge:<br>  version: "0.11.9"<br>usb_device:<br>  path: components/usb/usb_device<br>  git: https://github.com/espressif/esp-iot-bridge.git<br>  rules:<br>  - if: "target in [esp32s2, esp32s3]"<br>  - if: "idf_version < 5.1.4"</pre> |

### Feature

- feat: add host driver action debug log ([dc62187](https://github.com/espressif/esp-iot-bridge/commit/dc62187271cf295dc791f2070ec1304661fc3c38))
- feat: add host driver data debug log ([973a81d](https://github.com/espressif/esp-iot-bridge/commit/973a81de900c21743b153a1defb4027fc7aa528f))

### Bugfix

- fix: Adapt to ESP-IDF updates related to UART and Ethernet ([669edaf](https://github.com/espressif/esp-iot-bridge/commit/669edaf9962beb4cbc42da7b1ece05ef081b272d))
- fix: update usb_device's dependencies to fix IDF5.0 compilation error ([2f0f5d2](https://github.com/espressif/esp-iot-bridge/commit/2f0f5d245617126eb3a715c06dc4146f018f35fc))

### Chore

- Make usb_device a dependency of the iot_bridge component ([8ce55ae](https://github.com/espressif/esp-iot-bridge/commit/8ce55ae077a794977c200cec3b470b0d48bc9bcd))

### Patch

- Add IP_NAPT_ADD_FAILED_HOOK for monitoring napt status ([9c744cf](https://github.com/espressif/esp-iot-bridge/commit/9c744cfe91a3c9fe7ce160cd8ae17bbc973c3d1b))

## v0.11.9 - 2024.8.6

### Feature

- Support ESP32-C5 for esp-iot-bridge ([b2f0a28](https://github.com/espressif/esp-iot-bridge/commit/b2f0a28adc8b6a41e46a0dd18ab7f0d97bfeae92))

## v0.11.8 - 2024.7.1

### Feature

- feat: Add napt table clear ([5cf256c](https://github.com/espressif/esp-iot-bridge/commit/5cf256ceb9c7d9ee6796680011127f76d13c647f))
    - To ensure the fix works, please use the latest branch of IDF v5.x
- feat: Add some NAPT configurations for iot_bridge ([6417238](https://github.com/espressif/esp-iot-bridge/commit/6417238041dd3271db75d1fdb93d01d9117bf759))

### Bugfix

- fix: it causes infinite loop when set IP info of the netif if the netif is not the first one in the list ([ef320fa](https://github.com/espressif/esp-iot-bridge/commit/ef320fac8477cfd438b228a8aa416b0a0a088068))

## v0.11.7 - 2024.6.7

### Feature

- Support USB_BTH Class(Currently supported only in IDF versions tag 5.0 to 5.1.3) ([16772da](https://github.com/espressif/esp-iot-bridge/commit/16772da1117ee676da8f2dc0a98dbae6c274e61e))

### Bugfix

- fix(bridge_common): add null check for data_forwarding_netif when DNS info update ([e538c1a](https://github.com/espressif/esp-iot-bridge/commit/e538c1ac4ae562cec324173852898883a2b5e020))

## v0.11.6 - 2024.6.5

### Bugfix

- fix: Update if_key used to get ethernet netif ([8be4cee](https://github.com/espressif/esp-iot-bridge/commit/8be4cee48a4cf5e693f66ed94c9b8834861a2b76))

## v0.11.5 - 2024.6.3

### Feature

- Support ESP-IDF release/v5.2 & release/v5.3 ([a5a1ae56](https://github.com/espressif/esp-iot-bridge/commit/a5a1ae564853087bb8cf74d3635e892246c538ed))
- Add parameter in esp_bridge_netif_set_ip_info to check IP segment conflict. ([7212dc5](https://github.com/espressif/esp-iot-bridge/commit/7212dc50d3a6916bdf0b3e6334b88ee730975c35))
- Do not stop DHCP Server if ip info is identical when set SoftAP ip. ([a2d9498](https://github.com/espressif/esp-iot-bridge/commit/a2d94988cea07d69eb513c0a45693aea4c039e09))

### Bugfix

- fix(lwip/dhcp_server): Bind dhcps netif to avoid handling the dhcp packet from other netif ([bf4a5052](https://github.com/espressif/esp-iot-bridge/commit/bf4a50520f4111df52362b2a0a1dc3875cf6eb0f))
- Fix phone disconnected from SoftAP when provisioning wifi. ([f73f0d6](https://github.com/espressif/esp-iot-bridge/commit/f73f0d68afb99a6bf11d89423368076fb3f34d41))
- Add DHCP patch to update the DNS information of the dataforwarding netif when the DNS changes in the station. ([f73f0d6](https://github.com/espressif/esp-iot-bridge/commit/f73f0d68afb99a6bf11d89423368076fb3f34d41))

### Chore

- Remove macro definition from iot_bridge ([6f5e70b5](https://github.com/espressif/esp-iot-bridge/commit/6f5e70b5349d5808b46d1ae8502c49391056c945))
- Remove the support for idf4.x ([3388bde9](https://github.com/espressif/esp-iot-bridge/commit/3388bde999c880f558e3911c5b9d4d82724bc6bf))
- Post dhcps_change_cb when esp_bridge_netif_set_ip_info ([ecf7df94](https://github.com/espressif/esp-iot-bridge/commit/ecf7df9418d640b9d4eef5a9c25f6cc320a918f0))

### Doc

- docs: update readme ([fa19a2f8](https://github.com/espressif/esp-iot-bridge/commit/fa19a2f89b4b4bedefbfa6921238f583df877065))

## v0.11.4 - 2024.4.26

### Bugfix

- Fix the IP allocation conflict caused by updating data forwarding netif DNS information.

## v0.11.3 - 2024.4.7

### Chore

- include lwip/lwip_napt.h

## v0.11.2 - 2024.3.29

### Enhancements

- Support setting IP info for netifs

## v0.11.1 - 2024.2.18

### Chore

- Change some log output level
- fix undefined IP_NAPT_PORTMAP

## v0.11.0 - 2023.12.8

### Enhancements

- Update the DNS information of the external netif to the data forwarding netif.

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

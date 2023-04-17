# Patch for ESP-IDF

Since some bug fixes of esp-idf may not be synced to GitHub, you need to manually apply some patches to build the example.

## Apply Patch

You can apply the patch by entering the following command on the command line / terminal:

- If you need to communicate between different data forwarding interfaces **(ESP-IDF Release/v5.0)**, you need to apply `ip4_forward.patch`.

    ```
    cd $IDF_PATH/components/lwip/lwip
    git apply /path/to/esp-iot-bridge/components/iot_bridge/patch/ip4_forward.patch
    ```

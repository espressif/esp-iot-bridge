# Patch for ESP-IDF

Since some bug fixes of esp-idf may not be synced to GitHub, you need to manually apply some patches to build the example.

## Apply Patch

You can apply the patch by entering the following command on the command line / terminal:

- If you need to use **SDIO NIC** function **(ESP-IDF Release/v4.4)**, you need to apply idf_patch.patch.

    ```
    cd $IDF_PATH
    cp /path/to/esp-gateway/idf_patch/idf_patch.patch .
    git apply idf_patch.patch
    ```

- If you need to communicate between different data forwarding interfaces **(ESP-IDF Master)**, you need to apply ip4_forward.patch.

    ```
    cd $IDF_PATH/components/lwip/lwip
    cp /path/to/esp-gateway/idf_patch/ip4_forward.patch .
    git apply ip4_forward.patch
    ```

    

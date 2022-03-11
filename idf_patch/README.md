# Patch for ESP-IDF

Since some bug fixes of esp-idf may not be synced to GitHub, you need to manually apply some patches to build the example.

## Apply Patch

you can apply the patch by entering the following command on the command line / terminal:

```
cd $IDF_PATH
cp /path/to/esp-gateway/idf_patch/idf_patch.patch .
git apply idf_patch.patch
```


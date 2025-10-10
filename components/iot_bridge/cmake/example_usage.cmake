# Example: How other components can use the patch utility functions
#
# This file demonstrates how other components can include and use the patch
# utility functions provided by the iot_bridge component.

# Method 1: Include the patch utils directly (if you know the path)
# include(/path/to/esp-iot-bridge/components/iot_bridge/cmake/patch_utils.cmake)

# Method 2: If iot_bridge is in your component dependencies, you can reference it
# get_filename_component(IOT_BRIDGE_DIR "${CMAKE_CURRENT_LIST_DIR}/../iot_bridge" ABSOLUTE)
# include(${IOT_BRIDGE_DIR}/cmake/patch_utils.cmake)

# Usage examples:

# Example 1: Use default paths (patches.list and patch/ in current component directory)
# apply_patches_from_list()

# Example 2: Specify custom patches.list file
# apply_patches_from_list(PATCHES_LIST_FILE "${CMAKE_CURRENT_LIST_DIR}/my_patches.list")

# Example 3: Specify both custom patches.list and patch directory
# apply_patches_from_list(
#     PATCHES_LIST_FILE "${CMAKE_CURRENT_LIST_DIR}/custom_patches.list"
#     PATCH_BASE_DIR "${CMAKE_CURRENT_LIST_DIR}/patches"
# )

# Example 4: For a component that wants to use a different structure
# apply_patches_from_list(
#     PATCHES_LIST_FILE "${CMAKE_CURRENT_LIST_DIR}/config/patches.ini"
#     PATCH_BASE_DIR "${CMAKE_CURRENT_LIST_DIR}/files/patches"
# )

# Example patches.list with various version operators:
#
# [legacy_fix.patch]
#     path = esp-idf/components/lwip
#     idf_version < 5.4
#
# [new_feature.patch]
#     path = esp-idf/components/driver
#     idf_version >= 5.4
#
# [specific_bug_fix.patch]
#     path = esp-idf/components/esp_system
#     idf_version == 5.3.1
#
# [compatibility_patch.patch]
#     path = esp-idf/components/freertos
#     idf_version <= 5.3

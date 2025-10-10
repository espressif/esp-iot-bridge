# Patch Utilities for ESP-IDF Components

This directory contains reusable CMake functions for applying patches from configuration files.

## Files

- `patch_utils.cmake` - Core utility functions for patch management
- `example_usage.cmake` - Usage examples and documentation
- `README.md` - This documentation file

## Overview

The patch utility system allows ESP-IDF components to apply patches automatically during the build process based on configuration files. This provides a flexible way to manage patches across different ESP-IDF versions and configurations.

## Core Functions

### `apply_patches_from_list()`

Main function to apply patches based on a configuration file.

**Parameters:**
- `PATCHES_LIST_FILE` (optional) - Path to the patches configuration file
  - Default: `${CMAKE_CURRENT_LIST_DIR}/patch/patches.list`
- `PATCH_BASE_DIR` (optional) - Base directory containing patch files
  - Default: `${CMAKE_CURRENT_LIST_DIR}/patch`

### `apply_single_patch()`

Internal function to apply a single patch with version checking.

## Configuration File Format

The patches configuration file (`patches.list`) uses an INI-like format:

```ini
[patch_filename.patch]
    path = /path/to/working/directory
    idf_version < 5.5

[new_feature.patch]
    path = $ENV{IDF_PATH}/components/lwip/lwip
    idf_version >= 5.4

[version_specific.patch]
    path = $ENV{IDF_PATH}/components/driver
    idf_version == 5.3.1
```

**Supported options:**
- `path` - Working directory for applying the patch (supports environment variables)
- `idf_version <operator> <version>` - Version constraint with flexible comparison operators:
  - `idf_version < 5.4` - Apply only if current version is less than 5.4
  - `idf_version <= 5.4` - Apply only if current version is less than or equal to 5.4
  - `idf_version > 5.3` - Apply only if current version is greater than 5.3
  - `idf_version >= 5.4` - Apply only if current version is greater than or equal to 5.4
  - `idf_version = 5.4` or `idf_version == 5.4` - Apply only if current version equals 5.4

## Usage in Other Components

### Method 1: Direct Include

```cmake
# Include the patch utilities
include(/path/to/esp-iot-bridge/components/iot_bridge/cmake/patch_utils.cmake)

# Apply patches with default settings
apply_patches_from_list()
```

### Method 2: With Custom Parameters

```cmake
# Include the patch utilities
include(/path/to/esp-iot-bridge/components/iot_bridge/cmake/patch_utils.cmake)

# Apply patches with custom configuration
apply_patches_from_list(
    PATCHES_LIST_FILE "${CMAKE_CURRENT_LIST_DIR}/my_patches.list"
    PATCH_BASE_DIR "${CMAKE_CURRENT_LIST_DIR}/patches"
)
```

### Method 3: Component Dependency

If your component depends on `iot_bridge`, you can reference the utilities:

```cmake
# Get the iot_bridge component path
get_filename_component(IOT_BRIDGE_DIR "${CMAKE_CURRENT_LIST_DIR}/../iot_bridge" ABSOLUTE)
include(${IOT_BRIDGE_DIR}/cmake/patch_utils.cmake)

apply_patches_from_list()
```

## Features

- **Automatic patch detection**: Skips patches that are already applied
- **Version constraints**: Apply patches only for specific ESP-IDF versions
- **Flexible paths**: Support for different working directories per patch
- **Environment variable expansion**: Support for `$ENV{VAR}` syntax in paths
- **Error handling**: Proper error messages and warnings for missing files
- **Reusable**: Can be used by any ESP-IDF component

## Example Directory Structure

```
your_component/
├── CMakeLists.txt
├── patch/
│   ├── patches.list
│   ├── fix_bug.patch
│   └── add_feature.patch
└── src/
    └── your_code.c
```

## Error Handling

The system provides comprehensive error handling:
- Warnings for missing patch files or configuration files
- Fatal errors if patch application fails
- Status messages for successful operations and skipped patches

## Compatibility

This utility system is compatible with:
- ESP-IDF v4.4+
- CMake 3.16+
- Git (required for patch application)

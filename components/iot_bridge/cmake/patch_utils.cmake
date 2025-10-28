# CMake utility functions for applying patches from configuration files
#
# This file provides reusable functions for applying patches based on
# configuration files in INI-like format.

function(apply_patches_from_list)
    # Parse arguments: patches_list_file and optional patch_base_dir
    set(options "")
    set(oneValueArgs PATCHES_LIST_FILE PATCH_BASE_DIR)
    set(multiValueArgs "")
    cmake_parse_arguments(PATCHES "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Set default values (caller's context)
    if(NOT PATCHES_PATCHES_LIST_FILE)
        set(patches_list_file "${CMAKE_CURRENT_LIST_DIR}/patches/patches.list")
    else()
        set(patches_list_file "${PATCHES_PATCHES_LIST_FILE}")
    endif()

    if(NOT PATCHES_PATCH_BASE_DIR)
        set(patch_base_dir "${CMAKE_CURRENT_LIST_DIR}/patches")
    else()
        set(patch_base_dir "${PATCHES_PATCH_BASE_DIR}")
    endif()

    # Check if patches.list file exists
    if(NOT EXISTS ${patches_list_file})
        message(WARNING "patches.list file not found at ${patches_list_file}")
        return()
    endif()

    # Read patches.list file
    file(READ ${patches_list_file} patches_content)

    # Split content by lines
    string(REPLACE "\n" ";" lines "${patches_content}")

    set(current_patch "")
    set(current_path "")
    set(current_version_condition "")
    set(current_version_operator "")

    foreach(line ${lines})
        # Trim whitespace
        string(STRIP "${line}" line)

        # Skip empty lines
        if("${line}" STREQUAL "")
            continue()
        endif()

        # Check if this is a section header [patch_file]
        if("${line}" MATCHES "^\\[(.+)\\]$")
            # If we have a previous patch, process it
            if(NOT "${current_patch}" STREQUAL "")
                apply_single_patch("${current_patch}" "${current_path}" "${current_version_condition}" "${current_version_operator}" "${patch_base_dir}")
            endif()

            # Start new patch
            string(REGEX REPLACE "^\\[(.+)\\]$" "\\1" current_patch "${line}")
            set(current_path "")
            set(current_version_condition "")
            set(current_version_operator "")

        # Check if this is a path line
        elseif("${line}" MATCHES "^path[ ]*=[ ]*(.+)$")
            string(REGEX REPLACE "^path[ ]*=[ ]*(.+)$" "\\1" current_path "${line}")
            # Expand environment variables
            if("${current_path}" MATCHES "^esp-idf")
                if("$ENV{IDF_PATH}" STREQUAL "")
                    message(FATAL_ERROR "IDF_PATH environment variable is not set")
                endif()
                # Normalize IDF_PATH to forward slashes to avoid backslash escape sequences in replacement
                set(_idf_path_raw "$ENV{IDF_PATH}")
                file(TO_CMAKE_PATH "${_idf_path_raw}" _idf_path_norm)
                string(REGEX REPLACE "^esp-idf" "${_idf_path_norm}" current_path "${current_path}")
            endif()

        # Check if this is a version condition (support multiple operators: <, <=, >, >=, =, ==)
        elseif("${line}" MATCHES "^idf_version[ ]*(>=|<=|>|<|==|=)[ ]*(.+)$")
            string(REGEX REPLACE "^idf_version[ ]*(>=|<=|>|<|==|=)[ ]*(.+)$" "\\1" current_version_operator "${line}")
            string(REGEX REPLACE "^idf_version[ ]*(>=|<=|>|<|==|=)[ ]*(.+)$" "\\2" current_version_condition "${line}")
        endif()
    endforeach()

    # Process the last patch
    if(NOT "${current_patch}" STREQUAL "")
        apply_single_patch("${current_patch}" "${current_path}" "${current_version_condition}" "${current_version_operator}" "${patch_base_dir}")
    endif()
endfunction()

function(apply_single_patch patch_file patch_path version_condition version_operator patch_base_dir)
    # Check version condition if specified
    if(NOT "${version_condition}" STREQUAL "")
        set(current_version "${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}.${IDF_VERSION_PATCH}")

        # Default to '<' operator for backward compatibility
        if("${version_operator}" STREQUAL "")
            set(version_operator "<")
        endif()

        set(skip_patch FALSE)

        # Compare versions based on operator
        if("${version_operator}" STREQUAL "<")
            if(NOT "${current_version}" VERSION_LESS "${version_condition}")
                set(skip_patch TRUE)
            endif()
        elseif("${version_operator}" STREQUAL "<=")
            if("${current_version}" VERSION_GREATER "${version_condition}")
                set(skip_patch TRUE)
            endif()
        elseif("${version_operator}" STREQUAL ">")
            if(NOT "${current_version}" VERSION_GREATER "${version_condition}")
                set(skip_patch TRUE)
            endif()
        elseif("${version_operator}" STREQUAL ">=")
            if("${current_version}" VERSION_LESS "${version_condition}")
                set(skip_patch TRUE)
            endif()
        elseif("${version_operator}" STREQUAL "=" OR "${version_operator}" STREQUAL "==")
            if(NOT "${current_version}" VERSION_EQUAL "${version_condition}")
                set(skip_patch TRUE)
            endif()
        else()
            message(WARNING "Unsupported version operator: ${version_operator}, skipping patch ${patch_file}")
            set(skip_patch TRUE)
        endif()

        if(skip_patch)
            message(STATUS "Skip ESP-IDF Patch: ${patch_file} (version ${current_version} does not satisfy ${version_operator} ${version_condition})")
            return()
        endif()
    endif()

    # Use default path if not specified
    if("${patch_path}" STREQUAL "")
        if("$ENV{IDF_PATH}" STREQUAL "")
            message(FATAL_ERROR "IDF_PATH environment variable is not set and no patch path specified")
        endif()
        set(patch_path "$ENV{IDF_PATH}")
    endif()

    set(patch_file_path "${patch_base_dir}/${patch_file}")

    # Check if patch file exists
    if(NOT EXISTS ${patch_file_path})
        message(WARNING "Patch file not found: ${patch_file_path}")
        return()
    endif()
    message("patch_file_path: ${patch_file_path}")
    # Check if patch is already applied
    execute_process(
        COMMAND git apply --reverse --check ${patch_file_path}
        WORKING_DIRECTORY ${patch_path}
        RESULT_VARIABLE PATCH_ALREADY_APPLIED
        OUTPUT_QUIET ERROR_QUIET
    )

    if(NOT PATCH_ALREADY_APPLIED EQUAL 0)
        # Apply the patch
        execute_process(
            COMMAND git apply ${patch_file_path}
            WORKING_DIRECTORY ${patch_path}
            RESULT_VARIABLE PATCH_APPLIED_RESULT
            OUTPUT_QUIET ERROR_QUIET
        )

        if(NOT PATCH_APPLIED_RESULT EQUAL 0)
            message(FATAL_ERROR "Apply ESP-IDF Patch: ${patch_file} failed (working dir: ${patch_path})")
        else()
            message(STATUS "Apply ESP-IDF Patch: ${patch_file} success (working dir: ${patch_path})")
        endif()
    else()
        message(STATUS "ESP-IDF Patch: ${patch_file} already applied (working dir: ${patch_path})")
    endif()
endfunction()

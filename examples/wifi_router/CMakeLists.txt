cmake_minimum_required(VERSION 3.5)

# set(ENV{ESP_BRIDGE_COMPILATION_WORKAROUND} "ON")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

add_definitions(-w)

add_compile_options("-DIP_PORTMAP_MAX=255")

set(EXTRA_COMPONENT_DIRS "../../components/web_server")

string(REGEX REPLACE ".*/\(.*\)" "\\1" CURDIR ${CMAKE_CURRENT_SOURCE_DIR})
project(${CURDIR})

git_describe(PROJECT_VERSION ${COMPONENT_DIR})
message("Project commit: " ${PROJECT_VERSION})
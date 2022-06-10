# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

add_compile_options(-fdiagnostics-color=always)

if(DEFINED ENV{RMAKER_PATH})
  set(RMAKER_PATH $ENV{RMAKER_PATH})
else()
  set(RMAKER_PATH ${CMAKE_CURRENT_LIST_DIR}/../../../components/esp-rainmaker)
endif(DEFINED ENV{RMAKER_PATH})

set(EXCLUDE_COMPONENTS "coap" "asio")
set(EXTRA_COMPONENT_DIRS  ${RMAKER_PATH}/components
                          ${RMAKER_PATH}/components/esp-insights/components
                          "../common"
                          "../../../components")

if(NOT DEFINED IDF_TARGET)
    set(IDF_TARGET "esp32")
endif()

set(SDKCONFIG_DEFAULTS ${CMAKE_CURRENT_LIST_DIR}/sdkconfig_defaults/sdkconfig.defaults.${IDF_TARGET})

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(led_light)

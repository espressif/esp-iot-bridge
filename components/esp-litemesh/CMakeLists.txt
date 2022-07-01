
set(srcs)
set(require_components "json")

if (CONFIG_LITEMESH_ENABLE)
    list(APPEND srcs "src/esp_litemesh.c")
endif()

idf_component_register (SRCS "${srcs}"
                        INCLUDE_DIRS include
                        REQUIRES ${require_components})

string(CONCAT LIBS "esp_litemesh_" "${IDF_TARGET}")

add_library(${LIBS} STATIC IMPORTED)
set_property(TARGET ${LIBS} PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/lib/lib${LIBS}.a)
target_link_libraries(${COMPONENT_LIB} INTERFACE ${LIBS})
set_property(TARGET ${LIBS} APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${COMPONENT_LIB})
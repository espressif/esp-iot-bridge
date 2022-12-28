set(sub_srcs "driver/src/network_adapter.c")

set(include_dirs "driver/include" "../../examples/spi_and_sdio_host/common/include")

if(CONFIG_ESP_SDIO_HOST_INTERFACE)
    list(APPEND sub_srcs "driver/src/sdio_slave_api.c")
endif()

if(CONFIG_ESP_SPI_HOST_INTERFACE)
    list(APPEND sub_srcs "driver/src/spi_slave_api.c")
endif()

if(CONFIG_BRIDGE_BT_ENABLED)
    list(APPEND sub_srcs "driver/src/slave_bt.c")
endif()

set(priv_includes "${priv_includes}" "${include_dirs}" PARENT_SCOPE)
set(srcs "${srcs}" "${sub_srcs}" PARENT_SCOPE)

# set(requires "bt" "driver")

# idf_component_register(SRCS "${srcs}"
#                        INCLUDE_DIRS "${include_dirs}"
#                        REQUIRES "${requires}")
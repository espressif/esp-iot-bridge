set(srcs "src/network_adapter.c")

set(include_dirs "include" "../../examples/spi_and_sdio_host/common/include")

if(CONFIG_ESP_SDIO_HOST_INTERFACE)
    list(APPEND srcs "src/sdio_slave_api.c")
endif()

if(CONFIG_ESP_SPI_HOST_INTERFACE)
    list(APPEND srcs "src/spi_slave_api.c")
endif()

if(CONFIG_ESP_BRIDGE_BT_ENABLED)
    list(APPEND srcs "src/slave_bt.c")
endif()

set(requires "bt" "driver")

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "${include_dirs}"
                       REQUIRES "${requires}")
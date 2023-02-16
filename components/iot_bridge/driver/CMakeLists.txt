set(sub_srcs "driver/src/network_adapter.c")

set(driver_include "driver/include")

if(CONFIG_ESP_SDIO_HOST_INTERFACE)
    list(APPEND sub_srcs "driver/src/sdio_slave_api.c")
endif()

if(CONFIG_ESP_SPI_HOST_INTERFACE)
    list(APPEND sub_srcs "driver/src/spi_slave_api.c")
endif()

if(CONFIG_BRIDGE_BT_ENABLED)
    list(APPEND sub_srcs "driver/src/slave_bt.c")
endif()

set(include_dirs "${include_dirs}" "${driver_include}" PARENT_SCOPE)
set(srcs "${srcs}" "${sub_srcs}" PARENT_SCOPE)

set(srcs "src/gateway_common.c")
set(requires "")
set(include_dirs "include")
set(priv_includes "priv_inc")

if (CONFIG_GATEWAY_EXTERNAL_NETIF_STATION OR CONFIG_GATEWAY_DATA_FORWARDING_NETIF_SOFTAP)
    list(APPEND srcs "src/gateway_wifi.c")
endif()

if (CONFIG_SET_VENDOR_IE)
    list(APPEND srcs "src/gateway_vendor_ie.c")
endif()

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "${include_dirs}"
                       PRIV_INCLUDE_DIRS "${priv_includes}"
                       REQUIRES "${requires}")

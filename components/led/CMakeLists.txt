set(srcs "led_indicator.c")

set(include_dirs "include")

set(requires "driver")

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "${include_dirs}"
                       REQUIRES "${requires}")
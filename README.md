# Pico SDK Wiznet W5x00 driver

```
if(NOT DEFINED WIZNET_CHIP)
    set(WIZNET_CHIP W5100S)
endif()

if(${WIZNET_CHIP} STREQUAL W5100S)
    add_definitions(-D_WIZCHIP_=W5100S)
    message("WIZNET_CHIP is = ${WIZNET_CHIP}")
elseif(${WIZNET_CHIP} STREQUAL W5500)
    add_definitions(-D_WIZCHIP_=W5500)
    message("WIZNET_CHIP is = ${WIZNET_CHIP}")
else()
    message(FATAL_ERROR "WIZNET_CHIP is unsupported = ${WIZNET_CHIP}")
endif()
```

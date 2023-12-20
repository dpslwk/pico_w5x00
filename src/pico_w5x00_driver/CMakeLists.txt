if (DEFINED ENV{PICO_IOLIBRARY_DRIVER_PATH} AND (NOT PICO_IOLIBRARY_DRIVER_PATH))
    set(PICO_IOLIBRARY_DRIVER_PATH $ENV{PICO_IOLIBRARY_DRIVER_PATH})
    message("Using PICO_IOLIBRARY_DRIVER_PATH from environment ('${PICO_IOLIBRARY_DRIVER_PATH}')")
endif()

set(W5X00_DRIVER_TEST_FILE "Ethernet/wizchip_conf.h")

if (NOT PICO_IOLIBRARY_DRIVER_PATH)
    set(PICO_IOLIBRARY_DRIVER_PATH ${CMAKE_CURRENT_LIST_DIR}/../../lib/ioLibrary_Driver)
    if (PICO_W5X00_SUPPORTED AND NOT EXISTS ${PICO_IOLIBRARY_DRIVER_PATH}/${W5X00_DRIVER_TEST_FILE})
        message(WARNING "ioLibrary_Driver submodule has not been initialized; Wiznet Ethernet support will be unavailable
hint: try 'git submodule update --init' from your SDK directory (${CMAKE_SOURCE_DIR}).")
    endif()
elseif (NOT EXISTS ${PICO_IOLIBRARY_DRIVER_PATH}/${W5X00_DRIVER_TEST_FILE})
    message(WARNING "PICO_IOLIBRARY_DRIVER_PATH specified but content not present.")
endif()

if(NOT DEFINED WIZNET_CHIP)
    set(WIZNET_CHIP W5100S)
endif()

if(${WIZNET_CHIP} STREQUAL W5100S)
    add_definitions(-D_WIZCHIP_=W5100S)
elseif(${WIZNET_CHIP} STREQUAL W5500)
    add_definitions(-D_WIZCHIP_=W5500)
else()
    message(FATAL_ERROR "WIZNET_CHIP is unsupported = ${WIZNET_CHIP}")
endif()

if (EXISTS ${PICO_IOLIBRARY_DRIVER_PATH}/${W5X00_DRIVER_TEST_FILE})
    message("w5x00-driver available at ${PICO_IOLIBRARY_DRIVER_PATH}")

    pico_register_common_scope_var(PICO_IOLIBRARY_DRIVER_PATH)

    # base driver without our bus
    pico_add_library(w5x00_driver NOFLAG)
    target_sources(w5x00_driver INTERFACE
            ${PICO_IOLIBRARY_DRIVER_PATH}/Ethernet/socket.c
            ${PICO_IOLIBRARY_DRIVER_PATH}/Ethernet/wizchip_conf.c
            ${PICO_IOLIBRARY_DRIVER_PATH}/Ethernet/W5100S/w5100s.c
            ${PICO_IOLIBRARY_DRIVER_PATH}/Ethernet/W5500/w5500.c
            )
    target_include_directories(w5x00_driver_headers INTERFACE
            ${PICO_IOLIBRARY_DRIVER_PATH}/Ethernet
            ${PICO_IOLIBRARY_DRIVER_PATH}/Ethernet/W5100S
            ${PICO_IOLIBRARY_DRIVER_PATH}/Ethernet/W5500
            )

    # pico_w5x00_driver adds async_context integration to w5x00_driver
    pico_add_library(pico_w5x00_driver NOFLAG)
    target_sources(pico_w5x00_driver INTERFACE
            w5x00_driver.c
            w5x00_spi.c
            w5x00_lwip.c
            )
    target_include_directories(pico_w5x00_driver_headers INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
    pico_mirrored_target_link_libraries(pico_w5x00_driver INTERFACE
            w5x00_driver
            hardware_gpio
            hardware_irq
            hardware_spi
            hardware_dma
            hardware_exception
            )

    pico_promote_common_scope_vars()
endif()

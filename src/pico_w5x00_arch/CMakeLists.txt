if (TARGET w5x00_driver)
    pico_add_library(pico_w5x00_arch)
    target_sources(pico_w5x00_arch INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/w5x00_arch.c
            ${CMAKE_CURRENT_LIST_DIR}/w5x00_arch_poll.c
            ${CMAKE_CURRENT_LIST_DIR}/w5x00_arch_threadsafe_background.c
            ${CMAKE_CURRENT_LIST_DIR}/w5x00_arch_freertos.c
            )

    target_include_directories(pico_w5x00_arch_headers INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/include)

    pico_mirrored_target_link_libraries(pico_w5x00_arch INTERFACE
            pico_unique_id
            w5x00_driver # ioLibrary
            pico_w5x00_driver # integration with async_context
            )

    if (NOT TARGET pico_lwip)
        message(WARNING "lwIP is not available; Full Wiznet Ethernet support will be unavailable")
    else()
        message("Wiznet Ethernet build support available.")
        pico_add_library(pico_w5x00_arch_poll NOFLAG)
        target_compile_definitions(pico_w5x00_arch_poll_headers INTERFACE
                PICO_W5X00_ARCH_POLL=1
                )
        pico_mirrored_target_link_libraries(pico_w5x00_arch_poll INTERFACE
                pico_w5x00_arch
                pico_async_context_poll)

        pico_add_library(pico_w5x00_arch_lwip_poll NOFLAG)
        pico_mirrored_target_link_libraries(pico_w5x00_arch_lwip_poll INTERFACE
                pico_lwip_nosys
                pico_w5x00_arch_poll)
        target_compile_definitions(pico_w5x00_arch_lwip_poll_headers INTERFACE
                W5X00_LWIP=1
                )

        pico_add_library(pico_w5x00_arch_threadsafe_background NOFLAG)
        pico_mirrored_target_link_libraries(pico_w5x00_arch_threadsafe_background INTERFACE
                pico_w5x00_arch
                pico_async_context_threadsafe_background)
        target_compile_definitions(pico_w5x00_arch_threadsafe_background_headers INTERFACE
                PICO_W5X00_ARCH_THREADSAFE_BACKGROUND=1
                )

        pico_add_library(pico_w5x00_arch_lwip_threadsafe_background NOFLAG)
        pico_mirrored_target_link_libraries(pico_w5x00_arch_lwip_threadsafe_background INTERFACE
                pico_lwip_nosys
                pico_w5x00_arch_threadsafe_background)
        target_compile_definitions(pico_w5x00_arch_lwip_threadsafe_background_headers INTERFACE
                W5X00_LWIP=1
                )

        pico_add_library(pico_w5x00_arch_sys_freertos NOFLAG)
        pico_mirrored_target_link_libraries(pico_w5x00_arch_sys_freertos INTERFACE
                pico_w5x00_arch
                pico_async_context_freertos)
        target_compile_definitions(pico_w5x00_arch_sys_freertos_headers INTERFACE
                PICO_W5X00_ARCH_FREERTOS=1
                )

        pico_add_library(pico_w5x00_arch_lwip_sys_freertos NOFLAG)
        pico_mirrored_target_link_libraries(pico_w5x00_arch_lwip_sys_freertos INTERFACE
                pico_lwip_freertos
                pico_w5x00_arch_sys_freertos)
        target_compile_definitions(pico_w5x00_arch_lwip_sys_freertos_headers INTERFACE
                W5X00_LWIP=1
                LWIP_PROVIDE_ERRNO=1
                # now the default
                #PICO_LWIP_CUSTOM_LOCK_TCPIP_CORE=1 # we want to override the lwip locking mechanism to use our mutex
                )
    endif()

    pico_add_library(pico_w5x00_arch_none NOFLAG)
    pico_mirrored_target_link_libraries(pico_w5x00_arch_none INTERFACE
            pico_w5x00_arch
            pico_async_context_threadsafe_background)
    target_compile_definitions(pico_w5x00_arch_none_headers INTERFACE
            W5X00_LWIP=0
            PICO_W5X00_ARCH_THREADSAFE_BACKGROUND=1 # none still uses threadsafe_background to make gpio use easy
            )
endif()

if (PICO_W5X00_DRIVER_PATH AND EXISTS "${PICO_W5X00_DRIVER_PATH}")
    pico_add_doxygen(${PICO_W5X00_DRIVER_PATH}/src)
endif()

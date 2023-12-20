#ifdef W5X00_CONFIG_FILE
#include W5X00_CONFIG_FILE
#else
#include "w5x00_configport.h"
#endif

#ifndef W5X00_IOCTL_TIMEOUT_US
#define W5X00_IOCTL_TIMEOUT_US (500000)
#endif

#ifndef W5X00_SLEEP_MAX
#define W5X00_SLEEP_MAX (50)
#endif

#ifndef W5X00_LWIP
#define W5X00_LWIP (1)
#endif

#ifndef W5X00_PRINTF
#include <stdio.h>
#define W5X00_PRINTF(...) printf(__VA_ARGS__)
#endif

#ifndef W5X00_VDEBUG
#define W5X00_VDEBUG(...) (void)0
#define W5X00_VERBOSE_DEBUG 0
#endif

#ifndef W5X00_DEBUG
#ifdef NDEBUG
#define W5X00_DEBUG(...) (void)0
#else
#define W5X00_DEBUG(...) W5X00_PRINTF(__VA_ARGS__)
#endif
#endif

#ifndef W5X00_INFO
#define W5X00_INFO(...) W5X00_PRINTF(__VA_ARGS__)
#endif

#ifndef W5X00_WARN
#define W5X00_WARN(...) W5X00_PRINTF("[W5X00] " __VA_ARGS__)
#endif

#ifndef W5X00_FAIL_FAST_CHECK
#define W5X00_FAIL_FAST_CHECK(res) (res)
#endif

#ifndef W5X00_DEFAULT_IP_ADDRESS
#define W5X00_DEFAULT_IP_ADDRESS LWIP_MAKEU32(0, 0, 0, 0)
#endif

#ifndef W5X00_DEFAULT_IP_MASK
#define W5X00_DEFAULT_IP_MASK LWIP_MAKEU32(255, 255, 255, 0)
#endif

#ifndef W5X00_DEFAULT_IP_GATEWAY
#define W5X00_DEFAULT_IP_GATEWAY LWIP_MAKEU32(192, 168, 0, 1)
#endif

#ifndef W5X00_DEFAULT_IP_DNS
#define W5X00_DEFAULT_IP_DNS LWIP_MAKEU32(8, 8, 8, 8)
#endif

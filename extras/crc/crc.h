#ifndef __CRC_H_
#define __CRC_H_

#include <espressif/esp_misc.h> // sdk_os_delay_us
#include "FreeRTOS.h"

uint8_t onewire_crc8(const uint8_t *addr, uint8_t len);

#endif /* End crc.h */

/**
*   
*   @file crc.c
*
*   @brief Implementation for the crc module.
*
*   @details Implements the CRC16-CCITT algorithm.
*
*/

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "crc.h"

uint16_t crc16_ccitt(uint16_t length, const uint8_t* data) {
    uint16_t crc = 0xFFFF;

    if (data == NULL || !length) {
        return 0;
    }

    for (uint16_t i = 0; i < length; i++) {
        crc ^= ((uint16_t) *(data + i)) << 8;

        for (uint8_t j = 0; j < 8; j++) {
        
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            }
            
            else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

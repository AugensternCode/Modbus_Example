#include "modbus_crc.h"

uint16_t modbus_crc16(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xFFFFU;

    while (length-- > 0U) {
        crc ^= *data++;
        for (unsigned int i = 0U; i < 8U; i++) {
            if ((crc & 0x0001U) != 0U) {
                crc = (uint16_t)((crc >> 1U) ^ 0xA001U);
            } else {
                crc >>= 1U;
            }
        }
    }

    return crc;
}

#ifndef MODBUS_CRC_H
#define MODBUS_CRC_H

#include <stdint.h>
#include <stddef.h>

uint16_t modbus_crc16(const uint8_t *data, size_t length);

#endif

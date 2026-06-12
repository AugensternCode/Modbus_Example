#ifndef MODBUS_CRC_H
#define MODBUS_CRC_H

#include "stm32f10x.h"

/* 计算 Modbus RTU 使用的 CRC16，返回值低字节先发送。 */
uint16_t Modbus_CRC16(const uint8_t *data, uint16_t length);

#endif

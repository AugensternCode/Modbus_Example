#ifndef MODBUS_SLAVE_H
#define MODBUS_SLAVE_H

#include "stm32f10x.h"

#define MODBUS_RX_BUFFER_SIZE  256U

extern volatile uint8_t g_modbus_frame_ready;

void Modbus_Init(uint8_t slave_id);
void Modbus_ReceiveByte(uint8_t byte);
void Modbus_Poll(void);

#endif

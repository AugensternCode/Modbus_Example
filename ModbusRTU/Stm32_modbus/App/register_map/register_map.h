#ifndef REGISTER_MAP_H
#define REGISTER_MAP_H

#include "stm32f10x.h"

#define REG_LED_MODE       0U
#define REG_PWM_DUTY       1U
#define REG_HEARTBEAT      2U
#define REG_COUNT          3U

void RegisterMap_Init(void);
uint8_t RegisterMap_CheckRange(uint16_t addr, uint16_t count);
uint16_t RegisterMap_ReadHolding(uint16_t addr);
uint8_t RegisterMap_WriteHolding(uint16_t addr, uint16_t value);
void RegisterMap_Tick1s(void);

#endif

#ifndef BSP_SYS_H
#define BSP_SYS_H

#include "stm32f10x.h"

/* SysTick 提供 1 ms 系统时基，裸机任务调度都依赖该计数。 */
void SysTick_Init(void);
uint32_t GetTick(void);
void Delay_ms(uint32_t delay_ms);

#endif

#ifndef STM32F10X_IT_H
#define STM32F10X_IT_H

#include "stm32f10x.h"

/* Cortex-M3 异常和 USART1 中断入口，由启动文件中的向量表引用。 */
void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);
void USART1_IRQHandler(void);

#endif

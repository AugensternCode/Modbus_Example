#include "stm32f10x_it.h"
#include "modbus_slave.h"

/* 系统毫秒计数由 SysTick 中断维护，主循环只读取。 */
extern __IO uint32_t g_system_tick_ms;

void NMI_Handler(void)
{
    /* 当前工程未使用 NMI，保留空处理函数。 */
}

void HardFault_Handler(void)
{
    /* 严重异常进入死循环，便于调试器停在故障现场。 */
    while (1) {
    }
}

void MemManage_Handler(void)
{
    /* 内存管理异常保持现场。 */
    while (1) {
    }
}

void BusFault_Handler(void)
{
    /* 总线异常保持现场。 */
    while (1) {
    }
}

void UsageFault_Handler(void)
{
    /* 非法指令/除零等用法异常保持现场。 */
    while (1) {
    }
}

void SVC_Handler(void)
{
    /* 裸机工程未使用 SVC。 */
}

void DebugMon_Handler(void)
{
    /* 调试监视异常未启用。 */
}

void PendSV_Handler(void)
{
    /* 未使用 RTOS，因此 PendSV 保持空实现。 */
}

void SysTick_Handler(void)
{
    /* 1 ms 系统时基。 */
    g_system_tick_ms++;
}

void USART1_IRQHandler(void)
{
	if(USART_GetITStatus(USART1,USART_IT_RXNE)!=RESET)
	{
		Modbus_ReceiveByte(USART_ReceiveData(USART1));
	}
	if(USART_GetITStatus(USART1,USART_IT_IDLE)!=RESET)
	{
		volatile uint16_t clear_flag;
		clear_flag=USART1->SR;
		clear_flag=USART1->DR;
		(void)clear_flag;
		g_modbus_frame_ready=1U;
	}
}

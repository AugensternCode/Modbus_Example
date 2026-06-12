#ifndef BSP_RS485_H
#define BSP_RS485_H

#include "stm32f10x.h"

/* 野火 STM32F103VET6 板级 IO：
 * PA9  -> USART1_TX
 * PA10 -> USART1_RX
 * 默认使用自动收发方向控制的 RS485 模块。
 */
#define RS485_USARTx                   USART1
#define RS485_USART_CLK                RCC_APB2Periph_USART1
#define RS485_USART_APBxClkCmd         RCC_APB2PeriphClockCmd
#define RS485_BAUDRATE                 9600U

#define RS485_USART_GPIO_CLK           RCC_APB2Periph_GPIOA
#define RS485_USART_GPIO_APBxClkCmd    RCC_APB2PeriphClockCmd
#define RS485_TX_GPIO_PORT             GPIOA
#define RS485_TX_GPIO_PIN              GPIO_Pin_9
#define RS485_RX_GPIO_PORT             GPIOA
#define RS485_RX_GPIO_PIN              GPIO_Pin_10

/* 配置 USART1 和中断；发送函数按字节阻塞发送一帧数据。 */
void RS485_Config(void);
void RS485_Send_Data(uint8_t *buf, uint8_t len);

#endif

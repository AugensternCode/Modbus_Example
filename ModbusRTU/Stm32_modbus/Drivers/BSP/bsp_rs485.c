#include "bsp_rs485.h"

void RS485_Config(void)
{
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;
    NVIC_InitTypeDef nvic;

    /* 先打开 GPIO 和 USART 时钟，再配置引脚复用。 */
    RS485_USART_GPIO_APBxClkCmd(RS485_USART_GPIO_CLK, ENABLE);
    RS485_USART_APBxClkCmd(RS485_USART_CLK, ENABLE);

    gpio.GPIO_Pin = RS485_TX_GPIO_PIN;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(RS485_TX_GPIO_PORT, &gpio);

    gpio.GPIO_Pin = RS485_RX_GPIO_PIN;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(RS485_RX_GPIO_PORT, &gpio);

    /* Modbus RTU 使用 9600 8N1，收发均开启。 */
    usart.USART_BaudRate = RS485_BAUDRATE;
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(RS485_USARTx, &usart);

    /* USART1 中断负责收字节和帧结束检测。 */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    nvic.NVIC_IRQChannel = USART1_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 1U;
    nvic.NVIC_IRQChannelSubPriority = 1U;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    /* RXNE 收字节，IDLE 判断一帧 RTU 报文已经结束。 */
    USART_ITConfig(RS485_USARTx, USART_IT_RXNE, ENABLE);
    USART_ITConfig(RS485_USARTx, USART_IT_IDLE, ENABLE);
    USART_Cmd(RS485_USARTx, ENABLE);
}

void RS485_Send_Data(uint8_t *buf,uint8_t len)
{
	uint8_t i;
	for(i=0U;i<len;i++)
	{
		USART_SendData(RS485_USARTx,buf[i]);
		while(USART_GetFlagStatus(RS485_USARTx,USART_FLAG_TC)==RESET)
		{
		}
	}
}


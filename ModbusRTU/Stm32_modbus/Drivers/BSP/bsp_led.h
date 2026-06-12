#ifndef BSP_LED_H
#define BSP_LED_H

#include "stm32f10x.h"

/* 野火 STM32F103VET6 板载 RGB LED，低电平点亮。 */
#define LED1_GPIO_PORT     GPIOB
#define LED1_GPIO_CLK      RCC_APB2Periph_GPIOB
#define LED1_GPIO_PIN      GPIO_Pin_5

#define LED2_GPIO_PORT     GPIOB
#define LED2_GPIO_CLK      RCC_APB2Periph_GPIOB
#define LED2_GPIO_PIN      GPIO_Pin_0

#define LED3_GPIO_PORT     GPIOB
#define LED3_GPIO_CLK      RCC_APB2Periph_GPIOB
#define LED3_GPIO_PIN      GPIO_Pin_1

#define LED1_ON            GPIO_ResetBits(LED1_GPIO_PORT, LED1_GPIO_PIN)
#define LED1_OFF           GPIO_SetBits(LED1_GPIO_PORT, LED1_GPIO_PIN)
#define LED2_ON            GPIO_ResetBits(LED2_GPIO_PORT, LED2_GPIO_PIN)
#define LED2_OFF           GPIO_SetBits(LED2_GPIO_PORT, LED2_GPIO_PIN)
#define LED3_ON            GPIO_ResetBits(LED3_GPIO_PORT, LED3_GPIO_PIN)
#define LED3_OFF           GPIO_SetBits(LED3_GPIO_PORT, LED3_GPIO_PIN)

/* 组合三个单色 LED 得到常用状态颜色。 */
#define LED_RED            do { LED1_ON;  LED2_OFF; LED3_OFF; } while (0)
#define LED_GREEN          do { LED1_OFF; LED2_ON;  LED3_OFF; } while (0)
#define LED_BLUE           do { LED1_OFF; LED2_OFF; LED3_ON;  } while (0)
#define LED_YELLOW         do { LED1_ON;  LED2_ON;  LED3_OFF; } while (0)
#define LED_PURPLE         do { LED1_ON;  LED2_OFF; LED3_ON;  } while (0)
#define LED_CYAN           do { LED1_OFF; LED2_ON;  LED3_ON;  } while (0)
#define LED_WHITE          do { LED1_ON;  LED2_ON;  LED3_ON;  } while (0)
#define LED_RGBOFF         do { LED1_OFF; LED2_OFF; LED3_OFF; } while (0)

/* 初始化 LED GPIO，并默认关闭所有颜色。 */
void LED_GPIO_Config(void);

#endif

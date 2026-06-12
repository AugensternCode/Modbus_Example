#ifndef BSP_PWM_H
#define BSP_PWM_H

#include "stm32f10x.h"

/* TIM3_CH1 输出 PWM，用 0..1000 的比较值表示 0..100.0% 占空比。 */
void TIM3_PWM_Init(uint16_t arr, uint16_t psc);
void Motor_SetSpeed(uint16_t duty_val);

#endif

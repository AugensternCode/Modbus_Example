#include "stm32f10x.h"
#include "bsp_sys.h"
#include "bsp_led.h"
#include "bsp_pwm.h"
#include "bsp_rs485.h"
#include "modbus_slave.h"
#include "register_map.h"

int main(void)
{
	uint32_t last_tick;
	SysTick_Init();
	LED_GPIO_Config();
	
	TIM3_PWM_Init(1000U,71U);
	RS485_Config();
	Modbus_Init(1U);
	last_tick=GetTick();
	while(1)
	{
		Modbus_Poll();
		if((GetTick()-last_tick)>=1000U)
		{
			last_tick+=1000U;
			RegisterMap_Tick1s();
		}
	}
}

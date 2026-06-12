#include "bsp_led.h"

void LED_GPIO_Config(void)
{
    GPIO_InitTypeDef gpio;

    /* 三路 LED 共用 GPIOB 时钟，统一配置为推挽输出。 */
    RCC_APB2PeriphClockCmd(LED1_GPIO_CLK | LED2_GPIO_CLK | LED3_GPIO_CLK, ENABLE);

    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;

    gpio.GPIO_Pin = LED1_GPIO_PIN;
    GPIO_Init(LED1_GPIO_PORT, &gpio);

    gpio.GPIO_Pin = LED2_GPIO_PIN;
    GPIO_Init(LED2_GPIO_PORT, &gpio);

    gpio.GPIO_Pin = LED3_GPIO_PIN;
    GPIO_Init(LED3_GPIO_PORT, &gpio);

    /* LED 为低电平点亮，初始化完成后先全部关闭。 */
    LED_RGBOFF;
}

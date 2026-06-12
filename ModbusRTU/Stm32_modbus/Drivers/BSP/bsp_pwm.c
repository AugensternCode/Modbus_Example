#include "bsp_pwm.h"

/**
 * @brief 初始化TIM3的PWM输出功能（通道1，映射到PA6）
 * @param arr 自动重装载值（决定PWM周期）
 * @param psc 预分频器值（决定定时器计数时钟频率）
 * @note   PWM频率计算公式：f_PWM = 72MHz / (psc+1) / (arr+1)
 *         示例：arr=7199, psc=9 时，频率 = 72e6/10/7200 = 1000Hz = 1kHz
 */
void TIM3_PWM_Init(uint16_t arr, uint16_t psc)
{
    GPIO_InitTypeDef gpio;               // GPIO初始化结构体
    TIM_TimeBaseInitTypeDef time_base;   // 定时器时基初始化结构体
    TIM_OCInitTypeDef oc;                // 定时器输出比较初始化结构体

    /* 使能TIM3和GPIOA的时钟 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);  // TIM3挂载在APB1上
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); // GPIOA挂载在APB2上

    /* 配置PA6为复用推挽输出，最大频率50MHz，作为TIM3_CH1的输出引脚 */
    gpio.GPIO_Pin = GPIO_Pin_6;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;    // 复用推挽模式
    gpio.GPIO_Speed = GPIO_Speed_50MHz;  // 输出速度50MHz
    GPIO_Init(GPIOA, &gpio);

    /* 配置TIM3的时基单元（计数器、预分频器、自动重装载等） */
    TIM_TimeBaseStructInit(&time_base);  // 先用默认值填充结构体
    time_base.TIM_Period = arr;          // 自动重装载值，决定PWM周期
    time_base.TIM_Prescaler = psc;       // 预分频器，决定计数时钟频率
    time_base.TIM_ClockDivision = 0U;    // 时钟分割，这里不分频
    time_base.TIM_CounterMode = TIM_CounterMode_Up; // 向上计数模式
    TIM_TimeBaseInit(TIM3, &time_base);  // 初始化TIM3时基

    /* 配置TIM3通道1为PWM1模式，输出极性高有效，初始占空比为0 */
    TIM_OCStructInit(&oc);               // 先用默认值填充结构体
    oc.TIM_OCMode = TIM_OCMode_PWM1;     // PWM模式1：CNT<CCR时输出有效电平
    oc.TIM_OutputState = TIM_OutputState_Enable; // 使能输出比较通道
    oc.TIM_Pulse = 0U;                   // 初始比较值（占空比）为0
    oc.TIM_OCPolarity = TIM_OCPolarity_High; // 有效电平为高电平
    TIM_OC1Init(TIM3, &oc);              // 初始化TIM3通道1

    /* 使能通道1的预装载寄存器（更新事件时影子寄存器才更新） */
    TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
    /* 使能TIM3计数器开始工作 */
    TIM_Cmd(TIM3, ENABLE);
}

/**
 * @brief 设置电机的速度（通过改变PWM占空比）
 * @param duty_val 占空比值，范围0～1000，对应0.0%～100.0%
 * @note  duty_val/1000 即为实际占空比（例如500表示50%）
 *       最终调用TIM_SetCompare1写入TIM3的CCR1寄存器
 */
void Motor_SetSpeed(uint16_t duty_val)
{
    /* 限制占空比输入范围，防止超出自动重装载值（假设arr=1000） */
    if (duty_val > 1000U) 
    {
        duty_val = 1000U;
    }
    /* 设置TIM3通道1的比较值，即改变PWM占空比 */
    TIM_SetCompare1(TIM3, duty_val);
}
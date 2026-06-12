#include "bsp_sys.h"

/* 1 ms 系统节拍，在 SysTick_Handler 中自增。 */
__IO uint32_t g_system_tick_ms = 0U;

void SysTick_Init(void)
{
    /* SystemCoreClock / 1000 让 SysTick 每 1 ms 触发一次。 */
    if (SysTick_Config(SystemCoreClock / 1000U) != 0U) {   //原来72MHz表示1s中断一次，也就是计数72000000中断一次为1s,现在缩小1000倍,就是1ms
        while (1) {
        }
    }
}

uint32_t GetTick(void)
{
    /* 读取当前毫秒计数，主循环用它做非阻塞软件定时。 */
    return g_system_tick_ms;
}

void Delay_ms(uint32_t delay_ms)
{
    uint32_t start = GetTick();  //进入时，计数多少ms

    /* 基于差值判断可自然处理 uint32_t 回绕。 */
    while ((GetTick() - start) < delay_ms) {  //如果没达到这个计算差值，也就是ms数，就陷入循环。
    }
}

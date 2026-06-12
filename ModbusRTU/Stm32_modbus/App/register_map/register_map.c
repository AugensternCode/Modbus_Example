#include "register_map.h"
#include "bsp_led.h"
#include "bsp_pwm.h"

static uint16_t s_holding_regs[REG_COUNT];

static void RegisterMap_ApplyLed(uint16_t mode);
static void RegisterMap_ApplyPwm(uint16_t duty);

void RegisterMap_Init(void)
{
    s_holding_regs[REG_LED_MODE] = 0U;
    s_holding_regs[REG_PWM_DUTY] = 0U;
    s_holding_regs[REG_HEARTBEAT] = 0U;

    RegisterMap_ApplyLed(s_holding_regs[REG_LED_MODE]);
    RegisterMap_ApplyPwm(s_holding_regs[REG_PWM_DUTY]);
}

uint8_t RegisterMap_CheckRange(uint16_t addr, uint16_t count)
{
    if (count == 0U) {
        return 0U;
    }

    if (addr >= REG_COUNT) {
        return 0U;
    }

    if (count > (uint16_t)(REG_COUNT - addr)) {
        return 0U;
    }

    return 1U;
}

uint16_t RegisterMap_ReadHolding(uint16_t addr)
{
    if (addr >= REG_COUNT) {
        return 0U;
    }

    return s_holding_regs[addr];
}

uint8_t RegisterMap_WriteHolding(uint16_t addr, uint16_t value)
{
    if (addr >= REG_COUNT) {
        return 0U;
    }

    if ((addr == REG_LED_MODE) && (value > 7U)) {
        return 0U;
    }

    if ((addr == REG_PWM_DUTY) && (value > 1000U)) {
        return 0U;
    }

    s_holding_regs[addr] = value;

    if (addr == REG_LED_MODE) {
        RegisterMap_ApplyLed(value);
    } else if (addr == REG_PWM_DUTY) {
        RegisterMap_ApplyPwm(value);
    } else {
    }

    return 1U;
}

void RegisterMap_Tick1s(void)
{
    s_holding_regs[REG_HEARTBEAT]++;
}

static void RegisterMap_ApplyLed(uint16_t mode)
{
    switch (mode) {
    case 1U:
        LED_RED;
        break;
    case 2U:
        LED_GREEN;
        break;
    case 3U:
        LED_BLUE;
        break;
    case 4U:
        LED_YELLOW;
        break;
    case 5U:
        LED_PURPLE;
        break;
    case 6U:
        LED_CYAN;
        break;
    case 7U:
        LED_WHITE;
        break;
    default:
        LED_RGBOFF;
        break;
    }
}

static void RegisterMap_ApplyPwm(uint16_t duty)
{
    Motor_SetSpeed(duty);
}

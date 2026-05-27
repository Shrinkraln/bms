#include "dac8552.h"
#include "bsp.h"
#include "stm32g4xx_hal.h"

/* 软件 SPI 位拼 (按原理图 + 你最新 CubeMX：PA1/PA4 已设为 GPIO_Output):
 *   DIN = PA1, SCLK = PA4, SYNC#(CS) = PB10 (DAC_CS_PIN, 见 bsp.h)
 * DAC8552 在 SCLK 下降沿采样数据, SYNC# 上升沿把数据送入 DAC。 */
#define DAC_DIN_PORT    GPIOA
#define DAC_DIN_PIN     GPIO_PIN_1
#define DAC_SCLK_PORT   GPIOA
#define DAC_SCLK_PIN    GPIO_PIN_4

static inline void cs_low(void)   { HAL_GPIO_WritePin(DAC_CS_PORT,   DAC_CS_PIN,   GPIO_PIN_RESET); }
static inline void cs_high(void)  { HAL_GPIO_WritePin(DAC_CS_PORT,   DAC_CS_PIN,   GPIO_PIN_SET);   }
static inline void sclk_h(void)   { HAL_GPIO_WritePin(DAC_SCLK_PORT, DAC_SCLK_PIN, GPIO_PIN_SET);   }
static inline void sclk_l(void)   { HAL_GPIO_WritePin(DAC_SCLK_PORT, DAC_SCLK_PIN, GPIO_PIN_RESET); }
static inline void din(uint8_t b) { HAL_GPIO_WritePin(DAC_DIN_PORT,  DAC_DIN_PIN, b ? GPIO_PIN_SET : GPIO_PIN_RESET); }
static inline void dly(void)      { for (volatile int i = 0; i < 4; ++i) __NOP(); }

static bool send24(uint8_t ctrl, uint16_t data)
{
    uint32_t word = ((uint32_t)ctrl << 16) | data;   /* 24-bit, MSB 先 */
    sclk_h();
    cs_low();
    dly();
    for (int i = 23; i >= 0; --i) {
        din((word >> i) & 1u);
        sclk_h(); dly();
        sclk_l(); dly();        /* 下降沿采样当前位 */
    }
    cs_high();                  /* SYNC 上升沿: 输出更新 */
    sclk_h();
    return true;
}

bool dac8552_init(void)
{
    cs_high();
    /* 上电默认 0 */
    return dac8552_set_both(0, 0);
}

bool dac8552_set_a(uint16_t code)      { return send24(0x10 | 0x04, code); } // load A
bool dac8552_set_b(uint16_t code)      { return send24(0x20 | 0x04, code); } // load B
bool dac8552_set_both(uint16_t a, uint16_t b)
{
    if (!send24(0x10, a)) return false;        // buffer A，不更新
    if (!send24(0x24, b)) return false;        // buffer B + 同步更新 AB
    return true;
}

bool dac8552_set_voltage(uint8_t ch, float volts)
{
    if (volts < 0.0f) volts = 0.0f;
    if (volts > DAC_VREF_V) volts = DAC_VREF_V;
    uint32_t code = (uint32_t)((volts / DAC_VREF_V) * (float)DAC_FULL_SCALE + 0.5f);
    if (code > DAC_FULL_SCALE) code = DAC_FULL_SCALE;
    return (ch == DAC_CH_A) ? dac8552_set_a((uint16_t)code)
                            : dac8552_set_b((uint16_t)code);
}

bool dac8552_set_current(uint8_t ch, float current_mA)
{
    if (current_mA < 0.0f) current_mA = 0.0f;
    /* V_dac = I[A] × Rsns × 环路增益 */
    float volts = (current_mA * 0.001f) * DAC_RSNS_OHM * DAC_CC_LOOP_GAIN;
    return dac8552_set_voltage(ch, volts);
}

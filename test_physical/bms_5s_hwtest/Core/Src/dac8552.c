#include "dac8552.h"
#include "bsp.h"
#include "stm32g4xx_hal.h"

extern SPI_HandleTypeDef hspi3;   // CubeMX 生成：PA4=SCLK, PA1=MOSI(DIN)，CS=PB10 软件

static inline void cs_low(void)  { HAL_GPIO_WritePin(DAC_CS_PORT, DAC_CS_PIN, GPIO_PIN_RESET); }
static inline void cs_high(void) { HAL_GPIO_WritePin(DAC_CS_PORT, DAC_CS_PIN, GPIO_PIN_SET); }

static bool send24(uint8_t ctrl, uint16_t data)
{
    uint8_t tx[3];
    tx[0] = ctrl;
    tx[1] = (uint8_t)(data >> 8);
    tx[2] = (uint8_t)(data & 0xFF);

    cs_low();
    HAL_StatusTypeDef st = HAL_SPI_Transmit(&hspi3, tx, 3, 20);
    cs_high();
    return st == HAL_OK;
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

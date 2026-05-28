/**
 * xpt2046.c — XPT2046 触摸 SPI2 驱动
 *
 * 帧: 8-bit control + 16-bit response, 共 3 字节
 *   Control byte: S=1, A2:A0 = 通道(X=101, Y=001), MODE=0(12bit),
 *                 SER/DFR=0(差分), PD1:PD0=11(常开+ref on)
 *   常用: 0xD0 读 X, 0x90 读 Y
 *   返回的 16-bit: 高字节高 7 位 + 低字节高 5 位 = 12-bit
 */
#include "xpt2046.h"
#include "bsp.h"
#include "stm32g4xx_hal.h"
#include <string.h>

extern SPI_HandleTypeDef hspi2;   /* CubeMX 生成: SPI2 PB13/14/15 */

#define TOUCH_CS_PORT   GPIOA
#define TOUCH_CS_PIN    GPIO_PIN_0
#define TOUCH_IRQ_PORT  GPIOA
#define TOUCH_IRQ_PIN   GPIO_PIN_8

#define XPT_CMD_X       0xD0
#define XPT_CMD_Y       0x90

static inline void cs_l(void) { HAL_GPIO_WritePin(TOUCH_CS_PORT, TOUCH_CS_PIN, GPIO_PIN_RESET); }
static inline void cs_h(void) { HAL_GPIO_WritePin(TOUCH_CS_PORT, TOUCH_CS_PIN, GPIO_PIN_SET);   }

void xpt_init(void)
{
    cs_h();
}

bool xpt_pressed(void)
{
    return HAL_GPIO_ReadPin(TOUCH_IRQ_PORT, TOUCH_IRQ_PIN) == GPIO_PIN_RESET;
}

/* 单次 12-bit 读 (内部) */
static uint16_t xpt_raw_once(uint8_t cmd)
{
    uint8_t tx[3] = { cmd, 0x00, 0x00 };
    uint8_t rx[3] = { 0 };
    cs_l();
    HAL_SPI_TransmitReceive(&hspi2, tx, rx, 3, 20);
    cs_h();
    /* rx[0] 是发 cmd 时同步收到的, 丢弃。
     * rx[1] 高字节(7 valid bits), rx[2] 低字节(5 valid bits) */
    return (uint16_t)(((rx[1] << 5) | (rx[2] >> 3)) & 0x0FFF);
}

/* 中位数滤波: 取 5 次, 排序取中间值 */
static uint16_t xpt_raw_median(uint8_t cmd)
{
    uint16_t s[5];
    for (int i = 0; i < 5; ++i) s[i] = xpt_raw_once(cmd);
    /* 简单选择排序 (5 元素) */
    for (int i = 0; i < 4; ++i)
        for (int j = i + 1; j < 5; ++j)
            if (s[j] < s[i]) { uint16_t t = s[i]; s[i] = s[j]; s[j] = t; }
    return s[2];
}

static uint16_t map_axis(uint16_t raw, uint16_t raw_min, uint16_t raw_max,
                         uint16_t screen, uint8_t invert)
{
    if (raw < raw_min) raw = raw_min;
    if (raw > raw_max) raw = raw_max;
    uint32_t pos = ((uint32_t)(raw - raw_min) * screen) / (raw_max - raw_min);
    if (invert) pos = (screen - 1U) - pos;
    if (pos >= screen) pos = screen - 1U;
    return (uint16_t)pos;
}

bool xpt_read_xy(uint16_t *x, uint16_t *y)
{
    uint16_t rx = xpt_raw_median(XPT_CMD_X);
    uint16_t ry = xpt_raw_median(XPT_CMD_Y);

    /* 边界过滤: 笔抬起时读到的 raw 容易超界, 直接拒掉 */
    if (rx < 50 || rx > 4000 || ry < 50 || ry > 4000) return false;

#if XPT_SWAP_XY
    { uint16_t t = rx; rx = ry; ry = t; }
#endif

    *x = map_axis(rx, XPT_RAW_X_MIN, XPT_RAW_X_MAX, XPT_SCREEN_W, XPT_INVERT_X);
    *y = map_axis(ry, XPT_RAW_Y_MIN, XPT_RAW_Y_MAX, XPT_SCREEN_H, XPT_INVERT_Y);
    return true;
}

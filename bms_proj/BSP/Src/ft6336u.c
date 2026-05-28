/**
 * ft6336u.c — FocalTech FT6336U 电容触摸驱动 (I²C, 0x38)
 *
 * 通信顺序:
 *   1) [可选] RST 拉低 5ms, 拉高, 等 ~300ms
 *   2) 读 0xA3 取 CHIP_ID 校验
 *   3) 运行时: 轮询 INT 拉低 → I²C 一次读 0x02 起 5 字节, 解析点数 + X/Y
 */
#include "ft6336u.h"
#include "i2c_bus.h"
#include "stm32g4xx_hal.h"

bool ft6336u_init(void)
{
#if FT6_USE_RST
    HAL_GPIO_WritePin(FT6_RST_PORT, FT6_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(5);
    HAL_GPIO_WritePin(FT6_RST_PORT, FT6_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(300);     /* FT6336U 上电启动 ~300ms */
#endif

    /* 探测: 先 I2C IsReady, 再读 CHIP_ID */
    if (!i2c_probe(FT6_I2C_ADDR7)) return false;
    uint8_t chip_id = 0;
    if (!i2c_read_reg8(FT6_I2C_ADDR7, FT6_REG_CHIP_ID, &chip_id)) return false;
    /* FT6336U 的 CHIP_ID 常见为 0x64; 不强校验 (变种较多), 能读到非 0/非 FF 即认为在线 */
    if (chip_id == 0x00 || chip_id == 0xFF) return false;

    /* 进 Normal 模式 (0x00) */
    (void)i2c_write_reg8(FT6_I2C_ADDR7, FT6_REG_DEVICE_MODE, 0x00);
    return true;
}

bool ft6336u_pressed(void)
{
    return HAL_GPIO_ReadPin(FT6_INT_PORT, FT6_INT_PIN) == GPIO_PIN_RESET;
}

bool ft6336u_read(uint16_t *x, uint16_t *y)
{
    /* 从 0x02 起读 5 字节: TD_STATUS, T1_XH, T1_XL, T1_YH, T1_YL */
    uint8_t b[5];
    if (!i2c_read_buf(FT6_I2C_ADDR7, FT6_REG_TD_STATUS, b, 5)) return false;

    uint8_t n = b[0] & 0x0F;
    if (n == 0 || n > 5) return false;

    uint16_t rx = (uint16_t)(((b[1] & 0x0F) << 8) | b[2]);
    uint16_t ry = (uint16_t)(((b[3] & 0x0F) << 8) | b[4]);

    if (x) *x = rx;
    if (y) *y = ry;
    return true;
}

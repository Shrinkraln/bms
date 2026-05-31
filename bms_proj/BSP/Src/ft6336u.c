/**
 * ft6336u.c — FocalTech FT6336U 电容触摸驱动
 *
 * 物理接线: SCL=PB13, SDA=PB15, INT=PA8, RST=PA0
 * 使用软件 I²C (sw_i2c_touch), 不走硬件 I2C1 (PA15/PB7 是传感器总线)。
 *
 * 通信顺序:
 *   1) RST 拉低 5ms, 拉高, 等 ~300ms (PA0)
 *   2) 读 0xA3 取 CHIP_ID 校验
 *   3) 运行时: 轮询 INT 拉低 → 软件 I²C 一次读 0x02 起 5 字节, 解析点数 + X/Y
 */
#include "ft6336u.h"
#include "sw_i2c_touch.h"
#include "stm32g4xx_hal.h"
#include <stdio.h>

/* 触摸复位引脚: PA0 (转接板标注 RST, CubeMX 标注 TOUCH_CS) */
#define CTP_RST_PORT  GPIOA
#define CTP_RST_PIN   GPIO_PIN_0

bool ft6336u_init(void)
{
    swi2c_touch_init();

    /* 硬复位 FT6336U (PA0): 拉低 5ms, 拉高, 等 300ms */
    HAL_GPIO_WritePin(CTP_RST_PORT, CTP_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(5);
    HAL_GPIO_WritePin(CTP_RST_PORT, CTP_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(300);

    /* 探测: 读 CHIP_ID */
    uint8_t chip_id = 0;
    if (!swi2c_touch_read(FT6_I2C_ADDR7, FT6_REG_CHIP_ID, &chip_id, 1)) {
        printf("[FT6336U] I2C NACK (addr=0x%02X)\r\n", FT6_I2C_ADDR7);
        return false;
    }
    printf("[FT6336U] CHIP_ID=0x%02X\r\n", chip_id);
    if (chip_id == 0x00 || chip_id == 0xFF) return false;

    /* 进 Normal 模式 */
    uint8_t mode = 0x00;
    (void)swi2c_touch_write(FT6_I2C_ADDR7, FT6_REG_DEVICE_MODE, &mode, 1);
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
    if (!swi2c_touch_read(FT6_I2C_ADDR7, FT6_REG_TD_STATUS, b, 5)) return false;

    uint8_t n = b[0] & 0x0F;
    if (n == 0 || n > 5) return false;

    uint16_t rx = (uint16_t)(((b[1] & 0x0F) << 8) | b[2]);
    uint16_t ry = (uint16_t)(((b[3] & 0x0F) << 8) | b[4]);

    if (x) *x = rx;
    if (y) *y = ry;
    return true;
}

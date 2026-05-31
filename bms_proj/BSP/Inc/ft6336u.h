/**
 * ft6336u.h  — FocalTech FT6336U 电容触摸 (I²C, 0x38)
 *
 * 总线: hi2c1 (与 BQ/INA/TMP 共享, 地址 0x38 不冲突)
 * INT : 触屏拉低 (默认 PA8)
 * RST : 默认与 LCD_RST 共用 PB2 (lcd_init 会复位它)
 *
 * 板子接线不一样时, 改下面 4 个宏即可。
 */
#ifndef __FT6336U_H__
#define __FT6336U_H__

#include <stdint.h>
#include <stdbool.h>

/* ===== 引脚 (按实际转接板接线) ===== */
#define FT6_I2C_ADDR7   0x38

#define FT6_INT_PORT    GPIOA
#define FT6_INT_PIN     GPIO_PIN_8      /* 触摸中断, 按下拉低 */
/* RST = PA0 (独立引脚, 不与 LCD_RST 共享)
 * I²C = PB13(SCL) / PB15(SDA), 通过 sw_i2c_touch 软件驱动 */

/* ===== 寄存器 (节选, 见 FT6X36_寄存器地址.pdf) ===== */
#define FT6_REG_DEVICE_MODE   0x00
#define FT6_REG_GEST_ID       0x01
#define FT6_REG_TD_STATUS     0x02      /* 低 4 位 = 触点数 (0-5) */
#define FT6_REG_TOUCH1_XH     0x03      /* bits[3:0] = X[11:8] */
#define FT6_REG_TOUCH1_XL     0x04
#define FT6_REG_TOUCH1_YH     0x05      /* bits[3:0] = Y[11:8] */
#define FT6_REG_TOUCH1_YL     0x06
#define FT6_REG_CHIP_ID       0xA3
#define FT6_REG_FW_VERSION    0xA6

/* 初始化: 可选硬复位 + 探测 (读 CHIP_ID)。成功返回 true。 */
bool ft6336u_init(void);

/* 是否当前有触屏 (轮询 INT 引脚, 拉低 = 按下) */
bool ft6336u_pressed(void);

/* 读 1 点坐标 (12-bit 原始, 已经是屏分辨率范围, 不用像电阻屏标定)。
 * 返回 true = 成功读到有效触点; false = 没触点或读失败。 */
bool ft6336u_read(uint16_t *x, uint16_t *y);

#endif

#ifndef __SW_I2C_TOUCH_H__
#define __SW_I2C_TOUCH_H__

/* ============================================================
 *  软件 I²C — 专用于 FT6336U 触摸 (PB13=SCL, PB15=SDA)
 *
 *  不用硬件 I2C1 (PA15/PB7) 因为触摸线物理上接在 PB13/PB15。
 *  bsp_init() 已把这两个引脚配成开漏 + 内部上拉。
 *  速率 ~100kHz (HAL_Delay 不够精细, 用 NOP 循环凑)。
 * ============================================================ */
#include <stdint.h>
#include <stdbool.h>

/* 初始化 (bsp_init 里已经配好 GPIO, 这里只是拉高总线空闲) */
void swi2c_touch_init(void);

/* 标准 I²C 读写 (7-bit 地址, 自动 <<1) */
bool swi2c_touch_write(uint8_t addr7, uint8_t reg, const uint8_t *data, uint8_t len);
bool swi2c_touch_read (uint8_t addr7, uint8_t reg, uint8_t *data, uint8_t len);

#endif

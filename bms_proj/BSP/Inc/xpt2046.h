/**
 * xpt2046.h  — 触摸控制器 XPT2046 (SPI2)
 *
 * 引脚:  SCK=PB13, MISO=PB14, MOSI=PB15, CS=PA0, IRQ=PA8 (按下=低)
 * SPI2 由 CubeMX 配为 8-bit Master, 速率建议 ≤ 2.5 MHz (XPT2046 上限)。
 *
 * 标定: 实测 4 角原始 ADC 后改这两个宏；首版给一组常见默认值，上板再细调。
 */
#ifndef __XPT2046_H__
#define __XPT2046_H__

#include <stdint.h>
#include <stdbool.h>

/* ===== 上板可调的标定常数 (raw 12-bit ADC ↔ 屏幕坐标) ===== */
#define XPT_RAW_X_MIN   300U
#define XPT_RAW_X_MAX   3850U
#define XPT_RAW_Y_MIN   300U
#define XPT_RAW_Y_MAX   3850U
#define XPT_SCREEN_W    240U
#define XPT_SCREEN_H    240U
/* 方向取反 (按你板贴屏方式调): 1 = 翻转该轴 */
#define XPT_INVERT_X    0
#define XPT_INVERT_Y    0
#define XPT_SWAP_XY     0   /* 横屏可能要交换 X/Y */

/* 初始化: 拉高 CS (deselect). 必须在 SPI2 已初始化之后调用。 */
void xpt_init(void);

/* 是否按下 (PA8 IRQ 拉低) */
bool xpt_pressed(void);

/* 读一次坐标 (映射到 0..XPT_SCREEN_W/H-1)；
 * 仅在 xpt_pressed() 为 true 时调用才有意义；
 * 内部对 X/Y 各取 N 次中位数滤波。
 * 返回 true 表示读取成功。 */
bool xpt_read_xy(uint16_t *x, uint16_t *y);

#endif

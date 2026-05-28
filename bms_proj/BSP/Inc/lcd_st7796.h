/**
 * lcd_st7796.h  — Sitronix ST7796S 3.5" 320×480 TFT (4-line SPI)
 *
 * 引脚: SCK=PA5, MOSI=PA7 (SPI1), CS=PB1, DC=PB0, RST=PB2
 * 接口: 4-line SPI Master, 8-bit, CPOL=0 CPHA=0
 * 模组: CL35BC1017-40A (鸿讯电子)
 *
 * 自检颜色块 + lcd_set_window + lcd_write_pixels 给 LVGL flush 用。
 */
#ifndef __LCD_ST7796_H__
#define __LCD_ST7796_H__

#include <stdint.h>
#include <stdbool.h>

/* 横屏 480×320 (软件旋转, MADCTL 见 .c). 改竖屏: W=320 H=480 + 改 MADCTL=0x48 */
#define LCD_W 480
#define LCD_H 320

#define LCD_COLOR_RED    0xF800
#define LCD_COLOR_GREEN  0x07E0
#define LCD_COLOR_BLUE   0x001F
#define LCD_COLOR_WHITE  0xFFFF
#define LCD_COLOR_BLACK  0x0000

bool lcd_init(void);
void lcd_fill(uint16_t rgb565);

/* 暴露给 LVGL flush 用 (RGB565 高字节先发) */
void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void lcd_write_pixels(const uint8_t *buf, uint32_t n_bytes);

#endif

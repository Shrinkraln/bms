#ifndef __LCD_ST7789_H__
#define __LCD_ST7789_H__

#include <stdint.h>
#include <stdbool.h>

/* 仅作硬件验证：复位、初始化、清屏纯色。
 * 验证 SPI1 + PB0(DC) + PB1(CS) + PB2(RST) 通路是否正常。
 * 目视判断：屏幕能按顺序变红→绿→蓝→白 即 PASS。
 */
bool lcd_init(void);
void lcd_fill(uint16_t rgb565);

/* 暴露给 LVGL flush 用: 设窗口 + 大块写像素 (RGB565, 每像素高字节先发) */
void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void lcd_write_pixels(const uint8_t *buf, uint32_t n_bytes);

#define LCD_COLOR_RED   0xF800
#define LCD_COLOR_GREEN 0x07E0
#define LCD_COLOR_BLUE  0x001F
#define LCD_COLOR_WHITE 0xFFFF
#define LCD_COLOR_BLACK 0x0000

/* 默认按 240x240 配置，可按实际屏调整 */
#define LCD_W 240
#define LCD_H 240

#endif

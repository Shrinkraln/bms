/**
 * lvgl_port.c  — LVGL 9.x 显示/tick 适配 (ST7789 240×240, SPI1)
 */
#include "lvgl_port.h"
#include "lcd_st7789.h"
#include "xpt2046.h"
#include "lvgl.h"
#include "stm32g4xx_hal.h"

/* 部分缓冲: 240 列 × 40 行 × 2B = 19200 字节 (8% SRAM).
 * 单缓冲 + PARTIAL 模式，渲染→flush→渲染下一段，足够流畅。 */
#define LVGL_BUF_LINES   40
static uint8_t s_disp_buf[LCD_W * LVGL_BUF_LINES * 2];

/* tick: 返回 ms; LVGL 内部据此推进定时器 */
static uint32_t lvgl_tick_cb(void)
{
    return HAL_GetTick();
}

/* flush: 把 LVGL 渲好的 RGB565 像素块送到 ST7789 */
static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    const uint32_t w  = (uint32_t)(area->x2 - area->x1 + 1);
    const uint32_t h  = (uint32_t)(area->y2 - area->y1 + 1);
    const uint32_t px = w * h;

    /* LVGL 默认 RGB565 在内存里是小端 (low,high), ST7789 期望每像素高字节先发 */
    lv_draw_sw_rgb565_swap(px_map, px);

    lcd_set_window((uint16_t)area->x1, (uint16_t)area->y1,
                   (uint16_t)area->x2, (uint16_t)area->y2);
    lcd_write_pixels(px_map, px * 2);

    lv_display_flush_ready(disp);
}

/* ===== 触摸输入设备 read_cb ===== */
static void lvgl_touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    static uint16_t last_x = 0, last_y = 0;
    if (xpt_pressed()) {
        uint16_t x, y;
        if (xpt_read_xy(&x, &y)) { last_x = x; last_y = y; }
        data->point.x = (int32_t)last_x;
        data->point.y = (int32_t)last_y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->point.x = (int32_t)last_x;
        data->point.y = (int32_t)last_y;
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void lvgl_port_init(void)
{
    lv_init();
    lv_tick_set_cb(lvgl_tick_cb);

    /* 显示 */
    lv_display_t *disp = lv_display_create(LCD_W, LCD_H);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(disp, s_disp_buf, NULL,
                           sizeof(s_disp_buf),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, lvgl_flush_cb);

    /* 触摸 */
    xpt_init();
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, lvgl_touch_read_cb);
}

void lvgl_port_handler(void)
{
    lv_timer_handler();
}

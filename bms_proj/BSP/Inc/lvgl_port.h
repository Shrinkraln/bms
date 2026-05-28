/**
 * lvgl_port.h  — LVGL 与 ST7789 / SysTick 的硬件适配
 */
#ifndef __LVGL_PORT_H__
#define __LVGL_PORT_H__

/* 初始化: lv_init + 注册 tick + 创建 display + 注册 flush_cb。
 * 调用前必须先 lcd_init()。 */
void lvgl_port_init(void);

/* 主循环里周期调用 (建议每 ~5ms 一次): 推进 LVGL 时间轮 / 渲染。 */
void lvgl_port_handler(void);

#endif

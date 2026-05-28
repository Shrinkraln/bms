/**
 * bms_ui.h  — BMS 化成 UI (LVGL 9, ST7789 240×240, 多标签页)
 *
 *  3 个标签页:
 *    [Dashboard] 状态指示 + 大字总压/电流 + 5 节竖条 + 容量/温度/报警
 *    [Chart]     电压/电流 60 秒滚动曲线 (lv_chart, 双 Y 轴)
 *    [Setup]     START / STOP 大按钮 (触摸触发 fm_start / fm_stop)
 *
 *  KEY1: 切到下一标签。
 *  触摸: 切标签 (顶部 tab 条) + 按 Setup 页的按钮。
 */
#ifndef __BMS_UI_H__
#define __BMS_UI_H__

#include "formation.h"

/* 创建 UI; 保存 fm 指针供按钮回调使用 (start/stop)。
 * 必须在 lvgl_port_init() 之后调用。 */
void bms_ui_init(fm_ctx_t *fm);

/* 刷新数值 + 推一个新样本到曲线; 建议每秒一次。 */
void bms_ui_update(const fm_ctx_t *ctx);

/* KEY1 调用: 切到下一个标签页 (循环)。 */
void bms_ui_next_tab(void);

#endif

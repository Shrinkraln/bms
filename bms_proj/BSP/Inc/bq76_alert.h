#ifndef __BQ76_ALERT_H__
#define __BQ76_ALERT_H__

/* ============================================================
 *  BQ76920 ALERT 中断处理
 *  PB3 上升沿 (CubeMX 已配 EXTI3 + 下拉) -> bq76_alert_isr() 置 flag
 *  主循环里 bq76_alert_poll() 读 SYS_STAT:
 *    - 任何保护位 (OV/UV/SCD/OCD/DEVICE_XREADY/OVRD_ALERT) -> 立刻关 DAC
 *      并把 fm 推到 ERROR
 *    - 只有 CC_READY (CC_EN=1 时每 250ms 一次) -> 安静地清掉, FSM 继续跑
 *  最后写 SYS_STAT (W1C) 把处理过的位清掉, BQ 解除 ALERT 输出
 * ============================================================ */
#include <stdint.h>
#include <stdbool.h>
#include "formation.h"

/* 保存 fm 指针, 让 poll 能改 FSM 状态 */
void bq76_alert_init(fm_ctx_t *fm);

/* 在 main loop 周期调 (建议每轮); 内部按 flag 短路, 0 中断时几乎零开销 */
void bq76_alert_poll(void);

/* 由 HAL_GPIO_EXTI_Callback 在 GPIO_Pin == BQ76_ALTER_IN_Pin 时调用。
 * 仅置 volatile flag, 不做 I2C / printf。 */
void bq76_alert_isr(void);

/* 调试: 累计中断次数 (CC_READY 也算) */
uint32_t bq76_alert_count(void);

#endif

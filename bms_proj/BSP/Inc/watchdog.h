#ifndef __WATCHDOG_H__
#define __WATCHDOG_H__

/* ============================================================
 *  IWDG 独立看门狗
 *  时钟源: LSI ~32kHz (G4 内部低速 RC, 误差 ±10%)
 *  超时 : ~1.6s (足够覆盖 LCD 一次满屏刷新 + bq76 全量读取 + LVGL tick)
 *
 *  CubeMX 没勾 IWDG 也能用: 本模块在用户层直接调 HAL_IWDG_Init,
 *  不依赖 CubeMX 生成的 MX_IWDG_Init / hiwdg。
 *  hal_conf.h 中已 #define HAL_IWDG_MODULE_ENABLED。
 *
 *  典型用法:
 *      watchdog_init();          // 在所有自检/驱动 init 之后
 *      while (1) {
 *          ... 主循环工作 ...
 *          watchdog_feed();      // 末尾喂一次
 *      }
 *  注意:
 *      IWDG 一旦启动, 软件无法关闭 (只能复位)。
 *      bq76920 自检里的 HAL_Delay(250) 远小于 1.6s, 不会误触发。
 * ============================================================ */
#include <stdbool.h>

bool watchdog_init(void);   /* 启 IWDG; 失败返回 false (一般不会发生) */
void watchdog_feed(void);   /* 喂狗 (主循环每轮调一次) */

#endif

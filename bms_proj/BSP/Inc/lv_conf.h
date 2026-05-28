/**
 * lv_conf.h  — BMS-5S 上 LVGL 9.x 的极简配置
 *
 * 设计原则: 只覆盖必要项, 其余项依赖 LVGL 自带 lv_conf_internal.h 的默认值。
 * 硬件: STM32G474RET (170MHz, 128KB SRAM), ST7789 240×240 SPI1, 软件 SPI 不用。
 * 预算: LV_MEM ≈ 32KB, 显示缓冲 ≈ 19KB, 余 ~70KB 给应用。
 */

#ifndef LV_CONF_H
#define LV_CONF_H

/* ================== 颜色 / 内存 ================== */
#define LV_COLOR_DEPTH              16          /* RGB565 */

#define LV_USE_STDLIB_MALLOC        LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_STRING        LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF       LV_STDLIB_BUILTIN
#define LV_MEM_SIZE                 (32U * 1024U)   /* LVGL 内部 heap */

/* ================== OS / 时钟 ================== */
#define LV_USE_OS                   LV_OS_NONE
/* tick: 运行时用 lv_tick_set_cb(HAL_GetTick) 注册, 无需 SysTick 钩子 */

/* ================== 显示刷新 ================== */
#define LV_DEF_REFR_PERIOD          30          /* ~33 fps 上限 */
#define LV_DPI_DEF                  130         /* 2.0" 小屏典型 */

/* ================== 绘制 ================== */
#define LV_USE_DRAW_SW              1
#define LV_DRAW_SW_COMPLEX          1           /* 阴影/渐变 */

/* ================== 字体 ================== */
#define LV_FONT_MONTSERRAT_12       1
#define LV_FONT_MONTSERRAT_14       1
#define LV_FONT_MONTSERRAT_20       1
#define LV_FONT_MONTSERRAT_28       1           /* 总压/电流大字 */
#define LV_FONT_DEFAULT             &lv_font_montserrat_14

/* ================== 主题 (深色) ================== */
#define LV_USE_THEME_DEFAULT        1
#define LV_THEME_DEFAULT_DARK       1
#define LV_THEME_DEFAULT_GROW       1

/* ================== 关闭无用功能 (省 Flash) ================== */
#define LV_USE_LOG                  0
#define LV_USE_ASSERT_NULL          1   /* 留个最基本的, 出问题好定位 */
#define LV_USE_ASSERT_MALLOC        0
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0

#define LV_USE_PERF_MONITOR         0
#define LV_USE_MEM_MONITOR          0
#define LV_USE_REFR_DEBUG           0

/* 不需要的图像格式 / 库 */
#define LV_USE_BMP                  0
#define LV_USE_PNG                  0
#define LV_USE_LODEPNG              0
#define LV_USE_LIBPNG               0
#define LV_USE_SJPG                 0
#define LV_USE_LIBJPEG_TURBO        0
#define LV_USE_GIF                  0
#define LV_USE_QRCODE               0
#define LV_USE_BARCODE              0
#define LV_USE_FREETYPE             0
#define LV_USE_TINY_TTF             0
#define LV_USE_FFMPEG               0
#define LV_USE_RLOTTIE              0
#define LV_USE_THORVG_INTERNAL      0
#define LV_USE_THORVG_EXTERNAL      0
#define LV_USE_VECTOR_GRAPHIC       0

/* 文件系统 (本工程不用) */
#define LV_USE_FS_STDIO             0
#define LV_USE_FS_POSIX             0
#define LV_USE_FS_WIN32             0
#define LV_USE_FS_FATFS             0
#define LV_USE_FS_MEMFS             0
#define LV_USE_FS_LITTLEFS          0
#define LV_USE_FS_ARDUINO_ESP_LITTLEFS 0
#define LV_USE_FS_ARDUINO_SD        0

/* Demo / examples 全关 */
#define LV_USE_DEMO_WIDGETS         0
#define LV_USE_DEMO_KEYPAD_AND_ENCODER 0
#define LV_USE_DEMO_BENCHMARK       0
#define LV_USE_DEMO_RENDER          0
#define LV_USE_DEMO_STRESS          0
#define LV_USE_DEMO_MUSIC           0
#define LV_USE_DEMO_FLEX_LAYOUT     0
#define LV_USE_DEMO_MULTILANG       0
#define LV_USE_DEMO_TRANSFORM       0
#define LV_USE_DEMO_SCROLL          0

/* SDL / GLFW / X11 桌面后端 (我们是裸机) */
#define LV_USE_SDL                  0
#define LV_USE_X11                  0
#define LV_USE_WAYLAND              0
#define LV_USE_LINUX_FBDEV          0
#define LV_USE_LINUX_DRM            0

/* 触摸/键盘 input device: 我们用 KEY1 走 app 自己处理, 先不用内置 */

#endif /* LV_CONF_H */

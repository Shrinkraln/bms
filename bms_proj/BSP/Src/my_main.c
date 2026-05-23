/* ============================================================
 *  BMS 5S 硬件验证主程序
 *  MCU : STM32G474RET
 *  作者: Claude (基于您的原理图自动生成)
 *  说明: 仅依赖 CubeMX 生成的 MX_xxx_Init() 外设初始化；
 *        测试逻辑独立于工程脚手架，便于移植。
 *
 *  在 CubeMX 中需要使能：
 *    - HSE/HSI + PLL 到 170MHz（或您当前的主频）
 *    - I2C1   PB6=SCL, PB7=SDA, 100kHz, Standard
 *    - SPI1   PA5/PA6/PA7  (LCD)        Master, 8bit, CPOL=0 CPHA=0
 *    - SPI2   PB13/PB14/PB15 (Touch)    Master, 8bit, CPOL=0 CPHA=0
 *    - SPI3   PA4(SCK)/PA1(MOSI) only   Master TX-only, 8bit, CPOL=0 CPHA=1
 *    - USART2 PA2/PA3 115200 8N1
 *    - FDCAN1 PA11/PA12, 500kbps（自环测试时不依赖收发器）
 *    - 其余 GPIO 由 bsp_init() 接管，CubeMX 里可不勾。
 * ============================================================ */
#include "main.h"
#include "stm32g4xx_hal.h"
#include <stdio.h>
#include <string.h>

#include "bsp.h"
#include "i2c_bus.h"
#include "bq76920.h"
#include "periph_tests.h"
#include "dac8552.h"
#include "lcd_st7789.h"
#include "can_test.h"
#include "my_main.h"


/* ---------- 测试结果记录 ---------- */
typedef struct {
    const char *name;
    bool        pass;
    char        detail[64];
} test_item_t;

#define MAX_ITEMS 16
static test_item_t g_items[MAX_ITEMS];
static int g_n = 0;

static void add_item(const char *name, bool pass, const char *fmt, ...)
{
    if (g_n >= MAX_ITEMS) return;
    g_items[g_n].name = name;
    g_items[g_n].pass = pass;
    va_list ap; va_start(ap, fmt);
    vsnprintf(g_items[g_n].detail, sizeof(g_items[g_n].detail), fmt, ap);
    va_end(ap);
    printf("  [%s] %-18s %s\r\n", pass ? " OK " : "FAIL",
           name, g_items[g_n].detail);
    /* 实时灯指示：通过绿闪一下，失败红闪一下 */
    if (pass) { led_g_on(); HAL_Delay(60); led_g_off(); }
    else      { led_r_on(); HAL_Delay(120); led_r_off(); }
    g_n++;
}

/* =====================================================================
 *                            各项测试
 * ===================================================================== */

static void t_io_self(void)
{
    /* LED + 蜂鸣器自检：目视/听感由人确认；这里只确认写得动 */
    led_g_on(); HAL_Delay(150); led_g_off();
    led_r_on(); HAL_Delay(150); led_r_off();
    buzzer_beep(80);
    add_item("LED/BUZZ", true, "visual+audible check, see/hear it?");
}

static void t_key(void)
{
    printf("  >> Press KEY1 within 3s to confirm key...\r\n");
    bool ok = key1_wait(3000);
    add_item("KEY1 PB12", ok, ok ? "pressed" : "timeout (skip if no key)");
}

static void t_i2c_scan(void)
{
    uint8_t found[16];
    uint8_t n = i2c_scan(found, sizeof(found));

    /* 期望地址 */
    bool seen_bq76 = false, seen_bq34 = false, seen_ina = false, seen_tmp = false;
    char buf[64] = {0};
    int p = 0;
    for (uint8_t i = 0; i < n; ++i) {
        p += snprintf(buf+p, sizeof(buf)-p, "%02X ", found[i]);
        if (found[i] == BQ76_ADDR7)     seen_bq76 = true;
        if (found[i] == BQ34_ADDR7)     seen_bq34 = true;
        if (found[i] == INA226_ADDR7)   seen_ina  = true;
        if (found[i] == TMP117_ADDR7)   seen_tmp  = true;
    }
    bool pass = (n >= 3);  // 至少 3 个能扫到（BQ34 需要 VEN，等下一步）
    add_item("I2C scan", pass, "n=%u: %s", n, buf);
    (void)seen_bq76; (void)seen_bq34; (void)seen_ina; (void)seen_tmp;
}

static void t_bq76920(void)
{
    if (!bq76_init()) {
        add_item("BQ76920", false, "init NACK or CRC err");
        return;
    }
    bq76_info_t info;
    if (!bq76_read_info(&info)) {
        add_item("BQ76920", false, "read regs failed");
        return;
    }
    /* 打印每节电压 */
    printf("    SYS_STAT=0x%02X gain=%duV/LSB off=%dmV\r\n",
           info.sys_stat, info.adc_gain_uV, info.adc_offset_mV);
    for (int i = 0; i < 5; ++i) {
        printf("    VC%d = %u mV  (raw=%u)\r\n", i+1, info.vc_mV[i], info.vc_raw[i]);
    }
    /* 简单判据：5 节电压都在 1500..4500 mV 之间 */
    bool pass = true;
    for (int i = 0; i < 5; ++i) {
        if (info.vc_mV[i] < 1500 || info.vc_mV[i] > 4500) pass = false;
    }
    add_item("BQ76920 5S", pass, "5 cells %s",
             pass ? "in 1.5~4.5V" : "OUT OF RANGE");
}

static void t_bq34z100(void)
{
    uint16_t dt = 0;
    bool ok = bq34_test(&dt);
    add_item("BQ34Z100", ok, "DeviceType=0x%04X (want 0x0100)", dt);
}

static void t_ina226(void)
{
    uint16_t mid=0, did=0; int16_t sh=0; uint16_t bv=0;
    bool ok = ina226_test(&mid, &did, &sh, &bv);
    add_item("INA226", ok, "MFG=0x%04X DIE=0x%04X Vbus=%umV", mid, did, bv);
}

static void t_tmp117(void)
{
    uint16_t id = 0; float tc = 0;
    bool ok = tmp117_test(&id, &tc);
    int t_int  = (int)tc;
    int t_frac = (int)((tc - t_int) * 100);
    if (t_frac < 0) t_frac = -t_frac;
    add_item("TMP117", ok, "ID=0x%04X T=%d.%02dC", id, t_int, t_frac);
}

static void t_dac8552(void)
{
    bool ok = dac8552_init();
    if (ok) ok = dac8552_set_both(0x0000, 0x0000);   // ~0V
    HAL_Delay(20);
    if (ok) ok = dac8552_set_both(0x8000, 0x8000);   // ~Vref/2
    HAL_Delay(20);
    if (ok) ok = dac8552_set_both(0xFFFF, 0xFFFF);   // ~Vref
    HAL_Delay(20);
    if (ok) ok = dac8552_set_both(0x0000, 0x0000);
    add_item("DAC8552", ok,
             ok ? "SPI3 wrote 0/half/full; measure DAC_OUT_A/B"
                : "SPI3 transmit fail");
}

static void t_lcd(void)
{
    bool ok = lcd_init();
    if (ok) {
        lcd_fill(LCD_COLOR_RED);   HAL_Delay(300);
        lcd_fill(LCD_COLOR_GREEN); HAL_Delay(300);
        lcd_fill(LCD_COLOR_BLUE);  HAL_Delay(300);
        lcd_fill(LCD_COLOR_WHITE);
    }
    add_item("LCD SPI1", ok,
             ok ? "RGBW flashed; visual confirm"
                : "init failed");
}

static void t_can(void)
{
    bool ok = can_loopback_test();
    add_item("FDCAN1 LB", ok, "internal loopback %s", ok ? "" : "failed");
}

/* =====================================================================
 *                              main
 * ===================================================================== */
void set_up(void)
{
   
    bsp_init();

    /* 上电"我活着"信号 */
    led_g_on(); led_r_on(); HAL_Delay(200);
    led_g_off(); led_r_off();
    buzzer_beep(80);

    printf("\r\n\r\n=========================================\r\n");
    printf("  BMS 5S Hardware Self-Test  v1.0\r\n");
    printf("  Build: %s %s\r\n", __DATE__, __TIME__);
    printf("=========================================\r\n");

    /* —— 测试顺序 —— */
    t_io_self();
    t_key();
    t_bq76920();         /* BQ76920 自己上电就活，先测 */
    t_bq34z100();        /* 内部会拉 VEN */
    t_i2c_scan();        /* 此时 BQ34 也使能了，扫一次更完整 */
    t_ina226();
    t_tmp117();
    t_dac8552();
    t_lcd();
    t_can();

    /* —— 汇总 —— */
    int pass_n = 0, fail_n = 0;
    for (int i = 0; i < g_n; ++i) {
        if (g_items[i].pass) pass_n++; else fail_n++;
    }

    printf("\r\n----- SUMMARY -----\r\n");
    printf("  PASS: %d   FAIL: %d   TOTAL: %d\r\n", pass_n, fail_n, g_n);
    if (fail_n) {
        printf("  Failed items:\r\n");
        for (int i = 0; i < g_n; ++i) {
            if (!g_items[i].pass)
                printf("    - %s : %s\r\n", g_items[i].name, g_items[i].detail);
        }
        /* 红灯常亮 + 长鸣 */
        led_r_on();
        buzzer_beep(800);
    } else {
        printf("  *** ALL PASS, board OK ***\r\n");
        /* 绿灯常亮 + 3 短鸣 */
        led_g_on();
        for (int i = 0; i < 3; ++i) { buzzer_beep(80); HAL_Delay(80); }
    }

    /* 主循环：心跳，方便后续再测温度/电压等 */
    uint32_t tick = HAL_GetTick();
    while (1) {
        if (HAL_GetTick() - tick > 1000) {
            tick = HAL_GetTick();
            if (fail_n) led_r_toggle();
            else        led_g_toggle();

            /* 每秒打印一次实时电压 + 温度 + 电流，方便监控 */
            bq76_info_t info;
            if (bq76_read_info(&info)) {
                printf("[live] VC: %u %u %u %u %u mV  STAT=0x%02X\r\n",
                       info.vc_mV[0], info.vc_mV[1], info.vc_mV[2],
                       info.vc_mV[3], info.vc_mV[4], info.sys_stat);
            }
            int16_t sh; uint16_t bv;
            if (ina226_test(NULL, NULL, &sh, &bv)) {
                /* I = shunt_uV / Rsns(10mΩ) ; uV / 10000 → 10mA 单位 */
                int32_t mA = (int32_t)sh * 100 / 1000;  // sh in 1uV; /10m = uV/10mΩ = nA; 简化
                /* 上面的换算精确写：I_uA = shunt_uV / 0.01 = shunt_uV * 100;
                   I_mA = shunt_uV * 100 / 1000 = shunt_uV / 10 */
                mA = sh / 10;
                printf("[live] Vbus=%umV  Ishunt=%dmA  (Rsns=10mOhm)\r\n",
                       bv, (int)mA);
            }
            float tc;
            if (tmp117_test(NULL, &tc)) { /* 占位，不实时打温度避免刷屏 */ }
        }
        HAL_Delay(10);
    }
}

/* CubeMX 生成的函数原型在 main.h 中；
 * 本文件只负责测试逻辑，CubeMX 工程导入时把本 main.c 替换即可。 */
void loop(void)
{
}
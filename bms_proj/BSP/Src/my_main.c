/* ============================================================
 *  BMS 5S 化成测试主程序
 *  MCU : STM32G474RET6
 *
 *  外设依赖 (CubeMX):
 *    I2C1  PA15/PB7   传感器总线 (BQ76920/BQ34/INA226/TMP117)
 *    SPI1  PA5/PA7    LCD ST7796 (Mode3, 10.6MHz, NSSP off, IRQ off)
 *    USART2 PA2/PA3   调试串口 (CH340G)
 *    FDCAN1 PA11/PA12 CAN 总线 (TJA1042)
 *    GPIO  见 bsp.h
 *
 *  触摸 FT6336U: 软件 I²C (PB13=SCL, PB15=SDA), INT=PA8, RST=PA0
 *  DAC8552: 软件 SPI (PA1=DIN, PA4=SCLK, PB10=CS)
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
#include "lcd_st7796.h"
#include "can_test.h"
#include "formation.h"
#include "can_report.h"
#include "can_cmd.h"
#include "bq76_alert.h"
#include "watchdog.h"
#include "lvgl_port.h"
#include "bms_ui.h"
#include "my_main.h"


/* ---------- 自检结果记录 ---------- */
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
    if (pass) { led_g_on(); HAL_Delay(60); led_g_off(); }
    else      { led_r_on(); HAL_Delay(120); led_r_off(); }
    g_n++;
}

/* =====================================================================
 *                            各项自检
 * ===================================================================== */

static void t_io_self(void)
{
    led_g_on(); HAL_Delay(150); led_g_off();
    led_r_on(); HAL_Delay(150); led_r_off();
    buzzer_beep(80);
    add_item("LED/BUZZ", true, "visual+audible check");
}

static void t_i2c_scan(void)
{
    uint8_t found[16];
    uint8_t n = i2c_scan(found, sizeof(found));
    char buf[64] = {0};
    int p = 0;
    for (uint8_t i = 0; i < n; ++i)
        p += snprintf(buf+p, sizeof(buf)-p, "%02X ", found[i]);
    add_item("I2C scan", (n >= 1), "n=%u: %s", n, buf);
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
    printf("    SYS_STAT=0x%02X gain=%duV/LSB off=%dmV\r\n",
           info.sys_stat, info.adc_gain_uV, info.adc_offset_mV);
    for (int i = 0; i < 5; ++i)
        printf("    VC%d = %u mV  (raw=%u)\r\n", i+1, info.vc_mV[i], info.vc_raw[i]);
    bool pass = true;
    for (int i = 0; i < 5; ++i)
        if (info.vc_mV[i] < 1500 || info.vc_mV[i] > 4500) pass = false;
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
    if (ok) ok = dac8552_set_both(0x0000, 0x0000);
    HAL_Delay(20);
    if (ok) ok = dac8552_set_both(0x8000, 0x8000);
    HAL_Delay(20);
    if (ok) ok = dac8552_set_both(0x0000, 0x0000);
    add_item("DAC8552", ok, ok ? "wrote 0/half/0" : "transmit fail");
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
    add_item("LCD SPI1", ok, ok ? "RGBW flashed" : "init failed");
}

static void t_can(void)
{
    bool ok = can_loopback_test();
    add_item("FDCAN1 LB", ok, "loopback %s", ok ? "OK" : "failed");
}

/* =====================================================================
 *                              main
 * ===================================================================== */
void set_up(void)
{
    bsp_init();

    /* 上电信号 */
    led_g_on(); led_r_on(); HAL_Delay(200);
    led_g_off(); led_r_off();
    buzzer_beep(80);

    printf("\r\n=========================================\r\n");
    printf("  BMS 5S Self-Test  v2.0\r\n");
    printf("  Build: %s %s\r\n", __DATE__, __TIME__);
    printf("=========================================\r\n");

    /* —— 自检 (不阻塞: 跳过 key 等待) —— */
    t_io_self();
    t_bq76920();
    t_bq34z100();
    t_i2c_scan();
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
        led_r_on();
        buzzer_beep(800);
    } else {
        printf("  *** ALL PASS ***\r\n");
        led_g_on();
        for (int i = 0; i < 3; ++i) { buzzer_beep(80); HAL_Delay(80); }
    }

    /* === 化成主循环 === */
    fm_ctx_t fm;
    fm_init(&fm, NULL);
    ina226_init();
    if (can_report_init())  printf("[CAN] report+cmd enabled\r\n");
    else                    printf("[CAN] init failed\r\n");
    can_cmd_init(&fm);
    bq76_alert_init(&fm);

    lvgl_port_init();
    bms_ui_init(&fm);
    bms_ui_update(&fm);

    printf("\r\n[Formation] KEY1=next tab, touch START to begin.\r\n");

    watchdog_init();

    uint32_t tick = HAL_GetTick();
    while (1) {
        if (key1_pressed()) {
            bms_ui_next_tab();
            uint32_t t0 = HAL_GetTick();
            while (key1_pressed() && (HAL_GetTick() - t0) < 1000U) HAL_Delay(10);
        }

        can_cmd_poll();
        bq76_alert_poll();
        fm_tick(&fm);
        lvgl_port_handler();

        if (HAL_GetTick() - tick > 1000) {
            tick = HAL_GetTick();
            if (fail_n || fm.state == FM_ERROR) led_r_toggle();
            else                                led_g_toggle();

            printf("[%s] pack=%lumV I=%ldmA T=%dC chg=%d dsg=%d | C:%u %u %u %u %u\r\n",
                   fm_state_name(fm.state), (unsigned long)fm.pack_mV,
                   (long)fm.current_mA, fm.temp_C,
                   (int)fm.charged_mAh, (int)fm.discharged_mAh,
                   fm.cell_mV[0], fm.cell_mV[1], fm.cell_mV[2],
                   fm.cell_mV[3], fm.cell_mV[4]);

            can_report_send_status(&fm);
            can_report_recover_if_busoff();
            bms_ui_update(&fm);
        }
        watchdog_feed();
        HAL_Delay(10);
    }
}

void loop(void)
{
}

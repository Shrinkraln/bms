#include "bq76_alert.h"
#include "bq76920.h"
#include "dac8552.h"
#include "stm32g4xx_hal.h"
#include <stdio.h>

/* ---- 状态 ---- */
static fm_ctx_t      *s_fm = NULL;
static volatile uint32_t s_alert_count = 0;   /* 累计 ISR 次数 */
static volatile uint32_t s_pending     = 0;   /* 自上次 poll 以来新增的 ISR 数 */
static uint32_t          s_last_log_tick = 0; /* CC_READY 日志限流 */
static uint32_t          s_cc_ready_n    = 0; /* CC_READY 累计 (限流打印用) */

void bq76_alert_init(fm_ctx_t *fm)
{
    s_fm = fm;
    s_alert_count = 0;
    s_pending     = 0;
    s_cc_ready_n  = 0;
    s_last_log_tick = HAL_GetTick();

    /* 进入主循环前主动清一次 SYS_STAT, 避免 BQ76 上电状态位拉住 ALERT
     * (bq76_init 已写过 0xFF, 这里是双保险) */
    (void)bq76_clear_status(0xFF);
}

void bq76_alert_isr(void)
{
    /* ISR 上下文: 只能做最小操作。I2C/printf 留给 poll。 */
    s_alert_count++;
    s_pending++;
}

uint32_t bq76_alert_count(void) { return s_alert_count; }

/* SYS_STAT 故障位 -> fm_error_t 优先级 (越靠前越严重)
 *  DEVICE_XREADY (芯片本身故障) > OV > UV > SCD/OCD/OVRD_ALERT
 * 多个同时出现时取最严重的作为 FSM 的 latch 错误码。 */
static fm_error_t decode_fault(uint8_t stat)
{
    if (stat & BQ76_STAT_DEVICE_XREADY) return FM_ERR_SENSOR;
    if (stat & BQ76_STAT_OV)            return FM_ERR_CELL_OV;
    if (stat & BQ76_STAT_UV)            return FM_ERR_CELL_UV;
    if (stat & BQ76_STAT_SCD)           return FM_ERR_SENSOR;  /* 用 SENSOR 兜底 (FM_ERR_ 里没 SCD/OCD) */
    if (stat & BQ76_STAT_OCD)           return FM_ERR_SENSOR;
    if (stat & BQ76_STAT_OVRD_ALERT)    return FM_ERR_SENSOR;
    return FM_ERR_NONE;
}

void bq76_alert_poll(void)
{
    if (s_pending == 0) return;

    /* 原子拿走 pending (ISR 端只会 ++, 这里整段清 0 不会丢失,
     * 最坏新到一个 ISR 在我们清 0 后又把 pending 变 1, 下轮再处理一次, 幂等) */
    __disable_irq();
    uint32_t n = s_pending;
    s_pending = 0;
    __enable_irq();

    uint8_t stat = 0;
    if (!bq76_read_status(&stat)) {
        /* I2C 死了 = 没法确认 BQ 状态 -> 安全侧: 关输出 + 报 SENSOR */
        dac8552_set_both(0, 0);
        printf("[BQ ALERT] I2C read SYS_STAT failed (pending=%lu) - safe shutdown\r\n",
               (unsigned long)n);
        fm_external_fail(s_fm, FM_ERR_SENSOR);
        return;
    }

    uint8_t faults = stat & BQ76_STAT_FAULT_MASK;

    if (faults) {
        /* 关键路径: 先关 DAC, 后做其他事 */
        dac8552_set_both(0, 0);

        fm_error_t e = decode_fault(faults);
        printf("[BQ ALERT] FAULT SYS_STAT=0x%02X%s%s%s%s%s%s (n=%lu) -> FM_ERROR\r\n",
               stat,
               (faults & BQ76_STAT_DEVICE_XREADY) ? " DEVICE_XREADY" : "",
               (faults & BQ76_STAT_OVRD_ALERT)    ? " OVRD"          : "",
               (faults & BQ76_STAT_OV)            ? " OV"            : "",
               (faults & BQ76_STAT_UV)            ? " UV"            : "",
               (faults & BQ76_STAT_SCD)           ? " SCD"           : "",
               (faults & BQ76_STAT_OCD)           ? " OCD"           : "",
               (unsigned long)n);
        fm_external_fail(s_fm, e);
    } else if (stat & BQ76_STAT_CC_READY) {
        /* CC_EN=1 时, BQ 每 250ms 拉 ALERT 一次报新的 CC 样本可读;
         * 我们用 INA226 测电流, 不消费 CC, 这里只静默清 bit 不打扰 FSM。
         * 每 5s 打印一次累计计数, 方便观察 ALERT 通路是否活着。 */
        s_cc_ready_n++;
        uint32_t now = HAL_GetTick();
        if ((now - s_last_log_tick) >= 5000U) {
            printf("[BQ ALERT] CC_READY x%lu in last 5s (total %lu)\r\n",
                   (unsigned long)s_cc_ready_n, (unsigned long)s_alert_count);
            s_cc_ready_n   = 0;
            s_last_log_tick = now;
        }
    }
    /* 清掉所有我们处理过的位; 写 1 清 0, 写 0 不影响, 所以传 stat 本身 */
    if (stat) (void)bq76_clear_status(stat);
}

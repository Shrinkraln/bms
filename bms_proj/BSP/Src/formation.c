#include "formation.h"
#include "bq76920.h"
#include "periph_tests.h"
#include "dac8552.h"
#include "bsp.h"
#include "stm32g4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* DAC 通道与充放电方向映射 (上板核对): B=充电, A=放电 */
#define DAC_CH_CHG   DAC_CH_B
#define DAC_CH_DSG   DAC_CH_A

/* CV 阶段电压环积分增益 (mA/(mV·s))，与 tick 周期无关；需上板整定 */
#define FM_CV_KI  5.0f


/* ------------------------------------------------------------ */
void fm_default_config(fm_config_t *cfg)
{
    cfg->charge_current_mA     = 1250.0f;   /* 0.5C @ 2.5Ah */
    cfg->charge_pack_max_mV    = 21000;     /* 5 × 4.20V */
    cfg->cv_cutoff_current_mA  = 125.0f;    /* 0.05C */
    cfg->discharge_current_mA  = 1250.0f;
    cfg->discharge_pack_min_mV = 12500;     /* 5 × 2.50V */
    cfg->rest_ms               = 30u * 60u * 1000u;  /* 30 min */
    cfg->ov_cell_mV            = 4250;
    cfg->uv_cell_mV            = 2400;
    cfg->max_temp_C            = 60;
    cfg->cc_charge_timeout_ms    = 3u * 3600u * 1000u;  /* 3h 安全上限 */
    cfg->cv_charge_timeout_ms    = 2u * 3600u * 1000u;
    cfg->cc_discharge_timeout_ms = 3u * 3600u * 1000u;
}

const char *fm_state_name(fm_state_t s)
{
    switch (s) {
        case FM_IDLE:           return "IDLE";
        case FM_CC_CHARGE:      return "CC_CHARGE";
        case FM_CV_CHARGE:      return "CV_CHARGE";
        case FM_REST_AFTER_CHG: return "REST_AFTER_CHG";
        case FM_CC_DISCHARGE:   return "CC_DISCHARGE";
        case FM_REST_AFTER_DSG: return "REST_AFTER_DSG";
        case FM_COMPLETE:       return "COMPLETE";
        case FM_ERROR:          return "ERROR";
        default:                return "?";
    }
}

/* ------------------------------------------------------------ */
static void fm_output_off(void)
{
    dac8552_set_both(0, 0);
}

static void fm_goto(fm_ctx_t *ctx, fm_state_t s)
{
    ctx->state = s;
    ctx->state_start_tick = HAL_GetTick();
    printf("[FM] -> %s\r\n", fm_state_name(s));
}

static void fm_fail(fm_ctx_t *ctx, fm_error_t e)
{
    fm_output_off();
    ctx->error = e;
    fm_goto(ctx, FM_ERROR);
    printf("[FM] ERROR code=%d (cells %u %u %u %u %u mV, T=%dC)\r\n",
           (int)e, ctx->cell_mV[0], ctx->cell_mV[1], ctx->cell_mV[2],
           ctx->cell_mV[3], ctx->cell_mV[4], ctx->temp_C);
}

/* 读取一轮测量，写入 ctx。电压读失败视为致命，返回 false。
 * 温度变化慢, 节流到 1Hz 读 (tmp117_test 内有 20ms HAL_Delay, 每 tick 读太亏)。 */
static bool fm_read_sensors(fm_ctx_t *ctx)
{
    bq76_info_t info;
    if (!bq76_read_info(&info)) return false;

    uint32_t pack = 0;
    for (int i = 0; i < 5; ++i) {
        ctx->cell_mV[i] = info.vc_mV[i];
        pack += info.vc_mV[i];
    }
    ctx->pack_mV = pack;

    ctx->current_mA = ina226_current_mA();   /* 失败返回 0，非致命 */

    /* 温度 1Hz 节流: 比电压电流慢得多, 不必每 tick 都读 */
    static uint32_t last_temp_tick = 0;
    uint32_t now = HAL_GetTick();
    if ((now - last_temp_tick) >= 1000U) {
        float t;
        if (tmp117_test(NULL, &t)) ctx->temp_C = (int16_t)t;
        last_temp_tick = now;
    }
    return true;
}

static bool fm_any_cell_ge(const fm_ctx_t *ctx, uint16_t mv)
{
    for (int i = 0; i < 5; ++i) if (ctx->cell_mV[i] >= mv) return true;
    return false;
}

static bool fm_any_cell_le(const fm_ctx_t *ctx, uint16_t mv)
{
    for (int i = 0; i < 5; ++i) if (ctx->cell_mV[i] <= mv) return true;
    return false;
}

/* ------------------------------------------------------------ */
void fm_init(fm_ctx_t *ctx, const fm_config_t *cfg)
{
    memset(ctx, 0, sizeof(*ctx));
    if (cfg) ctx->cfg = *cfg;
    else     fm_default_config(&ctx->cfg);
    ctx->state = FM_IDLE;
    ctx->error = FM_ERR_NONE;
    fm_output_off();
}

void fm_start(fm_ctx_t *ctx)
{
    if (ctx->state != FM_IDLE && ctx->state != FM_COMPLETE && ctx->state != FM_ERROR)
        return;
    ctx->error = FM_ERR_NONE;
    ctx->charged_mAh = 0.0f;
    ctx->discharged_mAh = 0.0f;
    ctx->cv_setpoint_mA = ctx->cfg.charge_current_mA;
    ctx->last_tick = HAL_GetTick();
    fm_output_off();
    fm_goto(ctx, FM_CC_CHARGE);
}

void fm_stop(fm_ctx_t *ctx)
{
    fm_output_off();
    fm_goto(ctx, FM_IDLE);
}

/* 外部强制失败 (BQ76920 ALERT 等硬件中断走的路径)。
 * 关 DAC -> 设错误码 -> 进 ERROR 态。打印含 SYS_STAT 等当时上下文。
 * 已经是 ERROR 不再覆盖 state，但若新错误码更"重要"则覆写 (保留首因为主)。 */
void fm_external_fail(fm_ctx_t *ctx, fm_error_t e)
{
    if (!ctx) return;
    fm_output_off();
    if (ctx->state != FM_ERROR) {
        ctx->error = e;
        fm_goto(ctx, FM_ERROR);
    } else if (ctx->error == FM_ERR_NONE) {
        ctx->error = e;
    }
    /* 若已是 ERROR 且 error 已 latch, 保留首因, 只是再次确认输出已关 */
}

void fm_tick(fm_ctx_t *ctx)
{
    uint32_t now = HAL_GetTick();
    uint32_t dt_ms = now - ctx->last_tick;
    ctx->last_tick = now;

    /* ERROR：保持关断，不再读取/控制 */
    if (ctx->state == FM_ERROR) return;

    /* 其余状态都刷新测量值（供监控 / CAN 上报） */
    bool sensors_ok = fm_read_sensors(ctx);

    /* 非活动状态 (IDLE / COMPLETE)：只监控，不控制电流 */
    if (ctx->state == FM_IDLE || ctx->state == FM_COMPLETE) return;

    if (!sensors_ok) { fm_fail(ctx, FM_ERR_SENSOR); return; }

    /* 过温保护 (任何活动状态) */
    if (ctx->temp_C >= ctx->cfg.max_temp_C) { fm_fail(ctx, FM_ERR_OVER_TEMP); return; }

    /* 容量积分: mAh += |I| × dt / 3.6e6 */
    float dmAh = fabsf((float)ctx->current_mA) * (float)dt_ms / 3600000.0f;
    uint32_t elapsed = now - ctx->state_start_tick;

    switch (ctx->state) {

    case FM_CC_CHARGE:
        dac8552_set_current(DAC_CH_DSG, 0.0f);
        dac8552_set_current(DAC_CH_CHG, ctx->cfg.charge_current_mA);
        ctx->charged_mAh += dmAh;
        if (fm_any_cell_ge(ctx, ctx->cfg.ov_cell_mV)) { fm_fail(ctx, FM_ERR_CELL_OV); break; }
        if (elapsed >= ctx->cfg.cc_charge_timeout_ms) { fm_fail(ctx, FM_ERR_TIMEOUT); break; }
        if (ctx->pack_mV >= ctx->cfg.charge_pack_max_mV) {
            ctx->cv_setpoint_mA = ctx->cfg.charge_current_mA;
            fm_goto(ctx, FM_CV_CHARGE);
        }
        break;

    case FM_CV_CHARGE: {
        /* 积分控制维持总压 = charge_pack_max_mV，电流随之收敛 (增益按 dt 缩放，与 tick 频率无关) */
        float err_mV = (float)ctx->cfg.charge_pack_max_mV - (float)ctx->pack_mV;
        ctx->cv_setpoint_mA += err_mV * FM_CV_KI * ((float)dt_ms / 1000.0f);
        if (ctx->cv_setpoint_mA < 0.0f) ctx->cv_setpoint_mA = 0.0f;
        if (ctx->cv_setpoint_mA > ctx->cfg.charge_current_mA)
            ctx->cv_setpoint_mA = ctx->cfg.charge_current_mA;
        dac8552_set_current(DAC_CH_DSG, 0.0f);
        dac8552_set_current(DAC_CH_CHG, ctx->cv_setpoint_mA);
        ctx->charged_mAh += dmAh;
        if (fm_any_cell_ge(ctx, ctx->cfg.ov_cell_mV)) { fm_fail(ctx, FM_ERR_CELL_OV); break; }
        if (elapsed >= ctx->cfg.cv_charge_timeout_ms) { fm_fail(ctx, FM_ERR_TIMEOUT); break; }
        if (fabsf((float)ctx->current_mA) <= ctx->cfg.cv_cutoff_current_mA) {
            fm_output_off();
            fm_goto(ctx, FM_REST_AFTER_CHG);
        }
        break;
    }

    case FM_REST_AFTER_CHG:
        fm_output_off();
        if (elapsed >= ctx->cfg.rest_ms) fm_goto(ctx, FM_CC_DISCHARGE);
        break;

    case FM_CC_DISCHARGE:
        dac8552_set_current(DAC_CH_CHG, 0.0f);
        dac8552_set_current(DAC_CH_DSG, ctx->cfg.discharge_current_mA);
        ctx->discharged_mAh += dmAh;
        if (fm_any_cell_le(ctx, ctx->cfg.uv_cell_mV)) { fm_fail(ctx, FM_ERR_CELL_UV); break; }
        if (elapsed >= ctx->cfg.cc_discharge_timeout_ms) { fm_fail(ctx, FM_ERR_TIMEOUT); break; }
        if (ctx->pack_mV <= ctx->cfg.discharge_pack_min_mV) {
            fm_output_off();
            fm_goto(ctx, FM_REST_AFTER_DSG);
        }
        break;

    case FM_REST_AFTER_DSG:
        fm_output_off();
        if (elapsed >= ctx->cfg.rest_ms) {
            fm_goto(ctx, FM_COMPLETE);
            printf("[FM] DONE charged=%d mAh discharged=%d mAh\r\n",
                   (int)ctx->charged_mAh, (int)ctx->discharged_mAh);
        }
        break;

    default:
        break;
    }
}

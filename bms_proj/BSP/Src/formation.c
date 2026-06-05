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

/* CV 阶段 PI 控制器增益 (需上板整定)
 * Kp: 比例项 mA/mV — 电压偏差的即时响应
 * Ki: 积分项 mA/(mV·s) — 消除稳态误差 */
#define FM_CV_KP  2.0f
#define FM_CV_KI  5.0f

/* 软启动爬坡时间 (ms): CC 阶段前 N ms 内电流线性从 0 爬到设定值 */
#define FM_RAMP_MS  3000U

/* 单体压差告警阈值 (mV): 超过此值串口提示, 不停机 */
#define FM_CELL_DELTA_WARN_mV  200


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
    cfg->cc_charge_timeout_ms    = 3u * 3600u * 1000u;
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
    printf("[FM] ERROR code=%d (cells %u %u %u %u %u mV, T=%dC, delta=%umV)\r\n",
           (int)e, ctx->cell_mV[0], ctx->cell_mV[1], ctx->cell_mV[2],
           ctx->cell_mV[3], ctx->cell_mV[4], ctx->temp_C, ctx->cell_delta_mV);
}

/* 读取一轮测量，写入 ctx。电压读失败视为致命，返回 false。 */
static bool fm_read_sensors(fm_ctx_t *ctx)
{
    bq76_info_t info;
    if (!bq76_read_info(&info)) return false;

    uint32_t pack = 0;
    uint16_t vmax = 0, vmin = 0xFFFF;
    for (int i = 0; i < 5; ++i) {
        ctx->cell_mV[i] = info.vc_mV[i];
        pack += info.vc_mV[i];
        if (info.vc_mV[i] > vmax) vmax = info.vc_mV[i];
        if (info.vc_mV[i] < vmin) vmin = info.vc_mV[i];
    }
    ctx->pack_mV = pack;
    ctx->cell_max_mV = vmax;
    ctx->cell_min_mV = vmin;
    ctx->cell_delta_mV = vmax - vmin;

    ctx->current_mA = ina226_current_mA();

    /* 温度 1Hz 节流 */
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

/* 软启动爬坡系数: 0→1 线性, 超过 RAMP_MS 后恒 1 */
static float fm_ramp(uint32_t elapsed_ms)
{
    if (elapsed_ms >= FM_RAMP_MS) return 1.0f;
    return (float)elapsed_ms / (float)FM_RAMP_MS;
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
    ctx->prev_current_mA = 0;
    ctx->last_tick = HAL_GetTick();
    fm_output_off();
    fm_goto(ctx, FM_CC_CHARGE);
}

void fm_stop(fm_ctx_t *ctx)
{
    fm_output_off();
    fm_goto(ctx, FM_IDLE);
}

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
}

void fm_tick(fm_ctx_t *ctx)
{
    uint32_t now = HAL_GetTick();
    uint32_t dt_ms = now - ctx->last_tick;
    ctx->last_tick = now;

    if (ctx->state == FM_ERROR) return;

    bool sensors_ok = fm_read_sensors(ctx);

    if (ctx->state == FM_IDLE || ctx->state == FM_COMPLETE) return;

    if (!sensors_ok) { fm_fail(ctx, FM_ERR_SENSOR); return; }

    /* 过温保护 */
    if (ctx->temp_C >= ctx->cfg.max_temp_C) { fm_fail(ctx, FM_ERR_OVER_TEMP); return; }

    /* 梯形积分容量: mAh += (|I_prev| + |I_now|) / 2 × dt / 3.6e6 */
    float i_prev = fabsf((float)ctx->prev_current_mA);
    float i_now  = fabsf((float)ctx->current_mA);
    float dmAh   = (i_prev + i_now) * 0.5f * (float)dt_ms / 3600000.0f;
    ctx->prev_current_mA = ctx->current_mA;

    uint32_t elapsed = now - ctx->state_start_tick;

    /* 单体压差告警 (不停机, 仅日志) */
    if (ctx->cell_delta_mV >= FM_CELL_DELTA_WARN_mV) {
        static uint32_t last_warn = 0;
        if ((now - last_warn) >= 5000U) {
            printf("[FM] WARN cell delta=%umV (max=%u min=%u)\r\n",
                   ctx->cell_delta_mV, ctx->cell_max_mV, ctx->cell_min_mV);
            last_warn = now;
        }
    }

    switch (ctx->state) {

    case FM_CC_CHARGE: {
        float target = ctx->cfg.charge_current_mA * fm_ramp(elapsed);  /* 软启动 */
        dac8552_set_current(DAC_CH_DSG, 0.0f);
        dac8552_set_current(DAC_CH_CHG, target);
        ctx->charged_mAh += dmAh;
        if (fm_any_cell_ge(ctx, ctx->cfg.ov_cell_mV)) { fm_fail(ctx, FM_ERR_CELL_OV); break; }
        if (elapsed >= ctx->cfg.cc_charge_timeout_ms)  { fm_fail(ctx, FM_ERR_TIMEOUT); break; }
        if (ctx->pack_mV >= ctx->cfg.charge_pack_max_mV) {
            ctx->cv_setpoint_mA = ctx->cfg.charge_current_mA;
            fm_goto(ctx, FM_CV_CHARGE);
        }
        break;
    }

    case FM_CV_CHARGE: {
        /* PI 控制维持总压 = charge_pack_max_mV */
        float err_mV = (float)ctx->cfg.charge_pack_max_mV - (float)ctx->pack_mV;
        float dt_s = (float)dt_ms / 1000.0f;

        /* P 项: 即时响应 */
        float p_term = FM_CV_KP * err_mV;
        /* I 项: 消除稳态误差 (累积到 cv_setpoint) */
        ctx->cv_setpoint_mA += FM_CV_KI * err_mV * dt_s;

        float output = ctx->cv_setpoint_mA + p_term;
        if (output < 0.0f) output = 0.0f;
        if (output > ctx->cfg.charge_current_mA)
            output = ctx->cfg.charge_current_mA;
        /* 防积分饱和: 钳位后回写 setpoint */
        ctx->cv_setpoint_mA = output - p_term;
        if (ctx->cv_setpoint_mA < 0.0f) ctx->cv_setpoint_mA = 0.0f;

        dac8552_set_current(DAC_CH_DSG, 0.0f);
        dac8552_set_current(DAC_CH_CHG, output);
        ctx->charged_mAh += dmAh;
        if (fm_any_cell_ge(ctx, ctx->cfg.ov_cell_mV)) { fm_fail(ctx, FM_ERR_CELL_OV); break; }
        if (elapsed >= ctx->cfg.cv_charge_timeout_ms)  { fm_fail(ctx, FM_ERR_TIMEOUT); break; }
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

    case FM_CC_DISCHARGE: {
        float target = ctx->cfg.discharge_current_mA * fm_ramp(elapsed);  /* 软启动 */
        dac8552_set_current(DAC_CH_CHG, 0.0f);
        dac8552_set_current(DAC_CH_DSG, target);
        ctx->discharged_mAh += dmAh;
        if (fm_any_cell_le(ctx, ctx->cfg.uv_cell_mV))   { fm_fail(ctx, FM_ERR_CELL_UV); break; }
        if (elapsed >= ctx->cfg.cc_discharge_timeout_ms) { fm_fail(ctx, FM_ERR_TIMEOUT); break; }
        if (ctx->pack_mV <= ctx->cfg.discharge_pack_min_mV) {
            fm_output_off();
            fm_goto(ctx, FM_REST_AFTER_DSG);
        }
        break;
    }

    case FM_REST_AFTER_DSG:
        fm_output_off();
        if (elapsed >= ctx->cfg.rest_ms) {
            fm_goto(ctx, FM_COMPLETE);
            printf("[FM] DONE charged=%d mAh discharged=%d mAh delta=%umV\r\n",
                   (int)ctx->charged_mAh, (int)ctx->discharged_mAh, ctx->cell_delta_mV);
        }
        break;

    default:
        break;
    }
}

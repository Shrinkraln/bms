#ifndef __FORMATION_H__
#define __FORMATION_H__

/* ============================================================
 *  化成测试状态机 (formation)
 *  流程: IDLE → CC_CHARGE → CV_CHARGE → REST → CC_DISCHARGE → REST → DONE
 *  依赖现有 BSP: bq76920(电压) / periph_tests(INA226 电流, TMP117 温度) /
 *               dac8552(恒流设定)。
 *  恒流由 DAC 通道驱动 OPA2188 闭环: B=充电, A=放电 (上板核对方向)。
 * ============================================================ */
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    FM_IDLE = 0,
    FM_CC_CHARGE,
    FM_CV_CHARGE,
    FM_REST_AFTER_CHG,
    FM_CC_DISCHARGE,
    FM_REST_AFTER_DSG,
    FM_COMPLETE,
    FM_ERROR,
} fm_state_t;

typedef enum {
    FM_ERR_NONE = 0,
    FM_ERR_CELL_OV,
    FM_ERR_CELL_UV,
    FM_ERR_OVER_TEMP,
    FM_ERR_SENSOR,      /* 读传感器失败 */
    FM_ERR_TIMEOUT,     /* 某阶段超时 (未接电池/电压不动) */
} fm_error_t;

typedef struct {
    /* 充电 */
    float    charge_current_mA;     /* CC 充电电流 */
    uint16_t charge_pack_max_mV;    /* 充电截止总压 (≈5×4.2V) */
    float    cv_cutoff_current_mA;  /* CV 截止电流 (≈0.05C) */
    /* 放电 */
    float    discharge_current_mA;
    uint16_t discharge_pack_min_mV; /* 放电截止总压 (≈5×2.5V) */
    /* 通用 */
    uint32_t rest_ms;               /* 静置时长 */
    uint16_t ov_cell_mV;            /* 单体过压保护 */
    uint16_t uv_cell_mV;            /* 单体欠压保护 */
    int16_t  max_temp_C;            /* 过温保护 */
    /* 各阶段超时兜底 (未接电池/电压长时间不动时退出) */
    uint32_t cc_charge_timeout_ms;
    uint32_t cv_charge_timeout_ms;
    uint32_t cc_discharge_timeout_ms;
} fm_config_t;

typedef struct {
    fm_state_t  state;
    fm_error_t  error;
    uint32_t    state_start_tick;   /* 进入当前状态的时刻 (HAL_GetTick) */
    uint32_t    last_tick;          /* 上次 tick 时刻 (容量积分用) */

    /* 实时测量 */
    uint16_t    cell_mV[5];
    uint32_t    pack_mV;
    int32_t     current_mA;         /* 有符号 */
    int16_t     temp_C;

    /* 累计容量 */
    float       charged_mAh;
    float       discharged_mAh;

    /* CV 阶段当前电流设定 (内部状态) */
    float       cv_setpoint_mA;

    fm_config_t cfg;
} fm_ctx_t;

/* 填充一组默认参数 (5S / 2.5Ah / 0.5C, 静置 30 min)。 */
void  fm_default_config(fm_config_t *cfg);

/* 初始化上下文 (state=IDLE)。cfg 可为 NULL → 用默认。 */
void  fm_init(fm_ctx_t *ctx, const fm_config_t *cfg);

/* 启动一轮化成 (从 IDLE/COMPLETE 进入 CC_CHARGE)。 */
void  fm_start(fm_ctx_t *ctx);

/* 停止并回到 IDLE，关断电流。 */
void  fm_stop(fm_ctx_t *ctx);

/* 周期调用 (建议 50~500ms)；推进状态机、读测量、积分容量、控制 DAC。 */
void  fm_tick(fm_ctx_t *ctx);

/* 状态名 (打印用)。 */
const char *fm_state_name(fm_state_t s);

#endif

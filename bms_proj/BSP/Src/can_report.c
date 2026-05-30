#include "can_report.h"
#include "stm32g4xx_hal.h"
#include <stdio.h>

extern FDCAN_HandleTypeDef hfdcan1;

bool can_report_init(void)
{
    HAL_FDCAN_Stop(&hfdcan1);                 /* 自检的回环模式可能还在，先停 */
    hfdcan1.Init.Mode = FDCAN_MODE_NORMAL;
    if (HAL_FDCAN_Init(&hfdcan1) != HAL_OK) return false;

    /* 默认拒收, 然后单独放行 0x200 (上位机命令帧) → FIFO0 */
    HAL_FDCAN_ConfigGlobalFilter(&hfdcan1, FDCAN_REJECT, FDCAN_REJECT,
                                 FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE);

    FDCAN_FilterTypeDef f = {0};
    f.IdType       = FDCAN_STANDARD_ID;
    f.FilterIndex  = 0;
    f.FilterType   = FDCAN_FILTER_MASK;
    f.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    f.FilterID1    = 0x200;                   /* 命令 ID */
    f.FilterID2    = 0x7FF;                   /* 全 1 = 精确匹配 */
    if (HAL_FDCAN_ConfigFilter(&hfdcan1, &f) != HAL_OK) return false;

    return HAL_FDCAN_Start(&hfdcan1) == HAL_OK;
}

static inline void put_u16le(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)(v >> 8);
}

static bool can_tx(uint16_t id, const uint8_t data[8])
{
    if (HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan1) == 0) return false;  /* 满则丢弃 */

    FDCAN_TxHeaderTypeDef tx = {0};
    tx.Identifier          = id;
    tx.IdType              = FDCAN_STANDARD_ID;
    tx.TxFrameType         = FDCAN_DATA_FRAME;
    tx.DataLength          = FDCAN_DLC_BYTES_8;
    tx.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    tx.BitRateSwitch       = FDCAN_BRS_OFF;
    tx.FDFormat            = FDCAN_CLASSIC_CAN;
    tx.TxEventFifoControl  = FDCAN_NO_TX_EVENTS;
    tx.MessageMarker       = 0;

    return HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &tx, (uint8_t *)data) == HAL_OK;
}

/* === Bus-Off 恢复 ===
 * FDCAN 进入 Bus-Off (TEC > 255) 后, 默认按 ISO 11898 进入恢复序列:
 *   需要在 RX 引脚上看到 128 次连续 11 个隐性位才会回 Error-Active。
 * 但若总线完全没流量 (上位机也下了线), 永远恢复不了。
 * 这里检测到 Bus-Off 后, 主动 Stop->Start 强制重启控制器。
 * 节流: 至少间隔 1s 重启一次, 避免疯狂重启把日志/CPU 耗光。 */
bool can_report_recover_if_busoff(void)
{
    FDCAN_ProtocolStatusTypeDef ps = {0};
    if (HAL_FDCAN_GetProtocolStatus(&hfdcan1, &ps) != HAL_OK) return false;
    if (!ps.BusOff) return true;       /* 正常 */

    static uint32_t last_restart_tick = 0;
    uint32_t now = HAL_GetTick();
    if ((now - last_restart_tick) < 1000U) return false;
    last_restart_tick = now;

    FDCAN_ErrorCountersTypeDef ec = {0};
    (void)HAL_FDCAN_GetErrorCounters(&hfdcan1, &ec);
    printf("[CAN] BusOff detected -> restart (TEC=%lu REC=%lu LEC=%lu)\r\n",
           (unsigned long)ec.TxErrorCnt, (unsigned long)ec.RxErrorCnt,
           (unsigned long)ps.LastErrorCode);

    if (HAL_FDCAN_Stop(&hfdcan1) != HAL_OK) return false;
    if (HAL_FDCAN_Start(&hfdcan1) != HAL_OK) {
        printf("[CAN] restart Start() failed\r\n");
        return false;
    }
    printf("[CAN] restart OK\r\n");
    return true;
}

bool can_report_send_status(const fm_ctx_t *ctx)
{
    uint8_t d[8];
    bool ok = true;

    /* 0x100 STATUS */
    uint16_t pack = (ctx->pack_mV > 0xFFFF) ? 0xFFFF : (uint16_t)ctx->pack_mV;
    uint16_t chg  = (ctx->charged_mAh > 65535.0f) ? 65535 : (uint16_t)ctx->charged_mAh;
    put_u16le(&d[0], pack);
    put_u16le(&d[2], (uint16_t)(int16_t)ctx->current_mA);
    d[4] = (uint8_t)ctx->state;
    d[5] = (uint8_t)(int8_t)ctx->temp_C;
    put_u16le(&d[6], chg);
    ok &= can_tx(CAN_ID_STATUS, d);

    /* 0x101 CELLS_A (1..4) */
    put_u16le(&d[0], ctx->cell_mV[0]);
    put_u16le(&d[2], ctx->cell_mV[1]);
    put_u16le(&d[4], ctx->cell_mV[2]);
    put_u16le(&d[6], ctx->cell_mV[3]);
    ok &= can_tx(CAN_ID_CELLS_A, d);

    /* 0x102 CELLS_B (5 + discharged + error) */
    uint16_t dsg = (ctx->discharged_mAh > 65535.0f) ? 65535 : (uint16_t)ctx->discharged_mAh;
    put_u16le(&d[0], ctx->cell_mV[4]);
    put_u16le(&d[2], dsg);
    d[4] = (uint8_t)ctx->error;
    d[5] = d[6] = d[7] = 0;
    ok &= can_tx(CAN_ID_CELLS_B, d);

    return ok;
}

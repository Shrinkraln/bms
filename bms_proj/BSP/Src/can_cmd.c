/**
 * can_cmd.c — CAN 命令帧解析 + 分发 + ACK 回执
 */
#include "can_cmd.h"
#include "stm32g4xx_hal.h"
#include <stdio.h>

extern FDCAN_HandleTypeDef hfdcan1;

static fm_ctx_t *s_fm = NULL;

void can_cmd_init(fm_ctx_t *fm)
{
    s_fm = fm;
}

static inline uint16_t rd_u16le(const uint8_t *p)
{
    return (uint16_t)(p[0] | (p[1] << 8));
}

/* 回发 ACK 帧 0x201, payload = [opcode, ack_code, fm_state, fm_error, 0,0,0,0] */
static void send_ack(uint8_t opcode, uint8_t ack_code)
{
    if (HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan1) == 0) return;

    uint8_t d[8] = {0};
    d[0] = opcode;
    d[1] = ack_code;
    d[2] = (uint8_t)(s_fm ? s_fm->state : 0);
    d[3] = (uint8_t)(s_fm ? s_fm->error : 0);

    FDCAN_TxHeaderTypeDef tx = {0};
    tx.Identifier          = CAN_ACK_ID;
    tx.IdType              = FDCAN_STANDARD_ID;
    tx.TxFrameType         = FDCAN_DATA_FRAME;
    tx.DataLength          = FDCAN_DLC_BYTES_8;
    tx.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    tx.BitRateSwitch       = FDCAN_BRS_OFF;
    tx.FDFormat            = FDCAN_CLASSIC_CAN;
    tx.TxEventFifoControl  = FDCAN_NO_TX_EVENTS;
    tx.MessageMarker       = 0;

    (void)HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &tx, d);
}

/* 化成电流的合理范围 (硬件 DAC + OPA2188 + 0.1Ω 采样阻设计上限) */
#define CMD_I_MIN_mA  10
#define CMD_I_MAX_mA  10000

static uint8_t can_cmd_dispatch(uint8_t opcode, const uint8_t *p, uint8_t len)
{
    if (!s_fm) return CAN_ACK_BAD_STATE;

    switch (opcode) {

    case CAN_CMD_OP_START:
        if (s_fm->state != FM_IDLE && s_fm->state != FM_COMPLETE && s_fm->state != FM_ERROR) {
            printf("[CAN-CMD] START rejected (state=%s)\r\n", fm_state_name(s_fm->state));
            return CAN_ACK_BAD_STATE;
        }
        printf("[CAN-CMD] START\r\n");
        fm_start(s_fm);
        return CAN_ACK_OK;

    case CAN_CMD_OP_STOP:
        printf("[CAN-CMD] STOP\r\n");
        fm_stop(s_fm);
        return CAN_ACK_OK;

    case CAN_CMD_OP_SET_CHG_I:
        if (len < 3) return CAN_ACK_BAD_PARAM;
        {
            uint16_t mA = rd_u16le(p + 1);
            if (mA < CMD_I_MIN_mA || mA > CMD_I_MAX_mA) {
                printf("[CAN-CMD] SET_CHG_I %u out of range\r\n", mA);
                return CAN_ACK_BAD_PARAM;
            }
            s_fm->cfg.charge_current_mA = (float)mA;
            printf("[CAN-CMD] charge_current_mA = %u\r\n", mA);
            return CAN_ACK_OK;
        }

    case CAN_CMD_OP_SET_DSG_I:
        if (len < 3) return CAN_ACK_BAD_PARAM;
        {
            uint16_t mA = rd_u16le(p + 1);
            if (mA < CMD_I_MIN_mA || mA > CMD_I_MAX_mA) {
                printf("[CAN-CMD] SET_DSG_I %u out of range\r\n", mA);
                return CAN_ACK_BAD_PARAM;
            }
            s_fm->cfg.discharge_current_mA = (float)mA;
            printf("[CAN-CMD] discharge_current_mA = %u\r\n", mA);
            return CAN_ACK_OK;
        }

    case CAN_CMD_OP_CLEAR_ERR:
        if (s_fm->state != FM_ERROR) {
            printf("[CAN-CMD] CLEAR_ERR ignored (not in ERROR)\r\n");
            return CAN_ACK_BAD_STATE;
        }
        s_fm->error = FM_ERR_NONE;
        s_fm->state = FM_IDLE;
        printf("[CAN-CMD] CLEAR_ERR -> IDLE\r\n");
        return CAN_ACK_OK;

    default:
        printf("[CAN-CMD] unknown opcode 0x%02X\r\n", opcode);
        return CAN_ACK_UNKNOWN_OP;
    }
}

void can_cmd_poll(void)
{
    while (HAL_FDCAN_GetRxFifoFillLevel(&hfdcan1, FDCAN_RX_FIFO0) > 0) {
        FDCAN_RxHeaderTypeDef hdr;
        uint8_t data[8];
        if (HAL_FDCAN_GetRxMessage(&hfdcan1, FDCAN_RX_FIFO0, &hdr, data) != HAL_OK) break;
        if (hdr.Identifier != CAN_CMD_ID) continue;

        /* dlc 字段是 enum (FDCAN_DLC_BYTES_n), 不是直接的字节数 */
        uint8_t len = (uint8_t)(hdr.DataLength >> 16);  /* HAL G4: DataLength 是 enum 值; 简化按 8 处理 */
        if (len == 0 || len > 8) len = 8;

        uint8_t opcode = data[0];
        uint8_t ack    = can_cmd_dispatch(opcode, data, len);
        send_ack(opcode, ack);
    }
}

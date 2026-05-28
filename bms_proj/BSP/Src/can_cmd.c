/**
 * can_cmd.c — CAN 命令帧解析 + 分发
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

static void can_cmd_dispatch(uint8_t opcode, const uint8_t *p, uint8_t len)
{
    if (!s_fm) return;
    switch (opcode) {

    case CAN_CMD_OP_START:
        printf("[CAN-CMD] START\r\n");
        fm_start(s_fm);
        break;

    case CAN_CMD_OP_STOP:
        printf("[CAN-CMD] STOP\r\n");
        fm_stop(s_fm);
        break;

    case CAN_CMD_OP_SET_CHG_I:
        if (len >= 3) {
            uint16_t mA = rd_u16le(p + 1);
            s_fm->cfg.charge_current_mA = (float)mA;
            printf("[CAN-CMD] charge_current_mA = %u\r\n", mA);
        }
        break;

    case CAN_CMD_OP_SET_DSG_I:
        if (len >= 3) {
            uint16_t mA = rd_u16le(p + 1);
            s_fm->cfg.discharge_current_mA = (float)mA;
            printf("[CAN-CMD] discharge_current_mA = %u\r\n", mA);
        }
        break;

    case CAN_CMD_OP_CLEAR_ERR:
        if (s_fm->state == FM_ERROR) {
            s_fm->error = FM_ERR_NONE;
            s_fm->state = FM_IDLE;
            printf("[CAN-CMD] CLEAR_ERR -> IDLE\r\n");
        }
        break;

    default:
        printf("[CAN-CMD] unknown opcode 0x%02X\r\n", opcode);
        break;
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

        can_cmd_dispatch(data[0], data, len);
    }
}

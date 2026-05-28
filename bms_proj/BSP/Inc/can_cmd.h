/**
 * can_cmd.h — 上位机命令帧 (CAN ID 0x200) 解析与分发
 *
 *   0x200 CMD (DLC ≤ 8, little-endian):
 *     [0]   = opcode
 *     [1..] = params (per opcode)
 *
 *   Opcodes:
 *     0x01 START         — 无参; fm_start(&fm) (仅当 IDLE/COMPLETE/ERROR)
 *     0x02 STOP          — 无参; fm_stop(&fm)
 *     0x03 SET_CHG_I     — [1:2] u16 mA → cfg.charge_current_mA
 *     0x04 SET_DSG_I     — [1:2] u16 mA → cfg.discharge_current_mA
 *     0x05 CLEAR_ERR     — 无参; ERROR 态强制回 IDLE
 *
 *   依赖 can_report_init() 已经把 ID=0x200 的 RX filter 装到 FIFO0。
 */
#ifndef __CAN_CMD_H__
#define __CAN_CMD_H__

#include "formation.h"

#define CAN_CMD_ID            0x200

#define CAN_CMD_OP_START      0x01
#define CAN_CMD_OP_STOP       0x02
#define CAN_CMD_OP_SET_CHG_I  0x03
#define CAN_CMD_OP_SET_DSG_I  0x04
#define CAN_CMD_OP_CLEAR_ERR  0x05

/* 保存 fm 指针, 命令进来后操作它。 */
void can_cmd_init(fm_ctx_t *fm);

/* 主循环里周期调用: drain RxFIFO0, 解析并分发。 */
void can_cmd_poll(void);

#endif

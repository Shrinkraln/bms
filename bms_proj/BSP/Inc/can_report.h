#ifndef __CAN_REPORT_H__
#define __CAN_REPORT_H__

#include <stdbool.h>
#include "formation.h"

/* ============================================================
 *  化成数据 CAN 周期上报协议
 *  Classic CAN, 11-bit 标准 ID, 500 kbps, **小端字节序(little-endian)**
 *
 *  0x100 STATUS  (DLC 8)
 *    [0:1] u16  pack_mV        总电压
 *    [2:3] i16  current_mA     电流(有符号)
 *    [4]   u8   state          fm_state_t (0=IDLE..7=ERROR)
 *    [5]   i8   temp_C         温度
 *    [6:7] u16  charged_mAh    已充容量
 *  0x101 CELLS_A (DLC 8)
 *    [0:1] u16 cell1_mV  [2:3] cell2  [4:5] cell3  [6:7] cell4
 *  0x102 CELLS_B (DLC 8)
 *    [0:1] u16 cell5_mV  [2:3] u16 discharged_mAh  [4] u8 error  [5:7] 保留
 * ============================================================ */
#define CAN_ID_STATUS    0x100
#define CAN_ID_CELLS_A   0x101
#define CAN_ID_CELLS_B   0x102

/* 把 FDCAN1 配成正常模式并启动 (上报前调用一次)。成功返回 true。
 * 注意：需保证 CAN 总线上有节点应答 (你的 CAN-USB 模块)，否则无 ACK 会反复重发。 */
bool can_report_init(void);

/* 发送一组化成状态 (3 帧)。Tx FIFO 满时丢弃，不阻塞。 */
bool can_report_send_status(const fm_ctx_t *ctx);

/* 主循环周期调用 (建议每秒一次)。检测 CAN 协议状态，若进入 Bus-Off
 * 自动尝试 Stop + Start 恢复。CAN 线被拔/噪声会引起 Bus-Off, 否则节点
 * 失联后不会自动回来。返回 true 表示总线当前 Active. */
bool can_report_recover_if_busoff(void);

#endif

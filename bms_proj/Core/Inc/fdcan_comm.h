#ifndef FDCAN_COMM_H
#define FDCAN_COMM_H

#include <stdint.h>
#include "stm32g4xx_hal.h"
#include "bms.h"

/* CAN frame IDs (base = BMS_CAN_NODE_ID) */
#define CAN_ID_PACK_STATUS      (BMS_CAN_NODE_ID + 0x00U)  /* pack V, I, SOC, state */
#define CAN_ID_CELL_V_0_3       (BMS_CAN_NODE_ID + 0x01U)  /* cells 0-3 */
#define CAN_ID_CELL_V_4_7       (BMS_CAN_NODE_ID + 0x02U)
#define CAN_ID_CELL_V_8_11      (BMS_CAN_NODE_ID + 0x03U)
#define CAN_ID_CELL_V_12_15     (BMS_CAN_NODE_ID + 0x04U)
#define CAN_ID_TEMPERATURES     (BMS_CAN_NODE_ID + 0x05U)  /* temps + faults */
#define CAN_ID_CMD              (BMS_CAN_NODE_ID + 0x10U)  /* receive commands */

extern FDCAN_HandleTypeDef hfdcan1;

void FDCAN_Comm_Init(void);
void FDCAN_Comm_TxPack(const BMS_Handle *bms);
void FDCAN_Comm_Process(BMS_Handle *bms);

#endif /* FDCAN_COMM_H */

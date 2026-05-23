#include "can_test.h"
#include "stm32g4xx_hal.h"
#include <string.h>

extern FDCAN_HandleTypeDef hfdcan1;

bool can_loopback_test(void)
{
    /* 切到内部回环模式 */
    if (HAL_FDCAN_Stop(&hfdcan1) != HAL_OK) { /* 可能未启动，忽略 */ }
    hfdcan1.Init.Mode = FDCAN_MODE_INTERNAL_LOOPBACK;
    if (HAL_FDCAN_Init(&hfdcan1) != HAL_OK) return false;

    /* 接收 FIFO0 任意 ID */
    FDCAN_FilterTypeDef f = {0};
    f.IdType        = FDCAN_STANDARD_ID;
    f.FilterIndex   = 0;
    f.FilterType    = FDCAN_FILTER_MASK;
    f.FilterConfig  = FDCAN_FILTER_TO_RXFIFO0;
    f.FilterID1     = 0x000;
    f.FilterID2     = 0x000;
    if (HAL_FDCAN_ConfigFilter(&hfdcan1, &f) != HAL_OK) return false;
    HAL_FDCAN_ConfigGlobalFilter(&hfdcan1, FDCAN_REJECT, FDCAN_REJECT,
                                 FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE);

    if (HAL_FDCAN_Start(&hfdcan1) != HAL_OK) return false;

    /* 发一帧 */
    FDCAN_TxHeaderTypeDef tx = {0};
    tx.Identifier        = 0x123;
    tx.IdType            = FDCAN_STANDARD_ID;
    tx.TxFrameType       = FDCAN_DATA_FRAME;
    tx.DataLength        = FDCAN_DLC_BYTES_8;
    tx.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    tx.BitRateSwitch     = FDCAN_BRS_OFF;
    tx.FDFormat          = FDCAN_CLASSIC_CAN;
    tx.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    tx.MessageMarker     = 0;

    uint8_t tx_data[8] = { 0x55, 0xAA, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
    if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &tx, tx_data) != HAL_OK)
        return false;

    /* 等接收 */
    uint32_t t0 = HAL_GetTick();
    while (HAL_FDCAN_GetRxFifoFillLevel(&hfdcan1, FDCAN_RX_FIFO0) == 0) {
        if ((HAL_GetTick() - t0) > 50) return false;
    }

    FDCAN_RxHeaderTypeDef rx = {0};
    uint8_t rx_data[8] = {0};
    if (HAL_FDCAN_GetRxMessage(&hfdcan1, FDCAN_RX_FIFO0, &rx, rx_data) != HAL_OK)
        return false;

    if (rx.Identifier != 0x123) return false;
    if (memcmp(rx_data, tx_data, 8) != 0) return false;

    /* 恢复正常模式（如果上层后续要用） */
    HAL_FDCAN_Stop(&hfdcan1);
    hfdcan1.Init.Mode = FDCAN_MODE_NORMAL;
    HAL_FDCAN_Init(&hfdcan1);
    return true;
}

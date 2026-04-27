#include "fdcan_comm.h"
#include "bms_config.h"

FDCAN_HandleTypeDef hfdcan1;

static FDCAN_TxHeaderTypeDef s_tx_hdr = {
    .IdType      = FDCAN_STANDARD_ID,
    .TxFrameType = FDCAN_DATA_FRAME,
    .DataLength  = FDCAN_DLC_BYTES_8,
    .ErrorStateIndicator = FDCAN_ESI_ACTIVE,
    .BitRateSwitch       = FDCAN_BRS_OFF,
    .FDFormat            = FDCAN_CLASSIC_CAN,
    .TxEventFifoControl  = FDCAN_NO_TX_EVENTS,
    .MessageMarker       = 0,
};

void FDCAN_Comm_Init(void)
{
    /* FDCAN1: PA11 = RX, PA12 = TX */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin       = GPIO_PIN_11 | GPIO_PIN_12;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = GPIO_AF9_FDCAN1;
    HAL_GPIO_Init(GPIOA, &gpio);

    __HAL_RCC_FDCAN_CLK_ENABLE();

    hfdcan1.Instance                  = FDCAN1;
    hfdcan1.Init.ClockDivider         = FDCAN_CLOCK_DIV1;
    hfdcan1.Init.FrameFormat          = FDCAN_FRAME_CLASSIC;
    hfdcan1.Init.Mode                 = FDCAN_MODE_NORMAL;
    hfdcan1.Init.AutoRetransmission   = ENABLE;
    hfdcan1.Init.TransmitPause        = DISABLE;
    hfdcan1.Init.ProtocolException    = DISABLE;
    hfdcan1.Init.NominalPrescaler     = BMS_CAN_PRESCALER;
    hfdcan1.Init.NominalSyncJumpWidth = BMS_CAN_SJW;
    hfdcan1.Init.NominalTimeSeg1      = BMS_CAN_SEG1;
    hfdcan1.Init.NominalTimeSeg2      = BMS_CAN_SEG2;
    hfdcan1.Init.MessageRAMOffset     = 0;
    hfdcan1.Init.StdFiltersNbr        = 1;
    hfdcan1.Init.ExtFiltersNbr        = 0;
    hfdcan1.Init.RxFifo0ElmNbr       = 8;
    hfdcan1.Init.RxFifo0ElmSize       = FDCAN_DATA_BYTES_8;
    hfdcan1.Init.TxEventsNbr          = 0;
    hfdcan1.Init.TxBuffersNbr         = 0;
    hfdcan1.Init.TxFifoQueueElmNbr   = 8;
    hfdcan1.Init.TxFifoQueueMode      = FDCAN_TX_FIFO_OPERATION;
    hfdcan1.Init.TxElmSize            = FDCAN_DATA_BYTES_8;
    HAL_FDCAN_Init(&hfdcan1);

    /* Accept command frame */
    FDCAN_FilterTypeDef filter = {0};
    filter.IdType       = FDCAN_STANDARD_ID;
    filter.FilterIndex  = 0;
    filter.FilterType   = FDCAN_FILTER_MASK;
    filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    filter.FilterID1    = CAN_ID_CMD;
    filter.FilterID2    = 0x7FFU;
    HAL_FDCAN_ConfigFilter(&hfdcan1, &filter);
    HAL_FDCAN_ConfigGlobalFilter(&hfdcan1, FDCAN_REJECT, FDCAN_REJECT,
                                 FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE);
    HAL_FDCAN_Start(&hfdcan1);
}

static void tx_frame(uint32_t id, const uint8_t *data, uint8_t len)
{
    s_tx_hdr.Identifier = id;
    s_tx_hdr.DataLength = len << 16; /* FDCAN_DLC encoding: bytes<<16 */
    HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &s_tx_hdr, (uint8_t *)data);
}

void FDCAN_Comm_TxPack(const BMS_Handle *bms)
{
    uint8_t buf[8];

    /* Frame 0: pack status */
    uint16_t vpack_dv = (uint16_t)(bms->meas.pack_mv / 10);  /* 0.01 V unit */
    int16_t  i_da     = (int16_t)(bms->meas.current_ma / 100); /* 0.1 A unit */
    buf[0] = vpack_dv >> 8;
    buf[1] = vpack_dv & 0xFF;
    buf[2] = i_da >> 8;
    buf[3] = i_da & 0xFF;
    buf[4] = bms->meas.soc_pct;
    buf[5] = (uint8_t)bms->state;
    buf[6] = (uint8_t)(bms->faults & 0xFF);
    buf[7] = (uint8_t)((bms->faults >> 8) & 0xFF);
    tx_frame(CAN_ID_PACK_STATUS, buf, 8);

    /* Frames 1-4: cell voltages (4 cells × 2 bytes per frame) */
    const uint32_t cell_ids[4] = {
        CAN_ID_CELL_V_0_3, CAN_ID_CELL_V_4_7,
        CAN_ID_CELL_V_8_11, CAN_ID_CELL_V_12_15
    };
    for (uint8_t g = 0; g < 4; g++) {
        for (uint8_t k = 0; k < 4; k++) {
            uint16_t mv = bms->meas.cell_mv[g * 4 + k];
            buf[k * 2]     = mv >> 8;
            buf[k * 2 + 1] = mv & 0xFF;
        }
        tx_frame(cell_ids[g], buf, 8);
    }

    /* Frame 5: temperatures + balance mask */
    buf[0] = (uint8_t)((bms->meas.temp_dC[0] >> 8) & 0xFF);
    buf[1] = (uint8_t)(bms->meas.temp_dC[0] & 0xFF);
    buf[2] = (uint8_t)((bms->meas.temp_dC[1] >> 8) & 0xFF);
    buf[3] = (uint8_t)(bms->meas.temp_dC[1] & 0xFF);
    buf[4] = (uint8_t)(bms->bal_mask >> 8);
    buf[5] = (uint8_t)(bms->bal_mask & 0xFF);
    buf[6] = 0;
    buf[7] = 0;
    tx_frame(CAN_ID_TEMPERATURES, buf, 8);
}

void FDCAN_Comm_Process(BMS_Handle *bms)
{
    FDCAN_RxHeaderTypeDef rx_hdr;
    uint8_t rx_buf[8];

    while (HAL_FDCAN_GetRxFifoFillLevel(&hfdcan1, FDCAN_RX_FIFO0) > 0) {
        HAL_FDCAN_GetRxMessage(&hfdcan1, FDCAN_RX_FIFO0, &rx_hdr, rx_buf);

        if (rx_hdr.Identifier == CAN_ID_CMD) {
            /* Byte 0: command
               0x01 = clear faults
               0x02 = force idle (open FETs)  */
            switch (rx_buf[0]) {
            case 0x01:
                BMS_ClearFaults(bms->faults);
                break;
            case 0x02:
                BMS_EnableCharge(false);
                BMS_EnableDischarge(false);
                break;
            default:
                break;
            }
        }
    }
}

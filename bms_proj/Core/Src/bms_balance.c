#include "bms_balance.h"
#include "bms_config.h"
#include "stm32g4xx_hal.h"

static void apply_bal_mask(uint16_t mask)
{
    /* PB0-PB15: high = enable bleed resistor for that cell */
    GPIOB->ODR = (GPIOB->ODR & ~BMS_BAL_PINS_MASK) | (mask & BMS_BAL_PINS_MASK);
}

void BMS_Balance_Update(BMS_Handle *bms)
{
    /* Stop balancing if any fault is active */
    if (bms->faults) {
        BMS_Balance_StopAll();
        bms->bal_mask = 0;
        return;
    }

    /* Find max cell voltage */
    uint16_t vmax = 0;
    for (uint8_t i = 0; i < BMS_CELL_COUNT; i++) {
        if (bms->meas.cell_mv[i] > vmax)
            vmax = bms->meas.cell_mv[i];
    }

    /* No balancing below minimum voltage */
    if (vmax < BMS_BAL_MIN_MV) {
        BMS_Balance_StopAll();
        bms->bal_mask = 0;
        return;
    }

    /* Enable balance for cells above (vmax - delta) */
    uint16_t mask = 0;
    uint16_t threshold = vmax - BMS_BAL_DELTA_MV;
    for (uint8_t i = 0; i < BMS_CELL_COUNT; i++) {
        if (bms->meas.cell_mv[i] >= threshold)
            mask |= (1U << i);
    }

    bms->bal_mask = mask;
    apply_bal_mask(mask);
}

void BMS_Balance_StopAll(void)
{
    GPIOB->ODR &= ~BMS_BAL_PINS_MASK;
}

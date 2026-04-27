#include "bms_protect.h"
#include "bms_config.h"

/* Hysteresis tracking */
static uint32_t s_ovp_cells  = 0;  /* bitmask of cells in OVP */
static uint32_t s_uvp_cells  = 0;

static void check_voltage(BMS_Handle *bms)
{
    for (uint8_t i = 0; i < BMS_CELL_COUNT; i++) {
        uint16_t mv = bms->meas.cell_mv[i];
        uint32_t bit = 1UL << i;

        if (mv >= BMS_OVP_MV) {
            s_ovp_cells |= bit;
        } else if (mv <= BMS_OVP_RELEASE_MV) {
            s_ovp_cells &= ~bit;
        }

        if (mv <= BMS_UVP_MV) {
            s_uvp_cells |= bit;
        } else if (mv >= BMS_UVP_RELEASE_MV) {
            s_uvp_cells &= ~bit;
        }
    }

    if (s_ovp_cells) {
        BMS_SetFaults(BMS_FAULT_CELL_OVP);
        BMS_EnableCharge(false);
    } else if (!(bms->faults & BMS_FAULT_CELL_OVP)) {
        /* already cleared */
    } else {
        BMS_ClearFaults(BMS_FAULT_CELL_OVP);
    }

    if (s_uvp_cells) {
        BMS_SetFaults(BMS_FAULT_CELL_UVP);
        BMS_EnableDischarge(false);
    } else if (!(bms->faults & BMS_FAULT_CELL_UVP)) {
        /* already cleared */
    } else {
        BMS_ClearFaults(BMS_FAULT_CELL_UVP);
    }
}

static void check_current(BMS_Handle *bms)
{
    int32_t i_ma = bms->meas.current_ma;

    /* Short-circuit: immediate trip */
    if (i_ma <= BMS_SCP_MA) {
        BMS_SetFaults(BMS_FAULT_SCP);
        BMS_EnableCharge(false);
        BMS_EnableDischarge(false);
        return;
    }

    if (i_ma >= BMS_CHG_OCP_MA) {
        BMS_SetFaults(BMS_FAULT_CHG_OCP);
        BMS_EnableCharge(false);
    } else {
        BMS_ClearFaults(BMS_FAULT_CHG_OCP);
    }

    if (i_ma <= BMS_DSG_OCP_MA) {
        BMS_SetFaults(BMS_FAULT_DSG_OCP);
        BMS_EnableDischarge(false);
    } else {
        BMS_ClearFaults(BMS_FAULT_DSG_OCP);
    }
}

static void check_temperature(BMS_Handle *bms)
{
    for (uint8_t i = 0; i < BMS_TEMP_COUNT; i++) {
        int16_t t = bms->meas.temp_dC[i];

        if (t >= BMS_CHG_OTP) {
            BMS_SetFaults(BMS_FAULT_CHG_OTP);
            BMS_EnableCharge(false);
        } else if (t <= BMS_CHG_OTP_RELEASE) {
            BMS_ClearFaults(BMS_FAULT_CHG_OTP);
        }

        if (t >= BMS_DSG_OTP) {
            BMS_SetFaults(BMS_FAULT_DSG_OTP);
            BMS_EnableDischarge(false);
        } else if (t <= BMS_DSG_OTP_RELEASE) {
            BMS_ClearFaults(BMS_FAULT_DSG_OTP);
        }

        if (t <= BMS_CHG_UTP) {
            BMS_SetFaults(BMS_FAULT_CHG_UTP);
            BMS_EnableCharge(false);
        } else if (t >= BMS_CHG_UTP_RELEASE) {
            BMS_ClearFaults(BMS_FAULT_CHG_UTP);
        }
    }
}

void BMS_Protect_Check(BMS_Handle *bms)
{
    check_voltage(bms);
    check_current(bms);
    check_temperature(bms);
}

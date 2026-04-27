#ifndef BMS_H
#define BMS_H

#include <stdint.h>
#include <stdbool.h>
#include "bms_config.h"

typedef enum {
    BMS_STATE_INIT = 0,
    BMS_STATE_IDLE,
    BMS_STATE_CHARGING,
    BMS_STATE_DISCHARGING,
    BMS_STATE_BALANCING,
    BMS_STATE_FAULT,
} BMS_State;

/* Fault bit flags */
#define BMS_FAULT_CELL_OVP      (1UL << 0)
#define BMS_FAULT_CELL_UVP      (1UL << 1)
#define BMS_FAULT_CHG_OCP       (1UL << 2)
#define BMS_FAULT_DSG_OCP       (1UL << 3)
#define BMS_FAULT_SCP           (1UL << 4)
#define BMS_FAULT_CHG_OTP       (1UL << 5)
#define BMS_FAULT_DSG_OTP       (1UL << 6)
#define BMS_FAULT_CHG_UTP       (1UL << 7)
#define BMS_FAULT_ADC_ERR       (1UL << 8)

typedef struct {
    uint16_t cell_mv[BMS_CELL_COUNT];   /* Cell voltages in mV */
    uint32_t pack_mv;                   /* Total pack voltage mV */
    int32_t  current_ma;                /* Positive = charging */
    int16_t  temp_dC[BMS_TEMP_COUNT];   /* Temperature in 0.1 °C */
    uint8_t  soc_pct;                   /* State of charge 0-100 */
} BMS_Meas;

typedef struct {
    BMS_State state;
    uint32_t  faults;
    BMS_Meas  meas;
    uint16_t  bal_mask;                 /* Balance channel active mask */
} BMS_Handle;

extern BMS_Handle g_bms;

void BMS_Init(void);
void BMS_Loop(void);

void BMS_SetFaults(uint32_t bits);
void BMS_ClearFaults(uint32_t bits);

void BMS_EnableCharge(bool en);
void BMS_EnableDischarge(bool en);

#endif /* BMS_H */

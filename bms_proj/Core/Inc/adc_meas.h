#ifndef ADC_MEAS_H
#define ADC_MEAS_H

#include <stdint.h>
#include "stm32g4xx_hal.h"
#include "bms_config.h"

extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;

void ADC_Meas_Init(void);
void ADC_Meas_Update(void);

uint32_t ADC_Meas_GetPackVoltage_mV(void);
int32_t  ADC_Meas_GetCurrent_mA(void);
int16_t  ADC_Meas_GetTemp_dC(uint8_t idx);

/* Stub: implement with external AFE (BQ76940 / LTC6811 / etc.) or analog mux */
uint16_t ADC_Meas_GetCellVoltage_mV(uint8_t cell_idx);

#endif /* ADC_MEAS_H */

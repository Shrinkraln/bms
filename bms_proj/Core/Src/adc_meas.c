#include "adc_meas.h"
#include <math.h>
#include <string.h>

ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

/* DMA buffer: [ch0_s0, ch1_s0, ch2_s0, ch3_s0, ch0_s1, ...] */
static volatile uint16_t s_dma_buf[ADC_NUM_CH * ADC_OVERSAMPLE];
static uint16_t s_avg[ADC_NUM_CH];

/* ---------- private helpers ---------- */

static uint16_t avg_channel(uint8_t ch)
{
    uint32_t sum = 0;
    for (uint8_t i = 0; i < ADC_OVERSAMPLE; i++)
        sum += s_dma_buf[ch + i * ADC_NUM_CH];
    return (uint16_t)(sum / ADC_OVERSAMPLE);
}

static uint32_t adc_to_mv(uint16_t raw)
{
    return (uint32_t)raw * BMS_VREF_MV / BMS_ADC_MAX;
}

static int16_t ntc_to_temp_dC(uint16_t raw)
{
    if (raw == 0 || raw >= BMS_ADC_MAX) return -999;

    /* R_ntc = R_pull * raw / (ADC_MAX - raw) */
    float r_ntc = (float)BMS_NTC_PULL_OHM * raw / (BMS_ADC_MAX - raw);
    float ln_r  = logf(r_ntc / (float)BMS_NTC_R25_OHM);
    float t_inv = 1.0f / 298.15f + ln_r / (float)BMS_NTC_BETA;
    float t_c   = 1.0f / t_inv - 273.15f;
    return (int16_t)(t_c * 10.0f);
}

/* ---------- init ---------- */

void ADC_Meas_Init(void)
{
    /* DMA1 Channel 1 for ADC1 */
    __HAL_RCC_DMA1_CLK_ENABLE();

    hdma_adc1.Instance                 = DMA1_Channel1;
    hdma_adc1.Init.Request             = DMA_REQUEST_ADC1;
    hdma_adc1.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_adc1.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_adc1.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
    hdma_adc1.Init.Mode                = DMA_CIRCULAR;
    hdma_adc1.Init.Priority            = DMA_PRIORITY_LOW;
    HAL_DMA_Init(&hdma_adc1);
    __HAL_LINKDMA(&hadc1, DMA_Handle, hdma_adc1);

    /* ADC1: scan, 4 channels, continuous via DMA */
    __HAL_RCC_ADC12_CLK_ENABLE();

    hadc1.Instance                   = ADC1;
    hadc1.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.GainCompensation      = 0;
    hadc1.Init.ScanConvMode          = ADC_SCAN_ENABLE;
    hadc1.Init.EOCSelection          = ADC_EOC_SEQ_CONV;
    hadc1.Init.LowPowerAutoWait      = DISABLE;
    hadc1.Init.ContinuousConvMode    = ENABLE;
    hadc1.Init.NbrOfConversion       = ADC_NUM_CH;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.DMAContinuousRequests = ENABLE;
    hadc1.Init.Overrun               = ADC_OVR_DATA_OVERWRITTEN;
    hadc1.Init.OversamplingMode      = DISABLE;
    HAL_ADC_Init(&hadc1);

    /* GPIO: PA0-PA3 as analog inputs */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin  = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    /* ADC channels: rank 1-4 */
    ADC_ChannelConfTypeDef ch = {0};
    ch.SamplingTime = ADC_SAMPLETIME_47CYCLES_5;
    ch.SingleDiff   = ADC_SINGLE_ENDED;
    ch.OffsetNumber = ADC_OFFSET_NONE;
    ch.Offset       = 0;

    const uint32_t channels[ADC_NUM_CH] = {
        ADC_CHANNEL_1, ADC_CHANNEL_2, ADC_CHANNEL_3, ADC_CHANNEL_4
    };
    for (uint8_t i = 0; i < ADC_NUM_CH; i++) {
        ch.Channel = channels[i];
        ch.Rank    = ADC_REGULAR_RANK_1 + i;
        HAL_ADC_ConfigChannel(&hadc1, &ch);
    }

    HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)s_dma_buf, ADC_NUM_CH * ADC_OVERSAMPLE);
}

/* ---------- public API ---------- */

void ADC_Meas_Update(void)
{
    for (uint8_t i = 0; i < ADC_NUM_CH; i++)
        s_avg[i] = avg_channel(i);
}

uint32_t ADC_Meas_GetPackVoltage_mV(void)
{
    uint32_t vadc_mv = adc_to_mv(s_avg[ADC_CH_VPACK]);
    return vadc_mv * BMS_VPACK_RATIO_NUM / BMS_VPACK_RATIO_DEN;
}

int32_t ADC_Meas_GetCurrent_mA(void)
{
    int32_t vadc_mv = (int32_t)adc_to_mv(s_avg[ADC_CH_CURRENT]);
    /* I_mA = (V - Voffset) * 1000 / (shunt_uΩ * gain / 1e6) */
    int32_t v_diff_mv = vadc_mv - (int32_t)BMS_I_OFFSET_MV;
    return v_diff_mv * 1000000L / (int32_t)(BMS_SHUNT_UOHM * BMS_AMP_GAIN);
}

int16_t ADC_Meas_GetTemp_dC(uint8_t idx)
{
    if (idx >= BMS_TEMP_COUNT) return -999;
    return ntc_to_temp_dC(s_avg[ADC_CH_NTC1 + idx]);
}

/* TODO: implement with external AFE (e.g. BQ76940 over I2C, LTC6811 over SPI)
   or with an analog mux feeding ADC1_IN5+ for individual cell readings. */
uint16_t ADC_Meas_GetCellVoltage_mV(uint8_t cell_idx)
{
    (void)cell_idx;
    return 3700U; /* placeholder — replace with AFE readout */
}

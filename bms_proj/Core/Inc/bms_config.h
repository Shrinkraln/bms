#ifndef BMS_CONFIG_H
#define BMS_CONFIG_H

/* ===== Pack configuration ===== */
#define BMS_CELL_COUNT          16U
#define BMS_TEMP_COUNT          2U      /* Number of NTC temperature sensors */

/* ===== Voltage thresholds (mV per cell) ===== */
#define BMS_OVP_MV              4250U
#define BMS_OVP_RELEASE_MV      4150U
#define BMS_UVP_MV              2750U
#define BMS_UVP_RELEASE_MV      2900U
#define BMS_BAL_MIN_MV          3000U   /* Min cell voltage to enable balancing */
#define BMS_BAL_DELTA_MV        10U     /* Balance when max-min delta exceeds this */

/* ===== Current thresholds (mA, positive = charge) ===== */
#define BMS_CHG_OCP_MA          30000L
#define BMS_DSG_OCP_MA          (-60000L)
#define BMS_SCP_MA              (-200000L)

/* ===== Temperature thresholds (unit: 0.1 °C) ===== */
#define BMS_CHG_OTP             450     /* +45.0 °C */
#define BMS_CHG_OTP_RELEASE     400
#define BMS_DSG_OTP             600     /* +60.0 °C */
#define BMS_DSG_OTP_RELEASE     500
#define BMS_CHG_UTP             (-100)  /* -10.0 °C */
#define BMS_CHG_UTP_RELEASE     0

/* ===== Timing (ms) ===== */
#define BMS_MEASURE_INTERVAL_MS 100U
#define BMS_BALANCE_INTERVAL_MS 1000U
#define BMS_CAN_TX_INTERVAL_MS  200U
#define BMS_WDG_KICK_INTERVAL_MS 500U

/* ===== ADC (STM32G474, 3.3 V reference) ===== */
#define BMS_VREF_MV             3300U
#define BMS_ADC_MAX             4095U

/* Pack voltage: Vadc = Vpack * R_bot/(R_top+R_bot)
   Vpack_mV = Vadc_mV * (R_top + R_bot) / R_bot
   Default divider: 120k + 10k => ratio = 13 */
#define BMS_VPACK_RATIO_NUM     130U    /* (R_top+R_bot)/R_bot * 10 */
#define BMS_VPACK_RATIO_DEN     10U

/* Current sense: bidirectional, 0 A at VREF/2
   I_mA = (Vadc_mV - offset_mV) * 1000 / (shunt_uOhm * amp_gain / 1000) */
#define BMS_I_OFFSET_MV         1650U
#define BMS_SHUNT_UOHM          500U    /* 0.5 mΩ shunt */
#define BMS_AMP_GAIN            20U

/* NTC 10k beta 3950, pull-up to VCC with 10k series resistor */
#define BMS_NTC_BETA            3950U
#define BMS_NTC_R25_OHM         10000U
#define BMS_NTC_PULL_OHM        10000U

/* ===== ADC channel index in DMA buffer ===== */
#define ADC_CH_VPACK            0U
#define ADC_CH_CURRENT          1U
#define ADC_CH_NTC1             2U
#define ADC_CH_NTC2             3U
#define ADC_NUM_CH              4U
#define ADC_OVERSAMPLE          16U

/* ===== GPIO — update to match schematic ===== */
#define BMS_CHG_EN_PORT         GPIOA
#define BMS_CHG_EN_PIN          GPIO_PIN_4
#define BMS_DSG_EN_PORT         GPIOA
#define BMS_DSG_EN_PIN          GPIO_PIN_5
#define BMS_PRECHG_PORT         GPIOA
#define BMS_PRECHG_PIN          GPIO_PIN_6
#define BMS_FAULT_LED_PORT      GPIOA
#define BMS_FAULT_LED_PIN       GPIO_PIN_7

/* Balance FET GPIOs: PB0..PB15 for cells 0..15 */
#define BMS_BAL_PORT            GPIOB
#define BMS_BAL_PINS_MASK       0xFFFFU  /* PB0-PB15 */

/* ===== FDCAN ===== */
#define BMS_CAN_NODE_ID         0x100U
#define BMS_CAN_PRESCALER       17U     /* 500 kbps @ 170 MHz: (1+13+6)*17/170M */
#define BMS_CAN_SEG1            13U
#define BMS_CAN_SEG2            6U
#define BMS_CAN_SJW             1U

#endif /* BMS_CONFIG_H */

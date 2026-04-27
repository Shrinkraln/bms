#include "bms.h"
#include "adc_meas.h"
#include "bms_protect.h"
#include "bms_balance.h"
#include "fdcan_comm.h"
#include "stm32g4xx_hal.h"

BMS_Handle g_bms;

static IWDG_HandleTypeDef hiwdg;

static uint32_t s_last_measure_ms;
static uint32_t s_last_balance_ms;
static uint32_t s_last_can_tx_ms;
static uint32_t s_last_wdg_ms;

/* ---------- GPIO / output helpers ---------- */

void BMS_EnableCharge(bool en)
{
    HAL_GPIO_WritePin(BMS_CHG_EN_PORT, BMS_CHG_EN_PIN,
                      en ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void BMS_EnableDischarge(bool en)
{
    HAL_GPIO_WritePin(BMS_DSG_EN_PORT, BMS_DSG_EN_PIN,
                      en ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void fault_led(bool on)
{
    HAL_GPIO_WritePin(BMS_FAULT_LED_PORT, BMS_FAULT_LED_PIN,
                      on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* ---------- fault management ---------- */

void BMS_SetFaults(uint32_t bits)
{
    g_bms.faults |= bits;
}

void BMS_ClearFaults(uint32_t bits)
{
    g_bms.faults &= ~bits;
}

/* ---------- measurement update ---------- */

static void update_measurements(void)
{
    ADC_Meas_Update();

    g_bms.meas.pack_mv    = ADC_Meas_GetPackVoltage_mV();
    g_bms.meas.current_ma = ADC_Meas_GetCurrent_mA();

    for (uint8_t i = 0; i < BMS_TEMP_COUNT; i++)
        g_bms.meas.temp_dC[i] = ADC_Meas_GetTemp_dC(i);

    for (uint8_t i = 0; i < BMS_CELL_COUNT; i++)
        g_bms.meas.cell_mv[i] = ADC_Meas_GetCellVoltage_mV(i);
}

/* ---------- SOC (coulomb counting placeholder) ---------- */

static void update_soc(void)
{
    /* TODO: implement coulomb counting or OCV lookup */
    /* Simple OCV estimate from min cell voltage for now */
    uint16_t vmin = 0xFFFF;
    for (uint8_t i = 0; i < BMS_CELL_COUNT; i++) {
        if (g_bms.meas.cell_mv[i] < vmin)
            vmin = g_bms.meas.cell_mv[i];
    }
    /* Linear map: 2750 mV = 0%, 4200 mV = 100% */
    if (vmin <= BMS_UVP_MV)
        g_bms.meas.soc_pct = 0;
    else if (vmin >= BMS_OVP_MV)
        g_bms.meas.soc_pct = 100;
    else
        g_bms.meas.soc_pct = (uint8_t)((uint32_t)(vmin - BMS_UVP_MV) * 100
                                         / (BMS_OVP_MV - BMS_UVP_MV));
}

/* ---------- state machine ---------- */

static void state_transition(BMS_State next)
{
    if (g_bms.state == next) return;

    if (next == BMS_STATE_FAULT) {
        BMS_EnableCharge(false);
        BMS_EnableDischarge(false);
        BMS_Balance_StopAll();
        fault_led(true);
    } else {
        fault_led(false);
    }

    g_bms.state = next;
}

static void run_state_machine(void)
{
    if (g_bms.faults) {
        state_transition(BMS_STATE_FAULT);
        return;
    }

    switch (g_bms.state) {
    case BMS_STATE_INIT:
        state_transition(BMS_STATE_IDLE);
        break;

    case BMS_STATE_IDLE:
        BMS_EnableCharge(true);
        BMS_EnableDischarge(true);
        if (g_bms.meas.current_ma > 500L)
            state_transition(BMS_STATE_CHARGING);
        else if (g_bms.meas.current_ma < -500L)
            state_transition(BMS_STATE_DISCHARGING);
        break;

    case BMS_STATE_CHARGING:
        if (g_bms.meas.current_ma < 200L) {
            /* Charge complete — try balancing */
            state_transition(BMS_STATE_BALANCING);
        } else if (g_bms.meas.current_ma < -500L) {
            state_transition(BMS_STATE_DISCHARGING);
        }
        break;

    case BMS_STATE_DISCHARGING:
        if (g_bms.meas.current_ma > 500L)
            state_transition(BMS_STATE_CHARGING);
        else if (g_bms.meas.current_ma >= -500L)
            state_transition(BMS_STATE_IDLE);
        break;

    case BMS_STATE_BALANCING:
        if (g_bms.meas.current_ma > 500L)
            state_transition(BMS_STATE_CHARGING);
        else if (g_bms.meas.current_ma < -500L)
            state_transition(BMS_STATE_DISCHARGING);
        break;

    case BMS_STATE_FAULT:
        if (!g_bms.faults)
            state_transition(BMS_STATE_IDLE);
        break;
    }
}

/* ---------- GPIO init ---------- */

static void gpio_output_init(GPIO_TypeDef *port, uint16_t pin)
{
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = pin;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(port, &gpio);
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
}

static void bms_gpio_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    gpio_output_init(BMS_CHG_EN_PORT,   BMS_CHG_EN_PIN);
    gpio_output_init(BMS_DSG_EN_PORT,   BMS_DSG_EN_PIN);
    gpio_output_init(BMS_PRECHG_PORT,   BMS_PRECHG_PIN);
    gpio_output_init(BMS_FAULT_LED_PORT, BMS_FAULT_LED_PIN);

    /* Balance FET outputs PB0-PB15 */
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = BMS_BAL_PINS_MASK;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BMS_BAL_PORT, &gpio);
    BMS_BAL_PORT->ODR &= ~BMS_BAL_PINS_MASK;
}

/* ---------- IWDG ---------- */

static void iwdg_init(void)
{
    /* ~1 s window with LSI ~32 kHz: prescaler=32, reload=1000 */
    hiwdg.Instance       = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_32;
    hiwdg.Init.Reload    = 1000U;
    hiwdg.Init.Window    = IWDG_WINDOW_DISABLE;
    HAL_IWDG_Init(&hiwdg);
}

/* ---------- public API ---------- */

void BMS_Init(void)
{
    bms_gpio_init();
    ADC_Meas_Init();
    FDCAN_Comm_Init();
    iwdg_init();

    g_bms.state    = BMS_STATE_INIT;
    g_bms.faults   = 0;
    g_bms.bal_mask = 0;

    s_last_measure_ms = HAL_GetTick();
    s_last_balance_ms = HAL_GetTick();
    s_last_can_tx_ms  = HAL_GetTick();
    s_last_wdg_ms     = HAL_GetTick();
}

void BMS_Loop(void)
{
    uint32_t now = HAL_GetTick();

    /* Watchdog kick */
    if (now - s_last_wdg_ms >= BMS_WDG_KICK_INTERVAL_MS) {
        HAL_IWDG_Refresh(&hiwdg);
        s_last_wdg_ms = now;
    }

    /* Measurements + protection at fast rate */
    if (now - s_last_measure_ms >= BMS_MEASURE_INTERVAL_MS) {
        update_measurements();
        update_soc();
        BMS_Protect_Check(&g_bms);
        run_state_machine();
        s_last_measure_ms = now;
    }

    /* Cell balancing at slower rate */
    if (now - s_last_balance_ms >= BMS_BALANCE_INTERVAL_MS) {
        if (g_bms.state == BMS_STATE_BALANCING)
            BMS_Balance_Update(&g_bms);
        else
            BMS_Balance_StopAll();
        s_last_balance_ms = now;
    }

    /* CAN transmit */
    if (now - s_last_can_tx_ms >= BMS_CAN_TX_INTERVAL_MS) {
        FDCAN_Comm_TxPack(&g_bms);
        s_last_can_tx_ms = now;
    }

    /* CAN receive — process every loop */
    FDCAN_Comm_Process(&g_bms);
}

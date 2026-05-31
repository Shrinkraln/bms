#include "bsp.h"
#include "bq76_alert.h"

extern UART_HandleTypeDef huart2;   // PA2/PA3 调试串口（CubeMX 生成）
extern SPI_HandleTypeDef  hspi2;    // CubeMX 生成但实际不用 (PB13/15 是触摸 I²C)

void bsp_init(void)
{
    /* 时钟由 SystemClock_Config() 提前完成，这里只配 GPIO 模式 */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef gi = {0};

    /* LED 输出（推挽，默认拉低） */
    gi.Pin   = LED_G_PIN | LED_R_PIN;
    gi.Mode  = GPIO_MODE_OUTPUT_PP;
    gi.Pull  = GPIO_NOPULL;
    gi.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &gi);
    HAL_GPIO_WritePin(LED_G_PORT, LED_G_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_R_PORT, LED_R_PIN, GPIO_PIN_RESET);

    /* 蜂鸣器 PB8 */
    gi.Pin = BUZZER_PIN;
    HAL_GPIO_Init(BUZZER_PORT, &gi);
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);

    /* BQ34 VEN PB9 */
    gi.Pin = BQ34_VEN_PIN;
    HAL_GPIO_Init(BQ34_VEN_PORT, &gi);
    HAL_GPIO_WritePin(BQ34_VEN_PORT, BQ34_VEN_PIN, GPIO_PIN_RESET);  // 默认关

    /* DAC CS PB10 */
    gi.Pin = DAC_CS_PIN;
    HAL_GPIO_Init(DAC_CS_PORT, &gi);
    HAL_GPIO_WritePin(DAC_CS_PORT, DAC_CS_PIN, GPIO_PIN_SET);

    /* LCD 控制脚 (DC/CS/RST 需要 HIGH 速度; CubeMX 也配了 HIGH, 这里统一) */
    gi.Pin   = LCD_DC_PIN | LCD_CS_PIN | LCD_RST_PIN;
    gi.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gi);
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_SET);

    /* 按键 PB12 输入，上拉 */
    gi.Pin  = KEY1_PIN;
    gi.Mode = GPIO_MODE_INPUT;
    gi.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(KEY1_PORT, &gi);

    /* BQ76 ALERT PB3 输入: CubeMX 已配 EXTI_RISING + PULLDOWN + EXTI3_IRQn 优先级 2,
     * 这里不要再 HAL_GPIO_Init() 覆盖, 否则会把 EXTI 模式抹成 plain input。
     * 见 Core/Src/gpio.c 中 BQ76_ALTER_IN_Pin 段。 */

    /* ---- 撤销 SPI2, 释放 PB13/PB15 给触摸 I²C ----
     * CubeMX 生成的 MX_SPI2_Init() 把 PB13/14/15 配成 SPI2_AF, 但实际:
     *   PB13 = FT6336U I²C SCL
     *   PB15 = FT6336U I²C SDA
     *   PB14 = NC (空)
     * 这里反初始化 SPI2, 然后把 PB13/PB15 重配为开漏输出 (软件 I²C)。 */
    HAL_SPI_DeInit(&hspi2);
    __HAL_RCC_SPI2_CLK_DISABLE();

    /* PB13 SCL: 开漏 + 上拉 (I²C 总线需外部上拉电阻, 这里内部上拉兜底) */
    gi.Pin   = GPIO_PIN_13;
    gi.Mode  = GPIO_MODE_OUTPUT_OD;
    gi.Pull  = GPIO_PULLUP;
    gi.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gi);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);

    /* PB15 SDA: 同上 */
    gi.Pin = GPIO_PIN_15;
    HAL_GPIO_Init(GPIOB, &gi);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);

    /* PA0: 触摸 RST (推挽, 默认高=运行) — CubeMX 标注为 TOUCH_CS, 实际接触摸复位 */
    gi.Pin   = GPIO_PIN_0;
    gi.Mode  = GPIO_MODE_OUTPUT_PP;
    gi.Pull  = GPIO_NOPULL;
    gi.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &gi);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
}

void led_g_on(void)     { HAL_GPIO_WritePin(LED_G_PORT, LED_G_PIN, GPIO_PIN_SET); }
void led_g_off(void)    { HAL_GPIO_WritePin(LED_G_PORT, LED_G_PIN, GPIO_PIN_RESET); }
void led_g_toggle(void) { HAL_GPIO_TogglePin(LED_G_PORT, LED_G_PIN); }
void led_r_on(void)     { HAL_GPIO_WritePin(LED_R_PORT, LED_R_PIN, GPIO_PIN_SET); }
void led_r_off(void)    { HAL_GPIO_WritePin(LED_R_PORT, LED_R_PIN, GPIO_PIN_RESET); }
void led_r_toggle(void) { HAL_GPIO_TogglePin(LED_R_PORT, LED_R_PIN); }

void buzzer_on(void)  { HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET); }
void buzzer_off(void) { HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET); }
void buzzer_beep(uint32_t ms)
{
    buzzer_on();
    HAL_Delay(ms);
    buzzer_off();
}

bool key1_pressed(void)
{
    return HAL_GPIO_ReadPin(KEY1_PORT, KEY1_PIN) == GPIO_PIN_RESET;
}

bool key1_wait(uint32_t timeout_ms)
{
    uint32_t t0 = HAL_GetTick();
    while ((HAL_GetTick() - t0) < timeout_ms) {
        if (key1_pressed()) {
            HAL_Delay(20);  // 消抖
            if (key1_pressed()) {
                /* 等待松手再返回, 避免重入; 最多 5s 防卡住 (按键卡死/长按) */
                uint32_t t_rel = HAL_GetTick();
                while (key1_pressed() && (HAL_GetTick() - t_rel) < 5000U) {
                    HAL_Delay(10);
                }
                return true;
            }
        }
        HAL_Delay(5);
    }
    return false;
}

void bq34_enable(bool en)
{
    HAL_GPIO_WritePin(BQ34_VEN_PORT, BQ34_VEN_PIN,
                      en ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

bool bq76_alert_active(void)
{
    return HAL_GPIO_ReadPin(BQ76_ALERT_PORT, BQ76_ALERT_PIN) == GPIO_PIN_SET;
}

void delay_ms(uint32_t ms) { HAL_Delay(ms); }

/* ===== EXTI 中断回调集中分发 =====
 * HAL_GPIO_EXTI_IRQHandler() (在 stm32g4xx_it.c 的 EXTIx_IRQHandler 内调) 会
 * 调到这里。HAL 默认是 __weak, 我们提供强符号一次性覆盖, 按 GPIO_Pin 分发:
 *   - PB3 = BQ76_ALERT_PIN  -> bq76_alert_isr()
 *   - PA8 = TOUCH_INT_Pin   -> (FT6336U 目前轮询, 占位)
 *   - PB12 = KEY1_PIN       -> (按键目前轮询, 占位)
 * 任何耗时操作 (I2C/打印) 都丢给 main 的 *_poll() 处理。 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == BQ76_ALERT_PIN) {
        bq76_alert_isr();
    }
    /* 其他 pin 暂不处理: TOUCH_INT / KEY1 仍走轮询 */
}

/* ===== 串口 printf 重定向 ===== */
int uart_dbg_write(const uint8_t *buf, uint16_t len)
{
    return (HAL_UART_Transmit(&huart2, (uint8_t*)buf, len, 100) == HAL_OK) ? len : -1;
}

/* 重定向 stdout 到 UART2，便于 printf。
 * 用弱符号，避免与 newlib_nano 默认实现或 CubeMX 模板的 _write 冲突。 */
#if defined(__GNUC__)
__attribute__((weak)) int __io_putchar(int ch)
{
    uint8_t c = (uint8_t)ch;
    HAL_UART_Transmit(&huart2, &c, 1, 10);
    return ch;
}
__attribute__((weak)) int _write(int file, char *ptr, int len)
{
    (void)file;
    HAL_UART_Transmit(&huart2, (uint8_t*)ptr, (uint16_t)len, 100);
    return len;
}
#endif

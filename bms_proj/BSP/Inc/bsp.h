#ifndef __BSP_H__
#define __BSP_H__

#include "stm32g4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* ===== 引脚定义（严格按原理图） ===== */
#define LED_G_PORT      GPIOB
#define LED_G_PIN       GPIO_PIN_4      // PB4 LED 绿
#define LED_R_PORT      GPIOB
#define LED_R_PIN       GPIO_PIN_5      // PB5 LED 红

#define BUZZER_PORT     GPIOB
#define BUZZER_PIN      GPIO_PIN_8      // PB8 蜂鸣器（有源，高电平响）

#define KEY1_PORT       GPIOB
#define KEY1_PIN        GPIO_PIN_12     // PB12 按键（按下=低）

#define BQ34_VEN_PORT   GPIOB
#define BQ34_VEN_PIN    GPIO_PIN_9      // PB9 BQ34Z100 使能（高有效）

#define BQ76_ALERT_PORT GPIOB
#define BQ76_ALERT_PIN  GPIO_PIN_3      // PB3 BQ76920 ALERT 输入

#define DAC_CS_PORT     GPIOB
#define DAC_CS_PIN      GPIO_PIN_10     // PB10 DAC8552 SYNC#

#define LCD_DC_PORT     GPIOB
#define LCD_DC_PIN      GPIO_PIN_0      // PB0 LCD DC
#define LCD_CS_PORT     GPIOB
#define LCD_CS_PIN      GPIO_PIN_1      // PB1 LCD CS
#define LCD_RST_PORT    GPIOB
#define LCD_RST_PIN     GPIO_PIN_2      // PB2 LCD RST

/* ===== 基础 API ===== */
void bsp_init(void);

void led_g_on(void);
void led_g_off(void);
void led_g_toggle(void);
void led_r_on(void);
void led_r_off(void);
void led_r_toggle(void);

void buzzer_on(void);
void buzzer_off(void);
void buzzer_beep(uint32_t ms);

bool key1_pressed(void);                // 立即查询
bool key1_wait(uint32_t timeout_ms);    // 阻塞等待，超时返回 false

void bq34_enable(bool en);              // 控制 PB9 = VEN
bool bq76_alert_active(void);           // 读 ALERT 状态

void delay_ms(uint32_t ms);

/* 串口 printf 重定向支持 */
void uart_dbg_init(void);
int  uart_dbg_write(const uint8_t *buf, uint16_t len);

#endif

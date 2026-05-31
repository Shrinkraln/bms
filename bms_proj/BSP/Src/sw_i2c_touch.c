#include "sw_i2c_touch.h"
#include "stm32g4xx_hal.h"

/* ---- 引脚定义 ---- */
#define SCL_PORT  GPIOB
#define SCL_PIN   GPIO_PIN_13
#define SDA_PORT  GPIOB
#define SDA_PIN   GPIO_PIN_15

#define SCL_H()   HAL_GPIO_WritePin(SCL_PORT, SCL_PIN, GPIO_PIN_SET)
#define SCL_L()   HAL_GPIO_WritePin(SCL_PORT, SCL_PIN, GPIO_PIN_RESET)
#define SDA_H()   HAL_GPIO_WritePin(SDA_PORT, SDA_PIN, GPIO_PIN_SET)
#define SDA_L()   HAL_GPIO_WritePin(SDA_PORT, SDA_PIN, GPIO_PIN_RESET)
#define SDA_RD()  HAL_GPIO_ReadPin(SDA_PORT, SDA_PIN)

/* 半周期延时 ~5us → ~100kHz (170MHz Cortex-M4, 每次循环 ~3 cycles) */
static inline void i2c_delay(void)
{
    volatile uint32_t n = 140;   /* 上板微调; 偏大=更慢更稳 */
    while (n--) __NOP();
}

void swi2c_touch_init(void)
{
    SCL_H();
    SDA_H();
    i2c_delay();
}

static void i2c_start(void)
{
    SDA_H(); i2c_delay();
    SCL_H(); i2c_delay();
    SDA_L(); i2c_delay();
    SCL_L(); i2c_delay();
}

static void i2c_stop(void)
{
    SDA_L(); i2c_delay();
    SCL_H(); i2c_delay();
    SDA_H(); i2c_delay();
}

/* 发 1 字节, 返回 ACK (0=ACK, 1=NACK) */
static uint8_t i2c_send_byte(uint8_t d)
{
    for (int i = 7; i >= 0; --i) {
        if (d & (1u << i)) SDA_H(); else SDA_L();
        i2c_delay();
        SCL_H(); i2c_delay();
        SCL_L(); i2c_delay();
    }
    /* 读 ACK */
    SDA_H(); i2c_delay();          /* 释放 SDA */
    SCL_H(); i2c_delay();
    uint8_t ack = SDA_RD();
    SCL_L(); i2c_delay();
    return ack;
}

/* 收 1 字节, ack=0 发 ACK, ack=1 发 NACK */
static uint8_t i2c_recv_byte(uint8_t ack)
{
    uint8_t d = 0;
    SDA_H();                       /* 释放 SDA */
    for (int i = 7; i >= 0; --i) {
        i2c_delay();
        SCL_H(); i2c_delay();
        if (SDA_RD()) d |= (1u << i);
        SCL_L();
    }
    /* 发 ACK/NACK */
    if (ack) SDA_H(); else SDA_L();
    i2c_delay();
    SCL_H(); i2c_delay();
    SCL_L(); i2c_delay();
    SDA_H();
    return d;
}

bool swi2c_touch_write(uint8_t addr7, uint8_t reg, const uint8_t *data, uint8_t len)
{
    i2c_start();
    if (i2c_send_byte((uint8_t)(addr7 << 1)))       { i2c_stop(); return false; }
    if (i2c_send_byte(reg))                          { i2c_stop(); return false; }
    for (uint8_t i = 0; i < len; ++i) {
        if (i2c_send_byte(data[i]))                  { i2c_stop(); return false; }
    }
    i2c_stop();
    return true;
}

bool swi2c_touch_read(uint8_t addr7, uint8_t reg, uint8_t *data, uint8_t len)
{
    /* 写寄存器地址 */
    i2c_start();
    if (i2c_send_byte((uint8_t)(addr7 << 1)))       { i2c_stop(); return false; }
    if (i2c_send_byte(reg))                          { i2c_stop(); return false; }
    /* 重复起始 + 读 */
    i2c_start();
    if (i2c_send_byte((uint8_t)(addr7 << 1) | 1))   { i2c_stop(); return false; }
    for (uint8_t i = 0; i < len; ++i) {
        data[i] = i2c_recv_byte(i == len - 1 ? 1 : 0);  /* 最后一字节 NACK */
    }
    i2c_stop();
    return true;
}

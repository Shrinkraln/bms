/**
 * lcd_st7796.c — ST7796S 3.5" 480×320 SPI 驱动 (诊断版)
 * 所有 printf 去掉, 用 LED 闪烁指示每一步是否通过
 */
#include "lcd_st7796.h"
#include "bsp.h"
#include "stm32g4xx_hal.h"

extern SPI_HandleTypeDef hspi1;

static inline void lcd_cs_l(void)  { HAL_GPIO_WritePin(LCD_CS_PORT,  LCD_CS_PIN,  GPIO_PIN_RESET); }
static inline void lcd_cs_h(void)  { HAL_GPIO_WritePin(LCD_CS_PORT,  LCD_CS_PIN,  GPIO_PIN_SET);   }
static inline void lcd_dc_l(void)  { HAL_GPIO_WritePin(LCD_DC_PORT,  LCD_DC_PIN,  GPIO_PIN_RESET); }
static inline void lcd_dc_h(void)  { HAL_GPIO_WritePin(LCD_DC_PORT,  LCD_DC_PIN,  GPIO_PIN_SET);   }
static inline void lcd_rst_l(void) { HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_RESET); }
static inline void lcd_rst_h(void) { HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_SET);   }

/* LED 信号: 绿灯闪 n 次 */
static void blink_g(int n)
{
    for (int i = 0; i < n; i++) {
        led_g_on(); HAL_Delay(120);
        led_g_off(); HAL_Delay(120);
    }
}
/* LED 信号: 红灯闪 n 次 */
static void blink_r(int n)
{
    for (int i = 0; i < n; i++) {
        led_r_on(); HAL_Delay(120);
        led_r_off(); HAL_Delay(120);
    }
}

static void lcd_cmd(uint8_t c)
{
    lcd_dc_l(); lcd_cs_l();
    HAL_SPI_Transmit(&hspi1, &c, 1, 50);
    lcd_cs_h();
}
static void lcd_data(uint8_t d)
{
    lcd_dc_h(); lcd_cs_l();
    HAL_SPI_Transmit(&hspi1, &d, 1, 50);
    lcd_cs_h();
}
static void lcd_data_buf(const uint8_t *b, uint16_t n)
{
    lcd_dc_h(); lcd_cs_l();
    HAL_SPI_Transmit(&hspi1, (uint8_t *)b, n, 500);
    lcd_cs_h();
}

bool lcd_init(void)
{
    /* === 阶段 1: 进入 lcd_init === 绿闪 1 次 */
    blink_g(1);

    /* === 阶段 2: SPI1 能发数据? === */
    uint8_t test = 0x00;
    HAL_StatusTypeDef rc = HAL_SPI_Transmit(&hspi1, &test, 1, 50);
    if (rc != HAL_OK) {
        /* SPI 坏了: 红闪 2 次, 尝试复位 */
        blink_r(2);
        HAL_SPI_DeInit(&hspi1);
        HAL_SPI_Init(&hspi1);
        /* NSSP 修复 (和 spi.c USER CODE 区一样) */
        hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
        HAL_SPI_Init(&hspi1);
        rc = HAL_SPI_Transmit(&hspi1, &test, 1, 50);
        if (rc != HAL_OK) {
            /* 彻底不通: 红闪 10 次 + 红灯常亮 */
            blink_r(10);
            led_r_on();
            return false;
        }
    }
    /* SPI OK: 绿闪 2 次 */
    blink_g(2);

    /* === 阶段 3: LCD 硬复位 === */
    lcd_cs_h();
    lcd_rst_h(); HAL_Delay(1);
    lcd_rst_l(); HAL_Delay(10);
    lcd_rst_h(); HAL_Delay(120);

    /* 绿闪 3 次 = 复位完成 */
    blink_g(3);

    /* === 阶段 4: 初始化序列 === */
    lcd_cmd(0x11); HAL_Delay(120);                /* Sleep Out */

    lcd_cmd(0xF0); lcd_data(0xC3);
    lcd_cmd(0xF0); lcd_data(0x96);
    lcd_cmd(0x36); lcd_data(0x48);                /* MADCTL: 竖屏先试 (厂家默认值) */
    lcd_cmd(0x3A); lcd_data(0x55);                /* RGB565 */
    lcd_cmd(0xB4); lcd_data(0x01);
    lcd_cmd(0xB7); lcd_data(0xC6);

    lcd_cmd(0xE8); {
        const uint8_t d[8] = {0x40,0x8A,0x00,0x00,0x29,0x19,0xA5,0x33};
        lcd_data_buf(d, 8);
    }
    lcd_cmd(0xC1); lcd_data(0x06);
    lcd_cmd(0xC2); lcd_data(0xA7);
    lcd_cmd(0xC5); lcd_data(0x18);

    lcd_cmd(0xE0); {
        const uint8_t d[14] = {0xF0,0x09,0x0B,0x06,0x04,0x15,0x2F,0x54,
                               0x42,0x3C,0x17,0x14,0x18,0x1B};
        lcd_data_buf(d, 14);
    }
    lcd_cmd(0xE1); {
        const uint8_t d[14] = {0xF0,0x09,0x0B,0x06,0x04,0x03,0x2D,0x43,
                               0x42,0x3B,0x16,0x14,0x17,0x1B};
        lcd_data_buf(d, 14);
    }

    lcd_cmd(0xF0); lcd_data(0x3C);
    lcd_cmd(0xF0); lcd_data(0x69);
    HAL_Delay(120);
    lcd_cmd(0x29);                                /* Display ON */
    HAL_Delay(20);

    /* 绿闪 4 次 = init 序列发完 */
    blink_g(4);

    /* === 阶段 5: 填色测试 === */
    /* 用竖屏尺寸 320×480 填红色 (MADCTL=0x48 是竖屏) */
    {
        uint8_t b[4];
        lcd_cmd(0x2A);
        b[0]=0; b[1]=0; b[2]=(uint8_t)(319>>8); b[3]=(uint8_t)(319);
        lcd_data_buf(b, 4);
        lcd_cmd(0x2B);
        b[0]=0; b[1]=0; b[2]=(uint8_t)(479>>8); b[3]=(uint8_t)(479);
        lcd_data_buf(b, 4);
        lcd_cmd(0x2C);

        /* 发红色像素 */
        uint8_t red[2] = {0xF8, 0x00};  /* RGB565 RED */
        lcd_dc_h(); lcd_cs_l();
        for (uint32_t i = 0; i < 320UL * 480; i++) {
            HAL_SPI_Transmit(&hspi1, red, 2, 10);
        }
        lcd_cs_h();
    }

    /* 绿闪 5 次 = 填色完成, 屏幕应该是红色 */
    blink_g(5);
    HAL_Delay(2000);  /* 停 2 秒让你看清楚 */

    return true;
}

void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint8_t b[4];
    lcd_cmd(0x2A);
    b[0]=(uint8_t)(x0>>8); b[1]=(uint8_t)x0; b[2]=(uint8_t)(x1>>8); b[3]=(uint8_t)x1;
    lcd_data_buf(b, 4);
    lcd_cmd(0x2B);
    b[0]=(uint8_t)(y0>>8); b[1]=(uint8_t)y0; b[2]=(uint8_t)(y1>>8); b[3]=(uint8_t)y1;
    lcd_data_buf(b, 4);
    lcd_cmd(0x2C);
}

void lcd_write_pixels(const uint8_t *buf, uint32_t n_bytes)
{
    lcd_dc_h(); lcd_cs_l();
    const uint32_t CHUNK = 32768U;
    uint32_t off = 0;
    while (off < n_bytes) {
        uint32_t k = (n_bytes - off) > CHUNK ? CHUNK : (n_bytes - off);
        HAL_SPI_Transmit(&hspi1, (uint8_t *)(buf + off), (uint16_t)k, 2000);
        off += k;
    }
    lcd_cs_h();
}

void lcd_fill(uint16_t color)
{
    lcd_set_window(0, 0, LCD_W - 1, LCD_H - 1);
    static uint8_t line[LCD_W * 2];
    for (uint16_t i = 0; i < LCD_W; ++i) {
        line[i * 2]     = (uint8_t)(color >> 8);
        line[i * 2 + 1] = (uint8_t)(color & 0xFF);
    }
    for (uint16_t y = 0; y < LCD_H; ++y) {
        lcd_write_pixels(line, sizeof(line));
    }
}

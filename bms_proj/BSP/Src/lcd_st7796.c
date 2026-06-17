/**
 * lcd_st7796.c — Sitronix ST7796S 3.5" 480×320 SPI 驱动
 *
 * 接线: SCK=PA5, MOSI=PA7, CS=PB1, DC=PB0, RST=PB2, LED=3V3(常亮)
 * SPI1 Mode3 (CPOL=1 CPHA=1, 与厂家 demo 一致), 10.6MHz
 * NSSP=DISABLE, IRQ=DISABLE (polling 模式)
 *
 * 初始化序列照搬 ST7796_TN_Code.txt (鸿讯电子 CL35BC1017-40A)
 * MADCTL=0xE8: 横屏 480×320 (MV+MX+MY+BGR)
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

static bool s_spi_ok = true;

static void lcd_cmd(uint8_t c)
{
    lcd_dc_l(); lcd_cs_l();
    if (HAL_SPI_Transmit(&hspi1, &c, 1, 50) != HAL_OK) s_spi_ok = false;
    lcd_cs_h();
}

static void lcd_data(uint8_t d)
{
    lcd_dc_h(); lcd_cs_l();
    if (HAL_SPI_Transmit(&hspi1, &d, 1, 50) != HAL_OK) s_spi_ok = false;
    lcd_cs_h();
}

static void lcd_data_buf(const uint8_t *b, uint16_t n)
{
    lcd_dc_h(); lcd_cs_l();
    if (HAL_SPI_Transmit(&hspi1, (uint8_t *)b, n, 500) != HAL_OK) s_spi_ok = false;
    lcd_cs_h();
}

bool lcd_init(void)
{
    s_spi_ok = true;

    /* 硬复位 (与厂家 demo 完全一致的时序) */
    lcd_cs_h();
    lcd_rst_h(); HAL_Delay(1);
    lcd_rst_l(); HAL_Delay(10);
    lcd_rst_h(); HAL_Delay(120);
    HAL_Delay(120);   /* 厂家 demo 在复位后有额外 120ms */

    /* 初始化序列 (ST7796 厂家 TN code) */
    lcd_cmd(0x11); HAL_Delay(120);                /* Sleep Out */

    lcd_cmd(0xF0); lcd_data(0xC3);                /* Command Set Control 1 */
    lcd_cmd(0xF0); lcd_data(0x96);                /* Command Set Control 2 */
    lcd_cmd(0x36); lcd_data(0xE8);                /* MADCTL: 横屏 480×320 */
    lcd_cmd(0x3A); lcd_data(0x55);                /* COLMOD: RGB565 */
    lcd_cmd(0xB4); lcd_data(0x01);                /* Inversion */
    lcd_cmd(0xB7); lcd_data(0xC6);                /* Entry Mode */

    lcd_cmd(0xE8); {
        const uint8_t d[8] = {0x40,0x8A,0x00,0x00,0x29,0x19,0xA5,0x33};
        lcd_data_buf(d, 8);
    }
    lcd_cmd(0xC1); lcd_data(0x06);                /* Power Ctrl 2 */
    lcd_cmd(0xC2); lcd_data(0xA7);                /* Power Ctrl 3 */
    lcd_cmd(0xC5); lcd_data(0x18);                /* VCOM Ctrl */

    lcd_cmd(0xE0); {                              /* Gamma + */
        const uint8_t d[14] = {0xF0,0x09,0x0B,0x06,0x04,0x15,0x2F,0x54,
                               0x42,0x3C,0x17,0x14,0x18,0x1B};
        lcd_data_buf(d, 14);
    }
    lcd_cmd(0xE1); {                              /* Gamma - */
        const uint8_t d[14] = {0xF0,0x09,0x0B,0x06,0x04,0x03,0x2D,0x43,
                               0x42,0x3B,0x16,0x14,0x17,0x1B};
        lcd_data_buf(d, 14);
    }

    lcd_cmd(0xF0); lcd_data(0x3C);                /* Lock cmd set 1 */
    lcd_cmd(0xF0); lcd_data(0x69);                /* Lock cmd set 2 */
    HAL_Delay(120);
    lcd_cmd(0x29);                                /* Display ON */
    HAL_Delay(20);
    return s_spi_ok;
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

#include "lcd_st7789.h"
#include "bsp.h"
#include "stm32g4xx_hal.h"

extern SPI_HandleTypeDef hspi1;     // PA5/PA6/PA7

static inline void lcd_cs_l(void)  { HAL_GPIO_WritePin(LCD_CS_PORT,  LCD_CS_PIN,  GPIO_PIN_RESET); }
static inline void lcd_cs_h(void)  { HAL_GPIO_WritePin(LCD_CS_PORT,  LCD_CS_PIN,  GPIO_PIN_SET);   }
static inline void lcd_dc_l(void)  { HAL_GPIO_WritePin(LCD_DC_PORT,  LCD_DC_PIN,  GPIO_PIN_RESET); }
static inline void lcd_dc_h(void)  { HAL_GPIO_WritePin(LCD_DC_PORT,  LCD_DC_PIN,  GPIO_PIN_SET);   }
static inline void lcd_rst_l(void) { HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_RESET); }
static inline void lcd_rst_h(void) { HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_SET);   }

static void lcd_cmd(uint8_t c)
{
    lcd_dc_l();
    lcd_cs_l();
    HAL_SPI_Transmit(&hspi1, &c, 1, 20);
    lcd_cs_h();
}
static void lcd_data(uint8_t d)
{
    lcd_dc_h();
    lcd_cs_l();
    HAL_SPI_Transmit(&hspi1, &d, 1, 20);
    lcd_cs_h();
}
static void lcd_data_buf(const uint8_t *b, uint16_t n)
{
    lcd_dc_h();
    lcd_cs_l();
    HAL_SPI_Transmit(&hspi1, (uint8_t*)b, n, 200);
    lcd_cs_h();
}

bool lcd_init(void)
{
    lcd_cs_h();
    lcd_rst_l(); HAL_Delay(20);
    lcd_rst_h(); HAL_Delay(120);

    lcd_cmd(0x11);            // sleep out
    HAL_Delay(120);
    lcd_cmd(0x36); lcd_data(0x00);   // MADCTL
    lcd_cmd(0x3A); lcd_data(0x05);   // 16-bit RGB565
    lcd_cmd(0x21);            // inversion on（ST7789 IPS 屏典型）
    lcd_cmd(0x13);            // normal mode
    lcd_cmd(0x29);            // display on
    HAL_Delay(20);
    return true;
}

void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    lcd_cmd(0x2A);
    uint8_t b[4] = { (uint8_t)(x0>>8), (uint8_t)x0, (uint8_t)(x1>>8), (uint8_t)x1 };
    lcd_data_buf(b, 4);
    lcd_cmd(0x2B);
    b[0]=(uint8_t)(y0>>8); b[1]=(uint8_t)y0; b[2]=(uint8_t)(y1>>8); b[3]=(uint8_t)y1;
    lcd_data_buf(b, 4);
    lcd_cmd(0x2C);
}

/* 大块写像素: 一次性 SPI 传 RAMWR 之后的数据 (DC=H, CS 在整段保持低) */
void lcd_write_pixels(const uint8_t *buf, uint32_t n_bytes)
{
    lcd_dc_h();
    lcd_cs_l();
    /* HAL_SPI_Transmit Size 为 uint16_t, 大块时分段 */
    const uint32_t CHUNK = 32768U;
    uint32_t off = 0;
    while (off < n_bytes) {
        uint32_t k = (n_bytes - off) > CHUNK ? CHUNK : (n_bytes - off);
        HAL_SPI_Transmit(&hspi1, (uint8_t *)(buf + off), (uint16_t)k, 1000);
        off += k;
    }
    lcd_cs_h();
}

void lcd_fill(uint16_t color)
{
    lcd_set_window(0, 0, LCD_W-1, LCD_H-1);
    uint8_t line[LCD_W * 2];
    for (uint16_t i = 0; i < LCD_W; ++i) {
        line[i*2]   = (uint8_t)(color >> 8);
        line[i*2+1] = (uint8_t)(color & 0xFF);
    }
    for (uint16_t y = 0; y < LCD_H; ++y) {
        lcd_data_buf(line, sizeof(line));
    }
}

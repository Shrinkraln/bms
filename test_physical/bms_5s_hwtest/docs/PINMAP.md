# 引脚映射 PINMAP（按原理图核对）

> 主控：STM32G474RET。所有引脚来自您的 `Schematic1_2026-05-19.pdf` 上的 G474RET 主控排母网络名。

| 功能 | MCU 引脚 | 网络名 | 方向 | 备注 |
|---|---|---|---|---|
| **调试串口 UART2** | PA2 | PA2_UART_DBG_TX | OUT-AF | CH340G TXD ← MCU RXD 接 |
|  | PA3 | PA3_UART_DBG_RX | IN-AF  | |
| **CAN FDCAN1** | PA11 | PA11_CAN_RX | IN-AF | 经 TJA1042 |
|  | PA12 | PA12_CAN_TX | OUT-AF | |
| **I2C1**（多个从机） | PB6 | I2C_SCL | OD-AF | 默认 G474 I2C1 SCL |
|  | PB7 | I2C_SDA | OD-AF | |
| **LCD SPI1** | PA5 | PA5_LCD_SCK  | OUT-AF | |
|  | PA6 | PA6_LCD_MISO | IN-AF  | |
|  | PA7 | PA7_LCD_MOSI | OUT-AF | |
|  | PB0 | PB0_LCD_DC   | OUT | data/cmd |
|  | PB1 | PB1_LCD_CS   | OUT | |
|  | PB2 | PB2_LCD_RST  | OUT | |
| **触摸 SPI2** | PB13 | PB13_TOUCH_SCK   | OUT-AF | |
|  | PB14 | PB14_TOUCH_MISO  | IN-AF  | |
|  | PB15 | PB15_TOUCH_MOSI  | OUT-AF | |
|  | PA0  | PA0_TOUCH_CS     | OUT | |
|  | PA8  | PA8_TOUCH_INT    | IN  | 中断输入 |
| **DAC8552 SPI3** | PA4 | PA4_DAC_SCLK | OUT-AF | |
|  | PA1 | PA1_DAC_DIN  | OUT-AF | MOSI only |
|  | PB10 | PB10_DAC_CS  | OUT | SYNC# |
| **BQ76920 ALERT** | PB3 | PB3_BQ76_ALERT_IN | IN | 中断输入 |
| **BQ34Z100 VEN** | PB9 | PB9_BQ34_VEN | OUT | 高有效 |
| **LED 绿** | PB4 | PB4_LED_GREEN | OUT | 高电平点亮 |
| **LED 红** | PB5 | PB5_LED_RED   | OUT | 高电平点亮 |
| **蜂鸣器** | PB8 | PB8_BUZZER_CTRL | OUT | 有源蜂鸣器，2.4kHz |
| **按键 KEY1** | PB12 | PB12_KEY1 | IN-PU | 按下=低 |

## ⚠️ 需要您再核对的两点

1. **LED/蜂鸣的"高有效/低有效"**：本程序按"高电平点亮/响"实现。原理图上 LED1 与 GND 之间没有看到限流电阻在 MCU 侧，您看一下三极管/驱动电路再决定。蜂鸣器走的是 Q3 (S8050) NPN 驱动，**高电平时三极管导通蜂鸣器响**，所以 PB8=高响，没问题。
2. **I2C1 引脚**：G474 的 I2C1 默认 PB6/PB7（AF4）。如果您在 CubeMX 选了 PA15/PA14 之类的备用映射，请在 `MX_I2C1_Init()` 里改成相同的。

## SPI 工作模式建议

| SPI | 用途 | CPOL | CPHA | 速率 |
|---|---|---|---|---|
| SPI1 | ST7789 LCD | 0 | 0 | ≤ 20 MHz |
| SPI2 | XPT2046 触摸 | 0 | 0 | ≤ 2 MHz |
| SPI3 | DAC8552 | 0 | 1 | ≤ 30 MHz |

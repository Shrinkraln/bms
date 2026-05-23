# BMS 5S 硬件验证最小程序 (STM32G474RET)

> 目标：用最小代码量、一次烧录，串口/LED/蜂鸣器联合输出，把板子上 **每一颗芯片、每一路通信、每一个外设** 都"摸一遍"，输出 PASS/FAIL 报告。

## 一、覆盖范围（按原理图）

| 测试项 | 验证方式 | 通过判据 |
|---|---|---|
| MCU 自身 (STM32G474) | 时钟、SysTick、Flash 启动 | 串口能输出 banner |
| LED 绿 PB4 / LED 红 PB5 | 各闪 2 次 | 目视 + 串口提示 |
| 蜂鸣器 PB8 | 短鸣 100ms | 听感 + 串口提示 |
| 按键 PB12 | 等待 3s 内按下 | 检测到下降沿 |
| UART2 (CH340G PA2/PA3) | 上电打印 banner | 上位机串口能看到 |
| **I2C1 总线扫描** | 0x08~0x77 扫一遍 | 至少看到 BQ76920/BQ34/INA226/TMP117 |
| **BQ76920** (0x08) | 读 SYS_STAT、ADCGAIN/ADCOFFSET、5节VCx ADC | 5路电压 2.5~4.3V |
| **BQ34Z100-G1** (0x55) | VEN 拉高后读 DeviceType (0x01) | =0x0100 |
| **INA226** (0x40) | 读 Manufacturer ID (0xFE) | =0x5449 |
| **TMP117** (0x48) | 读 Device ID (0x0F) | =0x0117 |
| **NTC** (BQ76920 TS1) | 读温度 ADC | 在合理范围 |
| **DAC8552** SPI3 | 写 0V 和半量程 | 测量 DAC_OUT_A/B 是否变化（外测） |
| **LCD SPI1** | 写 ST7789 初始化 + 填充色块 | 目视显示 |
| **CAN FDCAN1** | 自环回模式发 1 帧收 1 帧 | 内部回环成功 |

## 二、判定流程

1. 上电 → 红绿 LED 各亮 200ms → 蜂鸣 100ms（"我活着"）
2. UART 打印 banner，开始逐项测试
3. 每一项打印 `[ OK ]` 或 `[FAIL]`，FAIL 时红灯快闪
4. 全部 OK → 绿灯常亮 + 蜂鸣 3 声
5. 任意 FAIL → 红灯常亮 + 蜂鸣长鸣 1 次，串口列出失败项

## 三、文件结构

```
Core/
├── Inc/
│   ├── main.h
│   ├── bsp.h          # 板级抽象：LED/按键/蜂鸣/延时
│   ├── i2c_bus.h      # I2C1 阻塞读写
│   ├── bq76920.h      # BQ76920 测试
│   ├── bq34z100.h     # BQ34Z100 测试
│   ├── ina226.h       # INA226 测试
│   ├── tmp117.h       # TMP117 测试
│   ├── dac8552.h      # DAC8552 测试
│   ├── lcd_st7789.h   # LCD 测试
│   └── can_test.h     # CAN 自环测试
└── Src/
    └── (对应 .c 文件)
```

详见 `docs/PINMAP.md` 和 `docs/I2C_ADDR.md`。

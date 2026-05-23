# 使用指南：如何在嘉立创/Keil/STM32CubeIDE 中跑起来

## 1. 用 STM32CubeMX 生成基础工程

打开 CubeMX，选 **STM32G474RETx**。

### Clock
- 配 PLL 到 170 MHz（HSI 内部 16M ×... 或 HSE 8M）。
- 启用 SysTick @ 1 ms。

### Peripherals（按 PINMAP.md 勾选）
| 外设 | Mode | 参数 |
|---|---|---|
| USART2 | Async | 115200 8N1 |
| I2C1 | Standard | 100 kHz；Pull-up 可以片外 4.7k |
| SPI1 | Master 8-bit | CPOL=0 CPHA=0 |
| SPI3 | Master TX-only 8-bit | CPOL=0 CPHA=1 (DAC8552 标准) |
| FDCAN1 | Classic CAN | 500 kbps，**先选 Normal**，loopback 由代码切换 |
| GPIO | 详见 PINMAP.md | LED/按键/CS 等按表配 |

> SPI2（触摸）这一版不在自检范围内，您后续需要的话可以补一个 `xpt2046_test()`。

### 注意
- **不要勾 NVIC 里 I2C/SPI 的中断**，本测试程序全部走阻塞模式，简单可靠。
- USART2 也不要开 DMA。

生成工程时选 "Generate peripheral initialization as a pair of '.c/.h' files"。

## 2. 替换/合并源文件

把本仓库 `Core/Inc/*.h`、`Core/Src/*.c`（除 main.c 外）整体复制到 CubeMX 生成的工程同名目录下。

`main.c` 有两种合并方式（任选）：
- **方式 A（推荐）**：直接用本仓库 `Core/Src/main.c` 替换 CubeMX 生成的 main.c，然后把 CubeMX 在原文件里生成的 `SystemClock_Config()`、`MX_xxx_Init()` 等函数粘贴到本 main.c 的尾部（CubeMX 工具区段以外）。
- **方式 B**：保留 CubeMX 的 main.c 骨架，把本仓库 main.c 里的 `t_*` 测试函数和 `int main()` 中 USER CODE 段的内容剪贴进去。

## 3. 烧录与观察

1. 拨码上电（注意 B+ 接 5 节电池组或可调电源 18~21V）。
2. 上位机打开 CH340G 对应的 COM 口，**115200 8N1**。
3. 上电应当看到：
   ```
   =========================================
     BMS 5S Hardware Self-Test  v1.0
     Build: May 19 2026 12:00:00
   =========================================
     [ OK ] LED/BUZZ            visual+audible check, see/hear it?
     >> Press KEY1 within 3s to confirm key...
     [ OK ] KEY1 PB12           pressed
     [ OK ] BQ76920 5S          5 cells in 1.5~4.5V
     [ OK ] BQ34Z100            DeviceType=0x0100 (want 0x0100)
     [ OK ] I2C scan            n=4: 08 40 48 55
     [ OK ] INA226              MFG=0x5449 DIE=0x2260 Vbus=20100mV
     [ OK ] TMP117              ID=0x0117 T=25.42C
     [ OK ] DAC8552             SPI3 wrote 0/half/full; measure DAC_OUT_A/B
     [ OK ] LCD SPI1            RGBW flashed; visual confirm
     [ OK ] FDCAN1 LB           internal loopback

   ----- SUMMARY -----
     PASS: 10   FAIL: 0   TOTAL: 10
     *** ALL PASS, board OK ***
   ```

## 4. 测哪一项失败了怎么办

| 失败项 | 排查方向 |
|---|---|
| LED/BUZZ 看不到 | PB4/PB5/PB8 焊接、Q3 三极管、电源 5V |
| KEY1 timeout | PB12 焊接、按键 SW1 焊接、上拉是否正确 |
| BQ76920 init fail | I2C 上拉电阻、B+ 是否接电池组、SCL/SDA 短路、型号是否带 CRC |
| BQ76920 cells out of range | 单体均衡线焊接 / R1~R6 100Ω 是否漏焊 / 电池差压过大 |
| BQ34Z100 fail | PB9 (VEN) 是否拉高、SRP/SRN 焊接、Rsns 10mΩ 焊接 |
| I2C scan 只 1~2 个 | 总体 I2C 上拉电阻 / 总线短路 |
| INA226 / TMP117 fail | 该芯片自身焊接、地址跳线 |
| DAC8552 fail | 万用表测 DAC_OUT_A/B 跟 PB10/PA4/PA1 波形 |
| LCD fail | 屏幕排线、PB0/1/2 焊接、SPI1 是否选对 AF |
| FDCAN1 LB fail | 时钟设置（FDCAN_CLK 必须配好）、PA11/PA12 复用功能 |

## 5. 后续扩展点

- 加 SPI2 触摸 XPT2046 测试
- 加 PACK 端 MOSFET (Q1/Q2 AOD508) 的导通测试（小心带载！）
- 加 USB-C CC 检测
- 加 CAN 总线对外通信测试（接 CANoe / 另一块板）

祝调机顺利 🎉

# BMS-5S 元件级详细连线总表 (BOM + Net List)

> 本文档为 `BMS_Hardware_Design.md` 的补充配件，提供**元件位号 → 具体型号 → 封装 → 引脚级连接**的完整对照。  
> 适用于直接用此表绘制原理图（KiCad / 立创EDA / Altium）。

---

## 目录

1. [元件位号总表（BOM）](#1-元件位号总表bom)
2. [核心芯片连线明细](#2-核心芯片连线明细)
3. [电池采样网络（VC0-VC5 + BAT）](#3-电池采样网络vc0-vc5--bat)
4. [电流采样回路（4 端开尔文）](#4-电流采样回路4-端开尔文)
5. [均衡电阻网络](#5-均衡电阻网络)
6. [充放电 MOS 驱动回路](#6-充放电-mos-驱动回路)
7. [BQ34Z100-G1 电压翻译电路](#7-bq34z100-g1-电压翻译电路)
8. [电源链](#8-电源链)
9. [I²C 总线](#9-ic-总线)
10. [外设连线（CAN / LCD / 按键 / LED / 蜂鸣器 / NTC）](#10-外设连线can--lcd--按键--led--蜂鸣器--ntc)
11. [L610 4G 模组连线](#11-l610-4g-模组连线)
12. [完整网络表（按网络名分组）](#12-完整网络表按网络名分组)

---

## 1. 元件位号总表（BOM）

### 1.1 主控与核心芯片

| 位号 | 元件 | 型号 | 封装 | 数量 | 备注 |
|---|---|---|---|---|---|
| U1 | 主控 MCU | STM32G474RET6 最小系统板 | LQFP-64 (板载) | 1 | 170MHz, 512KB Flash, 128KB RAM |
| U2 | AFE | **BQ7692006PWR** | TSSOP-20 | 1 | 3.3V LDO + CRC + 0x18 地址 |
| U3 | 电量计 | **BQ34Z100PWR-G1** | TSSOP-14 | 1 | Impedance Track, 0x55 地址 |
| U4 | CAN 收发器 | **TJA1042T/3** | SOIC-8 | 1 | 3.3V 兼容版（替代 5V 的 TJA1050） |
| U5 | 4G 模组 | 广和通 L610 成品扩展板 | 模组 | 1 | ADP-L610-Arduino 类，自带 SIM 卡座 |
| U6 | DC-DC #1（4G专用） | **MP1584EN** | SOIC-8 | 1 | 输出 4.0V/2A，给 L610 |
| U7 | DC-DC #2（系统） | **MP1584EN** | SOIC-8 | 1 | 输出 5.0V/1A |
| U8 | LDO | **AMS1117-3.3** | SOT-223 | 1 | 5V→3.3V/1A，给 STM32+LCD |
| U9 | 三极管（蜂鸣器驱动） | **S8050** | SOT-23 | 1 | NPN, 0.5A |
| U10 | TFT LCD 屏 | **1.8寸 ST7735 SPI 屏** | FPC | 1 | 128×160 分辨率 |
| U11 | USB 转 TTL | **CH340G 模块** | 模块 | 1 | 调试用，外置 |

### 1.2 功率器件

| 位号 | 元件 | 型号 | 封装 | 数量 | 备注 |
|---|---|---|---|---|---|
| Q1, Q2 | 充放电 NMOS | **AOD508** 或 **SQJ422EP** | DPAK / TO-252 | 2 | 40V/30A，Rds(on)<8mΩ |
| Q3 | 蜂鸣器驱动 NPN | S8050 | SOT-23 | 1 | 见 U9 |
| BZ1 | 蜂鸣器 | 有源蜂鸣器 3.3V | 直插 9×11mm | 1 | 一体式 |

### 1.3 保护器件

| 位号 | 元件 | 型号 | 封装 | 数量 | 备注 |
|---|---|---|---|---|---|
| F1 | 自恢复保险丝 | **MF-R300** (3A) | 直插 | 1 | 电池主回路 |
| D1 | TVS 二极管 | **SMBJ24A** | SMB | 1 | 电池端口防浪涌 |
| D2 | TVS 二极管 | SMBJ24A | SMB | 1 | CAN 总线保护 |
| D3 | 续流二极管 | 1N4148 | DO-35 | 1 | 蜂鸣器并联 |

### 1.4 电阻

| 位号 | 阻值 | 封装 | 精度 | 功率 | 数量 | 用途 |
|---|---|---|---|---|---|---|
| **Rsns** | **10 mΩ** | **2512** | **1%** | **2W** | 1 | 电流采样电阻（关键！） |
| R1~R6 | 100 Ω | 0805 | 1% | 1/8W | 6 | BQ76920 VC0~VC5 串联（共 6 个：BAT, VC5, VC4, VC3, VC2, VC1, VC0 中 6 节点 RC 网络的串联电阻；VC0-Cell1 共用一个） |
| R7 | 100 Ω | 0805 | 1% | 1/8W | 1 | BQ76920 REGSRC 滤波 |
| R8, R9 | 100 Ω | 0805 | 1% | 1/8W | 2 | BQ76920 SRP/SRN 串联滤波 |
| R10, R11 | 100 Ω | 0805 | 1% | 1/8W | 2 | BQ34Z100 SRP/SRN 串联滤波 |
| R12~R16 | 100 Ω, 1W | **2010** | 5% | 1W | 5 | 5 节均衡电阻（外置） |
| R17, R18 | 4.7 kΩ | 0805 | 5% | 1/8W | 2 | I²C 上拉（SCL + SDA） |
| R19, R20 | 10 kΩ | 0805 | 5% | 1/8W | 2 | CHG/DSG 栅源下拉电阻 |
| R21 | 1 MΩ | 0805 | 5% | 1/8W | 1 | ALERT 引脚下拉 |
| R22 | 1.5 MΩ | 0805 | **1%** | 1/8W | 1 | BQ34Z100 BAT 分压上臂 |
| R23 | 68 kΩ | 0805 | **1%** | 1/8W | 1 | BQ34Z100 BAT 分压下臂 |
| R24~R26 | 10 kΩ | 0805 | 5% | 1/8W | 3 | NTC1/2/3 上拉（NTC2/3 上拉 + 不用 TS 时下拉） |
| R27~R30 | 10 kΩ | 0805 | 5% | 1/8W | 4 | 按键上拉 |
| R31, R32 | 510 Ω | 0805 | 5% | 1/8W | 2 | LED 限流 |
| R33 | 1 kΩ | 0805 | 5% | 1/8W | 1 | 蜂鸣器三极管基极 |
| R34, R35 | 120 Ω | 0805 | 1% | 1/4W | 2 | CAN 终端电阻 |
| R36 | 100 Ω | 0805 | 5% | 1/8W | 1 | LCD 背光限流 |

### 1.5 电容

| 位号 | 容值 | 封装/类型 | 耐压 | 数量 | 用途 |
|---|---|---|---|---|---|
| C1~C6 | 1 µF | 0805 X7R | 25V | 6 | BQ76920 VC 节点旁路（VC0~VC5+BAT 共 6 个节点，相邻节点间电容） |
| C7 | 10 µF | 0805 X5R | 25V | 1 | BQ76920 REGSRC 滤波（Cf） |
| C8 | 1 µF | 0805 X7R | 16V | 1 | BQ76920 CAP1 (必需，内部电荷泵) |
| C9 | 1 µF | 0805 X7R | 16V | 1 | BQ76920 REGOUT 退耦 |
| C10 | 100 nF | 0603 X7R | 25V | 1 | SRP-SRN 跨接差分电容 (BQ76920) |
| C11 | 100 nF | 0603 X7R | 25V | 1 | SRP-SRN 跨接差分电容 (BQ34Z100) |
| C12 | 100 nF | 0603 X7R | 16V | 1 | BQ34Z100 REGIN 退耦 |
| C13 | 1 µF | 0805 X7R | 16V | 1 | BQ34Z100 REG25 退耦（必需） |
| C14 | 100 nF | 0603 X7R | 25V | 1 | BQ34Z100 BAT 分压点滤波 |
| C15~C20 | 100 nF | 0603 X7R | 16V | 6 | STM32 各 VDD 引脚退耦 |
| C21 | 1 µF | 0805 X7R | 16V | 1 | STM32 VBAT 退耦 |
| C22 | 22 pF | 0603 NPO | 50V | 2 | HSE 8MHz 晶振负载电容 |
| C23 | 4.7 µF | 0805 X5R | 16V | 1 | AMS1117-3.3 输入 |
| C24 | 22 µF | 1210 X5R | 16V | 1 | AMS1117-3.3 输出 |
| C25 | 22 µF | 1210 X5R | 25V | 1 | MP1584-1 (4V) 输入 |
| C26 | 22 µF | 1210 X5R | 16V | 1 | MP1584-1 (4V) 输出 |
| C27 | 22 µF | 1210 X5R | 25V | 1 | MP1584-2 (5V) 输入 |
| C28 | 22 µF | 1210 X5R | 16V | 1 | MP1584-2 (5V) 输出 |
| C29 | 100 µF | 直插电解 | 25V | 1 | L610 VBAT 大电容（突发） |
| C30 | 10 µF | 0805 X5R | 16V | 1 | L610 VBAT 中等电容 |
| C31 | 100 nF | 0603 X7R | 16V | 1 | L610 VBAT 高频退耦 |
| C32~C35 | 100 nF | 0603 X7R | 16V | 4 | 按键去抖电容 |

### 1.6 其它

| 位号 | 元件 | 型号 | 数量 | 备注 |
|---|---|---|---|---|
| Y1 | 晶振 | HSE 8MHz | 1 | 最小系统板自带 |
| LED1 | LED 绿色 | 0805 | 1 | 正常运行指示 |
| LED2 | LED 红色 | 0805 | 1 | 告警指示 |
| SW1~SW4 | 按键 | 6×6 mm 轻触 | 4 | 模式 / 确认 / 上 / 下 |
| NTC1 | 热敏电阻 | **103AT-2** (10kΩ B=3435) | 1 | 接 BQ76920 TS1 |
| NTC2 | 热敏电阻 | 103AT-2 | 1 | 接 STM32 ADC PA0 |
| NTC3 | 热敏电阻 | 103AT-2 | 1 | 接 STM32 ADC PA1 / BQ34Z100 TS |
| J1 | 接线端子 | KF128 2P 5.08mm | 1 | PACK+ / PACK- |
| J2 | 接线端子 | KF128 6P 2.54mm | 1 | 5 串电池 + 平衡线接口 (B-, C1, C2, C3, C4, B+) |
| J3 | 接线端子 | KF128 4P 2.54mm | 1 | CAN + 调试串口 |
| J4 | 排针 | 2.54mm 4P | 1 | SWD 调试接口 |
| J5 | 排针 | 2.54mm 6P | 1 | L610 模组接口 |

---

## 2. 核心芯片连线明细

### 2.1 STM32G474RET6 (U1)

> 假设最小系统板对外引出全部 64 个引脚（典型情况）。下表只列**项目实际使用的引脚**。

| 引脚号 | 引脚名 | 网络名 | 连接到 | 经过元件 |
|---|---|---|---|---|
| - | VBAT | 3V3 | 系统 3.3V | 板载 |
| - | VDD (×4) | 3V3 | 系统 3.3V | 各 100nF (C15~C18) |
| - | VDDA | 3V3 | 系统 3.3V | C19 = 100nF + C20 = 1µF |
| - | VSS, VSSA | GND | 系统地 | 直接 |
| 14 | PA0 | NTC2_AIN | NTC2 分压点 | 见 §10.6 |
| 15 | PA1 | NTC3_AIN | NTC3 分压点 | 见 §10.6 |
| 16 | PA2 | UART_DBG_TX | U11 (CH340) RXD | 直接 |
| 17 | PA3 | UART_DBG_RX | U11 (CH340) TXD | 直接 |
| 20 | PA4 | L610_PWRKEY | U5 J5-PWRKEY | 直接 |
| 21 | PA5 | LCD_SCK | U10 SCK | 直接 |
| 22 | PA6 | L610_RESET | U5 J5-RESET_N | 直接 |
| 23 | PA7 | LCD_MOSI | U10 SDA | 直接 |
| 41 | PA8 | L610_NETSTA | U5 J5-NET_STATUS | 直接 |
| 42 | PA9 | L610_RXD_IN | U5 J5-RXD（模组的RX，MCU送数据）| 直接（成品板已含电平转换） |
| 43 | PA10 | L610_TXD_OUT | U5 J5-TXD（模组的TX，MCU收数据）| 直接 |
| 44 | PA11 | CAN_RX | U4 (TJA1042) Pin 4 RXD | 直接 |
| 45 | PA12 | CAN_TX | U4 (TJA1042) Pin 1 TXD | 直接 |
| 46 | PA13 | SWDIO | J4-SWDIO | 直接 |
| 49 | PA14 | SWCLK | J4-SWCLK | 直接 |
| 26 | PB0 | LCD_DC | U10 DC | 直接 |
| 27 | PB1 | LCD_CS | U10 CS | 直接 |
| 28 | PB2 | LCD_RST | U10 RST | 直接 |
| 55 | PB3 | BQ_ALERT | U2 Pin 20 (ALERT) | R21=1MΩ 下拉到 GND |
| 56 | PB4 | LED_GREEN | LED1 阳极 | R31=510Ω |
| 57 | PB5 | LED_RED | LED2 阳极 | R32=510Ω |
| 58 | PB6 | I2C1_SCL | U2 Pin 5 + U3 Pin 13 | R17=4.7kΩ 上拉到 3V3 |
| 59 | PB7 | I2C1_SDA | U2 Pin 4 + U3 Pin 14 | R18=4.7kΩ 上拉到 3V3 |
| 61 | PB8 | BUZZER_CTRL | Q3 (S8050) Base | R33=1kΩ |
| 62 | PB9 | BQ34_VEN | U3 Pin 2 VEN | 直接（可悬空） |
| 33 | PB12 | KEY1 | SW1 一端 | R27=10kΩ 上拉, C32=100nF 去抖 |
| 34 | PB13 | KEY2 | SW2 一端 | R28=10kΩ 上拉, C33=100nF 去抖 |
| 35 | PB14 | KEY3 | SW3 一端 | R29=10kΩ 上拉, C34=100nF 去抖 |
| 36 | PB15 | KEY4 | SW4 一端 | R30=10kΩ 上拉, C35=100nF 去抖 |
| 7 | NRST | NRST | 板载复位按钮 | 板载 100nF |
| 5, 6 | PD0/PD1 (OSC_IN/OUT) | HSE | Y1 8MHz 晶振 | C22a/b=22pF（板载） |

### 2.2 BQ7692006PWR (U2) — TSSOP-20

| 引脚 | 引脚名 | 网络名 | 连接到 | 经过元件 |
|---|---|---|---|---|
| 1 | DSG | DSG_GATE | Q1 (DSG_FET) Gate | R19=10kΩ 下拉到 PACK_GND |
| 2 | CHG | CHG_GATE | Q2 (CHG_FET) Gate | R20=10kΩ 下拉到 PACK_GND |
| 3 | VSS | BAT_NEG (即 VC0 节点) | 电池组 B- | 直接（注意：VSS 与采样电阻**电池侧**等电位）|
| 4 | SDA | I2C1_SDA | STM32 PB7 | 共用 4.7kΩ 上拉 (R18) |
| 5 | SCL | I2C1_SCL | STM32 PB6 | 共用 4.7kΩ 上拉 (R17) |
| 6 | TS1 | NTC1_AIN | NTC1 一端 | R24=10kΩ（不用时下拉到 VSS） |
| 7 | CAP1 | CAP1_NET | C8=1µF 到 VSS | 必需，内部电荷泵 |
| 8 | REGOUT | BQ_3V3 | I²C 上拉电源（R17/R18 上端） | C9=1µF 到 VSS |
| 9 | REGSRC | REGSRC_NET | 电池 B+ 经滤波 | R7=100Ω + C7=10µF |
| 10 | BAT | VC_BAT | Cell5 正极 = B+ | R6=100Ω + C6=1µF（到 VC5 节点） |
| 11 | NC | — | 悬空 | — |
| 12 | VC5 | VC5_NET | Cell5 负极 = Cell4 正极 | R5=100Ω + C5=1µF（到 VC4 节点） |
| 13 | VC4 | VC4_NET | Cell4 负极 = Cell3 正极 | R4=100Ω + C4=1µF（到 VC3 节点） |
| 14 | VC3 | VC3_NET | Cell3 负极 = Cell2 正极 | R3=100Ω + C3=1µF（到 VC2 节点） |
| 15 | VC2 | VC2_NET | Cell2 负极 = Cell1 正极 | R2=100Ω + C2=1µF（到 VC1 节点） |
| 16 | VC1 | VC1_NET | Cell1 负极 = B- | R1=100Ω + C1=1µF（到 VC0 节点） |
| 17 | VC0 | BAT_NEG | 电池组 B- | 与 Pin 3 VSS 同网络 |
| 18 | SRP | SRP_NET | 采样电阻 Rsns 电池侧 | R8=100Ω；C10=100nF 跨接到 SRN |
| 19 | SRN | SRN_NET | 采样电阻 Rsns PACK 侧 | R9=100Ω；C10=100nF 跨接到 SRP |
| 20 | ALERT | BQ_ALERT | STM32 PB3 | R21=1MΩ 下拉到 VSS |

**关键说明**：

- 第 3 脚 VSS 和第 17 脚 VC0 同电位（都接电池组 B-），但 datasheet 要求**分开布线**：VSS 走芯片地（数字地），VC0 走采样信号路径，最后在采样电阻**电池侧**汇合
- 第 8 脚 REGOUT 输出 3.3V，**仅供 I²C 上拉电阻和 BQ76920 自身使用**，不要给 STM32 供电
- 第 18/19 脚 SRP/SRN 必须用差分对走线（开尔文连接，详见 §4）

### 2.3 BQ34Z100PWR-G1 (U3) — TSSOP-14

| 引脚 | 引脚名 | 网络名 | 连接到 | 经过元件 |
|---|---|---|---|---|
| 1 | P2 | GND | 不用 LED2 | 直接接 GND |
| 2 | VEN | BQ34_VEN | STM32 PB9 或 悬空 | （可选省电控制） |
| 3 | P1 | GND | 不用 LED1 | 直接接 GND |
| 4 | BAT | BQ34_BAT_AIN | 电池 B+ 经分压 | R22=1.5MΩ + R23=68kΩ + C14=100nF（详见 §7） |
| 5 | CE | 3V3 | 系统 3.3V（常使能） | 直接 |
| 6 | REGIN | 3V3 | 系统 3.3V | C12=100nF 退耦 |
| 7 | REG25 | REG25_NET | 仅滤波，不外接 | C13=1µF 到 VSS（必需） |
| 8 | VSS | GND | 系统数字地 | 直接 |
| 9 | SRP | SRP_BQ34 | 采样电阻 Rsns 电池侧 | R10=100Ω；C11=100nF 跨接到 SRN |
| 10 | SRN | SRN_BQ34 | 采样电阻 Rsns PACK 侧 | R11=100Ω；C11=100nF 跨接到 SRP |
| 11 | P6/TS | NTC3_BQ34 | NTC3 一端 | NTC 内部已 10kΩ 上拉到 REG25，外部不需要 |
| 12 | P5/HDQ | GND | 不用 HDQ | 直接下拉到 GND（或悬空）|
| 13 | P4/SCL | I2C1_SCL | STM32 PB6 | 共用 4.7kΩ 上拉 (R17) |
| 14 | P3/SDA | I2C1_SDA | STM32 PB7 | 共用 4.7kΩ 上拉 (R18) |

**重要**：

- BQ34Z100-G1 BAT 引脚最大允许电压 **5V**（datasheet 6.1 节绝对最大额定值），21V 电池电压**必须经分压**才能接！
- BQ34Z100-G1 的 SRP/SRN 极性约定：**SRP 靠近 BAT-（电池侧）**，SRN 靠近 PACK-——和 BQ76920 极性约定一致
- 两颗芯片可以**用同一颗 Rsns**，但 SRP/SRN 信号要**各自走差分对**到对应芯片，不能在一处汇合后再分叉

---

## 3. 电池采样网络（VC0-VC5 + BAT）

### 3.1 拓扑（datasheet 9.2 节标准拓扑）

```
 电池组拓扑（5S1P）：
 
    B+ ──┬───┬─[R6=100Ω]─►U2/Pin10 BAT  网络名: VC_BAT
         │   │
         │  Cell5
         │   │
    Cell4+ = Cell5- ──┬─[R5=100Ω]─►U2/Pin12 VC5  网络名: VC5_NET
                      │
                     Cell4
                      │
    Cell3+ = Cell4- ──┬─[R4=100Ω]─►U2/Pin13 VC4  网络名: VC4_NET
                      │
                     Cell3
                      │
    Cell2+ = Cell3- ──┬─[R3=100Ω]─►U2/Pin14 VC3  网络名: VC3_NET
                      │
                     Cell2
                      │
    Cell1+ = Cell2- ──┬─[R2=100Ω]─►U2/Pin15 VC2  网络名: VC2_NET
                      │
                     Cell1
                      │
    B- = Cell1-     ──┬─[R1=100Ω]─►U2/Pin16 VC1  网络名: VC1_NET
                      │           
                      └────[直连]──►U2/Pin17 VC0  网络名: BAT_NEG
                                  └►U2/Pin3  VSS  
                      
    （注意：上面的 R1~R6 是 BQ76920 输入串联电阻）
```

**电容连接（关键，旁路到下一节而不是直接对地）**：

| 电容位号 | 容值 | 一端连到 | 另一端连到 |
|---|---|---|---|
| **C1** | 1µF/25V | VC1_NET（R1 BQ侧）| BAT_NEG (VC0/VSS) |
| **C2** | 1µF/25V | VC2_NET（R2 BQ侧）| VC1_NET（R1 BQ侧）|
| **C3** | 1µF/25V | VC3_NET（R3 BQ侧）| VC2_NET（R2 BQ侧）|
| **C4** | 1µF/25V | VC4_NET（R4 BQ侧）| VC3_NET（R3 BQ侧）|
| **C5** | 1µF/25V | VC5_NET（R5 BQ侧）| VC4_NET（R4 BQ侧）|
| **C6** | 1µF/25V | VC_BAT（R6 BQ侧） | VC5_NET（R5 BQ侧）|

**为什么是这种连接方式？**

datasheet 要求每个 Cc 跨接在**相邻两个 VC 引脚之间**（不是单端到地），这样形成差分滤波。如果对地接，因为电池电压叠加，C1 承受 4.2V，C5 要承受 21V，元件应力差异巨大。差分接法每个电容只承受单节电池电压（约 4.2V），统一选 25V/1µF 即可。

### 3.2 物理布局建议

- R1~R6 **靠近 BQ76920** 摆放（不是靠近电池端子），目的是让长引线（电池到 PCB 端子）位于 RC 之前，吸收 ESD/瞬态
- C1~C6 **紧贴 R1~R6 的 BQ 侧**，差分电容尽量平行 BQ 引脚走线
- VC0~VC5 节点**禁止穿过开关电源、CAN、SPI 等高速信号区域**

---

## 4. 电流采样回路（4 端开尔文）

### 4.1 拓扑

```
    电池 B-（VSS / VC0 节点）
          │
          ├─── 大电流路径 ────┬─── Q1 (DSG) ─── Q2 (CHG) ────┐
          │                  │                              │
          ▼                  ▼                              ▼
     [开尔文电流端 A]      [开尔文电流端 B]             PACK- 端子
          │                  │
     ┌────┴──────────────────┴────┐
     │                            │
     │  Rsns 10mΩ / 2W (2512)     │
     │                            │
     ├────┐                  ┌────┤
     │    │                  │    │
     │   [开尔文传感端 A]    │   [开尔文传感端 B]
     │    │                  │    │
     │  R8=100Ω           R9=100Ω │
     │    │                  │    │
     │    ├──C10=100nF──────┤     │
     │    │   (差分跨接)     │     │
     │    │                  │    │
     │    ▼                  ▼    │
     │  U2/Pin18 SRP   U2/Pin19 SRN
     │                            │
     │  R10=100Ω          R11=100Ω│
     │    │                  │    │
     │    ├──C11=100nF──────┤     │
     │    │                  │    │
     │    ▼                  ▼    │
     │  U3/Pin9 SRP    U3/Pin10 SRN
     └────────────────────────────┘
```

### 4.2 关键说明

1. **采样电阻 Rsns 必须使用 4 端开尔文连接**：
   - 大电流走电池 B- → 开尔文电流端 A → Rsns 体 → 开尔文电流端 B → MOS 管
   - 小信号走开尔文传感端 A → R8/R10 → SRP；开尔文传感端 B → R9/R11 → SRN
   - 开尔文电流端 ≠ 开尔文传感端！**不可在 PCB 上把 SRP/SRN 直接连到电流路径的铜皮上**

2. **R8/R9 (BQ76920) 与 R10/R11 (BQ34Z100) 的 100Ω 必须各自独立**：不能两颗芯片共用一对 100Ω。原因：每颗芯片内部输入有 ~2.5MΩ 电阻，并联会改变滤波特性

3. **C10 / C11 是差分电容**：必须**跨接**在 SRP-SRN 之间，不能各自对地

4. **SRP 永远在电池侧**（datasheet 6.1.1：SRP "nearest VSS"），**SRN 永远在 PACK- 侧**。极性反了会让电流方向反转

### 4.3 PCB 布局要求

| 项目 | 要求 |
|---|---|
| Rsns 封装 | 2512（散热好），或 4527 / 5930 等更大开尔文专用封装 |
| 大电流铜厚 | ≥ 2 oz（**70µm**），过孔阵列加固 |
| SRP/SRN 走线 | 差分对，等长，宽度 0.2mm，间距 0.2mm |
| 100Ω 电阻位置 | 紧贴 BQ 芯片，远离 Rsns |
| 100nF 差分电容位置 | 紧贴 BQ 芯片 SRP/SRN 引脚 |

---

## 5. 均衡电阻网络

BQ76920 内部带均衡 FET，但需要外部均衡电阻泄放电流。

### 5.1 连接方式

5 节电池每节都需要一颗外部均衡电阻 R12~R16，**跨接在该节电池正负端之间，但实际电流路径通过 BQ76920 内部 FET**：

```
   每节电池 Celln：
   
   Celln+  ───── Rn=100Ω/1W (R12~R16) ───── Celln-
                                                │
   该节对应的 VCn 引脚已经通过 RC 连入 BQ76920
   
   说明：BQ76920 内部 CB FET 在 VCn 和 VCn-1 之间，
   开启时把 R12~R16 串入电池正负端，泄放电流约 50mA
```

| 位号 | 阻值 | 功率 | 跨接电池 | 一端 | 另一端 |
|---|---|---|---|---|---|
| R12 | 100Ω 1W | 2010 封装 | Cell1 | VC1_NET（R1 之前的电池侧）| VC0_NET（B-）|
| R13 | 100Ω 1W | 2010 封装 | Cell2 | VC2_NET | VC1_NET |
| R14 | 100Ω 1W | 2010 封装 | Cell3 | VC3_NET | VC2_NET |
| R15 | 100Ω 1W | 2010 封装 | Cell4 | VC4_NET | VC3_NET |
| R16 | 100Ω 1W | 2010 封装 | Cell5 | VC_BAT | VC4_NET |

**功率校验**：4.2V × 50mA = 210mW，2010 封装 1W 余量 4.7×

### 5.2 重要约束

- 均衡电阻接在**电池侧**（R1~R6 RC 网络的电池端，而不是 BQ 端）。如果接在 BQ 端，均衡电流会经过 100Ω 输入电阻产生 5V 压降，使 BQ 测量错误
- BQ76920 硬件限制：**相邻电池不能同时均衡**（CELLBAL 寄存器会自动避免），所以无需在硬件上做互锁

---

## 6. 充放电 MOS 驱动回路

### 6.1 低边 NMOS 拓扑（共源极）

```
                                     PACK+ (J1-1) 
                                       │
   B+ ───────────────────────────────┴──── 直通

   B- (BAT_NEG / VSS)
        │
        ├──── 大电流路径 ────┬──── Rsns 10mΩ ────┬─── Q1 Drain
        │                                     │
        │                                  Q1 (DSG_FET)
        │                                  AOD508
        │                                     │ Source
        │                                     │
        │                                  Q2 Drain
        │                                  Q2 (CHG_FET)
        │                                  AOD508
        │                                     │ Source
        │                                     │
        │                                     ├──── PACK- (J1-2)
        │                                     │
                                    
   栅极驱动：
   U2/Pin1 DSG ────[直接]────► Q1 Gate（同时 R19=10kΩ 下拉到 Q1 Source）
   U2/Pin2 CHG ────[直接]────► Q2 Gate（同时 R20=10kΩ 下拉到 Q2 Source）
```

**注意 Q1 Source ≠ Q2 Source**：

- Q1 (DSG) 的 Source 是 Q1-Q2 中间节点
- Q2 (CHG) 的 Source 是 PACK- 节点
- R19 下拉到 Q1 Source（即 Q1-Q2 中间节点）
- R20 下拉到 Q2 Source（即 PACK-）

实际等效：当 BQ76920 关断时，CHG/DSG 引脚通过内部弱下拉（CHG=1MΩ, DSG=2.5kΩ）到 VSS（=BAT_NEG），R19/R20 提供额外的栅源短路确保 MOS 关断。

### 6.2 MOS 选型详细参数

**AOD508 (DPAK 封装) 推荐参数**：

| 参数 | AOD508 |
|---|---|
| 封装 | DPAK / TO-252 |
| Vds | 40V |
| Vgs(th) | 1.5~2.5V |
| Vgs(max) | ±20V |
| Id 持续 | 30A @ 25°C |
| Rds(on) @ Vgs=10V | 8 mΩ (typ) |
| Qg @ Vgs=10V | 17 nC |

**为什么不用 BOM 单上的 AO3400？**

BOM 单 AO3400 的 Id = 5.7A（SOT-23 封装），对 5A 持续放电余量太小。如果只做演示用 1~2A 负载，AO3400 够用；但如果要演示 5A 以上场景或预留 10A 突发，**强烈建议升级到 AOD508/SQM50P03 等 DPAK 封装**。

### 6.3 BQ76920 输出特性匹配（datasheet 7.5 节）

| 参数 | 值 | 说明 |
|---|---|---|
| VFETON (REGSRC≥12V) | 12V (typ) | CHG/DSG 开启电压（5S 电池 18.5V 时 REGSRC=18V，符合条件）|
| tFET_ON | 250µs | 上升时间（10nF 等效负载）|
| tDSG_OFF | 90µs | DSG 关断下降时间 |
| RCHG_OFF | 1MΩ | CHG 弱下拉到 VSS |
| RDSG_OFF | 2.5kΩ | DSG 弱下拉到 VSS |

**MOS 选型校验**：

- AOD508 Qg ≈ 17nC，BQ76920 充电电流 ≈ Qg/tFET_ON = 17nC/250µs = 68µA，**远低于** BQ76920 输出能力（~mA 级别）
- AOD508 Vgs(th) max = 2.5V，BQ76920 输出 12V，**充分驱动**

### 6.4 保险丝与 TVS

```
   B+ (电池正极) ───── F1 (MF-R300, 3A 自恢复) ────┬───── 系统主电源轨
                                                  │
                                                  ├──── D1 (SMBJ24A) ──── B- (TVS 限压)
                                                  │
                                                  └──── 主路径
   
   PACK+ ── D2 (SMBJ24A) ── PACK-（直接保护负载端口）
```

---

## 7. BQ34Z100-G1 电压翻译电路

### 7.1 分压网络

```
   电池 B+ (12.5V ~ 21V)
        │
        ▼
        ├──── R22 = 1.5MΩ (1%, 0805) ────┬──── U3/Pin4 BAT
        │                                │
        │                                ├──── R23 = 68kΩ (1%, 0805)
        │                                │       │
        │                                │       ▼
        │                                │      GND
        │                                │
        │                                ├──── C14 = 100nF (0603)
        │                                │       │
        │                                │       ▼
        │                                │      GND
        │
   计算：
     满电时 (21V)：U3/BAT = 21 × 68/(1500+68) = 911mV
     标称时 (18.5V)：U3/BAT = 18.5 × 68/1568 = 802mV
     最低时 (12.5V)：U3/BAT = 12.5 × 68/1568 = 542mV
   
   全部在 BQ34Z100 BAT 引脚的允许范围内（VA2: VSS-0.125V 到 5V）
```

### 7.2 软件 Pack Configuration 配置

需要把分压比写入 BQ34Z100 数据闪存：

```
Subclass 64 - Configuration:
  Pack Configuration (Offset 0x0A): Bit VOLTSEL = 1 (使用外部分压)
  
Subclass 80 - System Data:
  Voltage Divider (Offset 0x14): 
    标定值 = 21000 / 911 × 1000 = 23047
    （表示输入 1mV 等于实际 23.047mV）
```

### 7.3 关于 VEN 引脚的可选省电控制

如果要节省 13.4µA 的常态漏电（年漏 117mAh），可以加一颗 PMOS 在分压网络上方：

```
   电池 B+ ────┬──── PMOS Source
              │     │
              │     PMOS Drain ─── R22 ─── U3/Pin4 BAT
              │     │
              │     PMOS Gate ─── 经下拉电阻 ── BQ34_VEN (U3/Pin2)
              │
              （或 STM32 PB9 控制 VEN）
```

**对于本项目（功能演示），建议省略 PMOS，VEN 悬空，分压电路常通**。BQ34Z100-G1 自带 SLEEP/FULLSLEEP 模式（30µA），整体待机电流可控。

---

## 8. 电源链

### 8.1 电源树详细网络图

```
   [电池组 B+ 12.5V~21V] ─── F1 自恢复保险丝 ──┬── BAT_PROTECTED 网络
                                              │
                                              ├──► U6 (MP1584 #1)
                                              │     输入：BAT_PROTECTED
                                              │     输出：4V0 (4.0V/2A)
                                              │     反馈电阻：上臂 R_fb1=68kΩ, 下臂 R_fb2=20kΩ
                                              │            (Vout = 0.8 × (1+R1/R2) = 3.52V → 调整为 4V 用 R1=80k/R2=20k)
                                              │     输入电容：C25=22µF/25V
                                              │     输出电容：C26=22µF/16V + C30=10µF + C29=100µF/16V
                                              │     ↓
                                              │  [4V0 网络] ──► U5 L610 VBAT
                                              │
                                              ├──► U7 (MP1584 #2)
                                              │     输入：BAT_PROTECTED
                                              │     输出：5V0 (5.0V/1A)
                                              │     反馈电阻：上臂 R=105kΩ, 下臂 R=20kΩ
                                              │     输入电容：C27=22µF/25V
                                              │     输出电容：C28=22µF/16V
                                              │     ↓
                                              │  [5V0 网络] ──► U8 (AMS1117-3.3)
                                              │                  ↓
                                              │              [3V3 网络] ──► STM32, LCD, 上拉电阻, BQ34Z100 REGIN, NTC上拉
                                              │
                                              ├──► R7=100Ω + C7=10µF ──► U2 REGSRC
                                              │                              ↓
                                              │                          U2 内部 LDO ──► U2 REGOUT (BQ_3V3)
                                              │                                              ↓
                                              │                                          仅供 I²C 上拉(R17/R18) + U2 内部
                                              │
                                              ├──► R6=100Ω + C6=1µF (差分) ──► U2/Pin10 BAT
                                              │
                                              └──► R22=1.5MΩ 分压 ──► U3/Pin4 BAT
```

### 8.2 MP1584 反馈电阻精确计算

MP1584 反馈参考电压 0.8V，设输出 V_OUT，反馈下臂 R_FB2，上臂 R_FB1：

```
V_OUT = 0.8 × (1 + R_FB1 / R_FB2)
```

**4V 输出**：选 R_FB2 = 20kΩ，R_FB1 = (4/0.8 - 1) × 20k = 80kΩ → 取 E96 标准值 **80.6kΩ**  
实际输出 = 0.8 × (1 + 80.6/20) = 4.024V ✓

**5V 输出**：选 R_FB2 = 20kΩ，R_FB1 = (5/0.8 - 1) × 20k = 105kΩ → 取 E96 标准值 **105kΩ** 或 100kΩ  
实际输出 (R_FB1=105k) = 0.8 × (1 + 105/20) = 5.0V ✓

**MP1584 还需要的外围元件（按 datasheet）**：

| 元件 | 值 | 封装 | 备注 |
|---|---|---|---|
| L1, L2 (电感) | 22µH | DR74-22µH 或类似 5×5×4mm | 饱和电流 ≥ 2A |
| C_BS (自举电容) | 10nF | 0603 | 引脚 BS 到 SW |
| R_BS（启动） | 0Ω | 0603 | 自举电阻（可短路） |
| D1 (续流) | SS34（肖特基） | SMA | 内部已集成，外部可省 |
| R_EN | 100kΩ + 10kΩ 分压 | 0603 | EN 引脚使能 |
| C_COMP | 22pF | 0603 | 频率补偿（GND 引脚） |

### 8.3 接地网络划分

```
   网络名             连接关系
   ──────────────────────────────────────────────────────
   PACK_GND          PACK- 端子（外部负载侧）
   BAT_NEG           电池组 B- = VSS = VC0
   AGND              模拟地（BQ76920 VSS, BQ34Z100 VSS, NTC 网络）
   DGND              数字地（STM32, LCD, L610, 蜂鸣器）
   
   连接策略：
   - PACK_GND 通过 Q1 Source（DSG MOS 关断时与 BAT_NEG 隔离）
   - AGND = BAT_NEG（直接合并）
   - DGND = AGND，在 Rsns 电池侧单点连接（避免大电流串入）
```

---

## 9. I²C 总线

### 9.1 总线拓扑

```
   STM32/PB6 ──┬───────────────────────────┬─── U2/Pin5 (SCL)
               │                           │
               R17 = 4.7kΩ                 └─── U3/Pin13 (SCL)
               │
               U2/Pin8 REGOUT (BQ_3V3 = 3.3V)
                       │
   STM32/PB7 ──┬───────┴───────────────────┬─── U2/Pin4 (SDA)
               │                           │
               R18 = 4.7kΩ                 └─── U3/Pin14 (SDA)
```

**注意**：

- 上拉电阻**只放一对**（R17/R18），并联到 BQ76920 REGOUT（BQ_3V3）
- 不能用系统 3V3 上拉（虽然电平相同，但 BQ76920 上电时序可能让 REGOUT 早于 3V3，确保两颗从机不会出现电平冲突）
- 上拉到 BQ_3V3 还有一个好处：BQ76920 进入 SHIP 模式时 REGOUT 关断，I²C 自然失效（防止 SHIP 模式下 STM32 误读寄存器）

### 9.2 I²C 地址

| 芯片 | 7-bit 地址 | 8-bit 写地址 | 8-bit 读地址 |
|---|---|---|---|
| BQ7692006 (U2) | 0x18 | 0x30 | 0x31 |
| BQ34Z100-G1 (U3) | 0x55 | 0xAA | 0xAB |

**软件读法**：

```c
// HAL 库
HAL_I2C_Master_Transmit(&hi2c1, 0x18 << 1, ...); // 写 BQ76920
HAL_I2C_Master_Receive (&hi2c1, (0x18 << 1) | 1, ...); // 读 BQ76920
HAL_I2C_Master_Transmit(&hi2c1, 0x55 << 1, ...); // 写 BQ34Z100
```

---

## 10. 外设连线（CAN / LCD / 按键 / LED / 蜂鸣器 / NTC）

### 10.1 CAN 总线（U4 = TJA1042T/3）

| TJA1042 引脚 | 网络名 | 连接到 | 备注 |
|---|---|---|---|
| 1 TXD | CAN_TX | STM32 PA12 | 直接 |
| 2 GND | DGND | 系统地 | 直接 |
| 3 VCC | 3V3 | 系统 3.3V | C=100nF 退耦 |
| 4 RXD | CAN_RX | STM32 PA11 | 直接 |
| 5 SPLIT | (悬空) | 不用分裂端口 | 通过 4.7kΩ 接 GND（可选） |
| 6 CANL | CANL | J3 端子 | 经 D2 (SMBJ24A) 保护 |
| 7 CANH | CANH | J3 端子 | — |
| 8 STB | DGND | 接地 = 高速模式 | 直接（如果需要待机控制可接 GPIO）|

**终端电阻**：R34=120Ω, 跨接 CANH-CANL（如果 BMS 是 CAN 总线终端节点，否则不接）

### 10.2 TFT LCD（U10 = ST7735 1.8寸）

| LCD 引脚 | 网络名 | 连接到 |
|---|---|---|
| GND | DGND | 系统地 |
| VCC | 3V3 | 系统 3.3V |
| SCL/SCK | LCD_SCK | STM32 PA5 |
| SDA/MOSI | LCD_MOSI | STM32 PA7 |
| RES/RST | LCD_RST | STM32 PB2 |
| DC | LCD_DC | STM32 PB0 |
| CS | LCD_CS | STM32 PB1 |
| BLK/LED | LCD_BLK | 经 R36=100Ω 到 3V3 |

### 10.3 按键

```
   3V3 ────── R27=10kΩ ───┬─── STM32 PB12 (KEY1)
                          │
                          ├─── C32=100nF ── DGND（去抖）
                          │
                          └─── SW1 一端
                          
   SW1 另一端 ─── DGND
   
   （SW2-SW4 同理，对应 R28-R30, C33-C35, PB13-PB15）
```

### 10.4 LED 指示

```
   STM32 PB4 ─── R31=510Ω ─── LED1 阳极
   LED1 阴极 ─── DGND
   （LED1 = 绿色 0805 LED，正常运行心跳，1Hz 闪烁）
   
   STM32 PB5 ─── R32=510Ω ─── LED2 阳极
   LED2 阴极 ─── DGND
   （LED2 = 红色 0805 LED，告警常亮）
```

### 10.5 蜂鸣器

```
   STM32 PB8 ─── R33=1kΩ ─── Q3 (S8050) Base
   3V3 (或 5V0) ─── BZ1 (蜂鸣器) (+) 端
   BZ1 (-) 端 ─── Q3 Collector
   Q3 Emitter ─── DGND
   
   续流二极管 D3 (1N4148)：
     D3 阳极 ─── Q3 Collector
     D3 阴极 ─── 蜂鸣器 (+) 端 (=3V3)
   （续流保护，吸收电感性反向电压）
```

### 10.6 NTC 温度传感器

```
   NTC1 (BQ76920 内置 ADC):
   U2/Pin6 TS1 ─── NTC1 一端
   NTC1 另一端 ─── BAT_NEG (= U2 VSS)
   （BQ76920 内部已有 10kΩ 上拉到 REGOUT，外部不需上拉）
   
   NTC2 (STM32 ADC1_IN1):
   3V3 ─── R24=10kΩ ─── ┬─── STM32 PA0
                       │
                       └─── NTC2 一端
   NTC2 另一端 ─── DGND
   
   NTC3 (BQ34Z100 内置 ADC):
   U3/Pin11 P6/TS ─── NTC3 一端
   NTC3 另一端 ─── DGND
   （BQ34Z100 内部已有 10kΩ 上拉到 REG25，外部不需上拉）
```

**实际放置位置建议**：

- NTC1：电池组中央（最高温位置）
- NTC2：MOS 管旁边（监测发热）
- NTC3：电池组边缘（温度梯度比较）

---

## 11. L610 4G 模组连线

### 11.1 模组接口（J5 = 6Pin 排针，与成品板对接）

```
   J5-1 (VBAT)        ◄──── 4V0 网络（来自 U6 MP1584 #1）
                            └─ C29 = 100µF/25V (电解电容，紧靠模组)
                            └─ C30 = 10µF/16V
                            └─ C31 = 100nF/16V
                            
   J5-2 (GND)         ◄──── DGND
   
   J5-3 (TXD - 模组输出) ──► STM32 PA10 (USART1_RX)
                            （成品板已含 1.8V↔3.3V 电平转换，直接接）
                            
   J5-4 (RXD - 模组输入) ◄── STM32 PA9  (USART1_TX)
   
   J5-5 (PWRKEY)      ◄──── STM32 PA4
                            软件控制：拉低 ≥2s 触发开机
                            
   J5-6 (RESET_N)     ◄──── STM32 PA6
                            软件控制：拉低 100ms 复位
                            
   (可选) J5-7 (NET_STATUS) ──► STM32 PA8 (EXTI8)
                            网络状态指示信号
```

### 11.2 SIM 卡（成品板自带）

成品 ADP-L610 类扩展板自带 6PIN SIM 卡座，物联卡直接插入即可。如果要自己设计裸模组贴板，需要按 ETSI TS 102.221 标准设计 SIM 卡座 + ESD 防护，工作量很大，**强烈建议用成品扩展板**。

### 11.3 4G 天线

成品扩展板带 IPEX/UFL 天线接口，按 BOM 单选 **IPEX 全频段贴片天线**（或鞭状天线，有更好的接收性能但较大）。

---

## 12. 完整网络表（按网络名分组）

按网络名整理，便于绘原理图时对照（一个网络名可以连多个引脚）。

### 12.1 电源网络

| 网络名 | 电压 | 连接的引脚 |
|---|---|---|
| **BAT_RAW** | 12.5V~21V | 电池组 B+, F1 一端 |
| **BAT_PROTECTED** | 12.5V~21V | F1 另一端, U6 IN, U7 IN, R7 一端, R6 一端, R22 一端, D1 阴极 |
| **4V0** | 4.0V | U6 OUT, C26 (+), C29 (+), C30 (+), C31 (+), J5-1 (L610 VBAT) |
| **5V0** | 5.0V | U7 OUT, C28 (+), U8 IN, C23 (+) |
| **3V3** | 3.3V | U8 OUT, C24 (+), STM32 VDD/VDDA, U3 REGIN/CE, U4 VCC, U10 VCC, R17/R18 上端（可选）, R24-R30 上端, R36 一端 |
| **BQ_3V3** | 3.3V | U2 REGOUT (Pin 8), C9 (+), R17/R18 上端 |
| **REG25_NET** | 2.5V | U3 REG25 (Pin 7), C13 (+) [仅供 BQ34Z100 内部] |
| **REGSRC_NET** | 12.5V~21V | U2 REGSRC (Pin 9), R7 另一端, C7 (+) |

### 12.2 电池采样网络

| 网络名 | 电池节点 | 连接的引脚 |
|---|---|---|
| **VC_BAT** | Cell5+ = B+ | R6 BQ侧, U2/Pin10 BAT, C6 一端 |
| **VC5_NET** | Cell5- = Cell4+ | R5 BQ侧, U2/Pin12 VC5, C5 一端, C6 另一端, R16 一端 |
| **VC4_NET** | Cell4- = Cell3+ | R4 BQ侧, U2/Pin13 VC4, C4 一端, C5 另一端, R15 一端, R16 另一端 |
| **VC3_NET** | Cell3- = Cell2+ | R3 BQ侧, U2/Pin14 VC3, C3 一端, C4 另一端, R14 一端, R15 另一端 |
| **VC2_NET** | Cell2- = Cell1+ | R2 BQ侧, U2/Pin15 VC2, C2 一端, C3 另一端, R13 一端, R14 另一端 |
| **VC1_NET** | Cell1- = B- | R1 BQ侧, U2/Pin16 VC1, C1 一端, C2 另一端, R12 一端, R13 另一端 |
| **BAT_NEG (=GND)** | B- = VC0 | U2/Pin3 VSS, U2/Pin17 VC0, C1 另一端, R12 另一端, Rsns 电池侧, AGND |

### 12.3 电流采样网络

| 网络名 | 连接的引脚 |
|---|---|
| **SRP_NET** (BQ76920) | Rsns 电池侧 → R8 一端 → U2/Pin18 SRP, C10 一端 |
| **SRN_NET** (BQ76920) | Rsns PACK侧 → R9 一端 → U2/Pin19 SRN, C10 另一端 |
| **SRP_BQ34** | Rsns 电池侧 → R10 一端 → U3/Pin9 SRP, C11 一端 |
| **SRN_BQ34** | Rsns PACK侧 → R11 一端 → U3/Pin10 SRN, C11 另一端 |
| **PACK_NEG** | Rsns PACK侧, Q1 Drain |

### 12.4 通信总线网络

| 网络名 | 连接的引脚 |
|---|---|
| **I2C1_SCL** | STM32 PB6, U2/Pin5 SCL, U3/Pin13 P4/SCL, R17 一端 |
| **I2C1_SDA** | STM32 PB7, U2/Pin4 SDA, U3/Pin14 P3/SDA, R18 一端 |
| **CAN_TX** | STM32 PA12, U4/Pin1 TXD |
| **CAN_RX** | STM32 PA11, U4/Pin4 RXD |
| **CANH** | U4/Pin7 CANH, J3-1, R34 一端, D2 一端 |
| **CANL** | U4/Pin6 CANL, J3-2, R34 另一端, D2 另一端 |
| **UART_DBG_TX** | STM32 PA2, J3-3 (调试接口) |
| **UART_DBG_RX** | STM32 PA3, J3-4 |
| **L610_TXD_OUT** | STM32 PA10, J5-3 (L610 模组 TXD) |
| **L610_RXD_IN** | STM32 PA9, J5-4 (L610 模组 RXD) |

### 12.5 控制信号网络

| 网络名 | 连接的引脚 |
|---|---|
| **DSG_GATE** | U2/Pin1 DSG, Q1 Gate, R19 一端 |
| **CHG_GATE** | U2/Pin2 CHG, Q2 Gate, R20 一端 |
| **BQ_ALERT** | U2/Pin20 ALERT, STM32 PB3, R21 一端 |
| **L610_PWRKEY** | STM32 PA4, J5-5 |
| **L610_RESET** | STM32 PA6, J5-6 |
| **L610_NETSTA** | STM32 PA8, J5-7 |
| **BQ34_VEN** | U3/Pin2 VEN, STM32 PB9 (可悬空) |
| **BQ34_BAT_AIN** | U3/Pin4 BAT, R22 BQ侧, R23 一端, C14 一端 |
| **NTC1_AIN** | U2/Pin6 TS1, NTC1, R24（不用时）|
| **NTC2_AIN** | STM32 PA0, NTC2, R25 |
| **NTC3_AIN** | U3/Pin11 P6/TS, NTC3 |
| **CAP1_NET** | U2/Pin7 CAP1, C8 (+) |

### 12.6 SPI / GPIO 网络

| 网络名 | 连接的引脚 |
|---|---|
| **LCD_SCK** | STM32 PA5, U10 SCK |
| **LCD_MOSI** | STM32 PA7, U10 SDA/MOSI |
| **LCD_CS** | STM32 PB1, U10 CS |
| **LCD_DC** | STM32 PB0, U10 DC |
| **LCD_RST** | STM32 PB2, U10 RST |
| **LCD_BLK** | U10 BLK, R36 一端 |
| **LED_GREEN** | STM32 PB4, R31 一端 |
| **LED_RED** | STM32 PB5, R32 一端 |
| **BUZZER_CTRL** | STM32 PB8, R33 一端 |
| **KEY1** | STM32 PB12, R27 一端, SW1, C32 一端 |
| **KEY2** | STM32 PB13, R28 一端, SW2, C33 一端 |
| **KEY3** | STM32 PB14, R29 一端, SW3, C34 一端 |
| **KEY4** | STM32 PB15, R30 一端, SW4, C35 一端 |
| **SWDIO** | STM32 PA13, J4-2 |
| **SWCLK** | STM32 PA14, J4-3 |

---

## 附录 A：物料采购建议

| 元件 | 立创商城型号示例 | 备选 |
|---|---|---|
| BQ7692006PWR | C100856 | 嘉立创 BQ7692003PWR (C2865456) |
| BQ34Z100PWR-G1 | C146595 | — |
| TJA1042T/3 | C95334 | SN65HVD230D (C12343) |
| AOD508 | C144568 | SQM50P03 (C155060) |
| MP1584EN | C29630 | TPS54331 (C9852) |
| AMS1117-3.3 | C6186 | RT9013-33GB |
| CH340G 模块 | 通用 | FT232 模块 |
| 1.8寸 ST7735 | 通用 | 2.4寸 ILI9341 (BOM 单备选) |
| L610 成品板 | ADP-L610-Arduino | EC600S/EC200S 替代 (引脚不同) |
| Rsns 10mΩ 2W | C29316 (LR2512-R010F) | TLR3A2WLR010FTE (C145820) |

## 附录 B：本表与上一份《BMS_Hardware_Design.md》的关系

- **BMS_Hardware_Design.md**：宏观设计文档，关注架构、原理、avoid 雷区
- **本表（BMS_Wiring_BOM.md）**：元件级实施清单，关注每颗元件的 footprint、位号、连接关系

绘原理图时对照本表，PCB 布局时对照 BMS_Hardware_Design.md 的 §11.5 PCB 布局核心约束。

---

> **作者：Claude (Anthropic) — BMS-5S Wiring BOM List — 2026-04-25**  
> **校验依据**：TI BQ76920 datasheet (SLUSBK2I rev I), TI BQ34Z100-G1 datasheet (SLUSBZ5D rev D), MP1584 datasheet, ST STM32G474xx datasheet (DS12288)

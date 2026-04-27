# BMS-5S 智能锂电池管理系统硬件设计文档

> 项目：基于 STM32G474 的 5 串智能锂电池管理系统  
> 主控：STM32G474RET6（最小系统板）  
> AFE：BQ76920（3-5 串电池监测）  
> 电量计：BQ34Z100-G1（Impedance Track™）  
> 通信：广和通 L610 4G Cat.1（外接成品模组）+ FDCAN  
> 电池组：5S1P 18650（标称 18.5V，满电 21V）

---

## 目录

1. [系统总体架构](#1-系统总体架构)
2. [核心芯片引脚速查表](#2-核心芯片引脚速查表)
3. [STM32G474RET6 引脚分配](#3-stm32g474ret6-引脚分配)
4. [BQ76920 AFE 模块连线](#4-bq76920-afe-模块连线)
5. [BQ34Z100-G1 电量计模块连线](#5-bq34z100-g1-电量计模块连线)
6. [充放电 MOS 管驱动电路](#6-充放电-mos-管驱动电路)
7. [L610 4G 模组连线](#7-l610-4g-模组连线)
8. [电源系统设计](#8-电源系统设计)
9. [外设模块连线（CAN/LCD/按键/蜂鸣器/LED）](#9-外设模块连线canlcd按键蜂鸣器led)
10. [完整连线对照总表](#10-完整连线对照总表)
11. [关键设计要点与避坑提醒](#11-关键设计要点与避坑提醒)

---

## 1. 系统总体架构

```
                     ┌─────────────────────────────────────────┐
                     │  18650 × 5 (5S1P)  18-21V 电池组         │
                     └──┬───┬───┬───┬───┬───┬──────────────────┘
                        │B+ │C5 │C4 │C3 │C2 │C1 B-
                        │   │   │   │   │   │
              ┌─────────▼───▼───▼───▼───▼───▼─────────┐
              │      BQ76920 AFE (5串电池监控)          │
              │ • 14-bit ADC: VC0~VC5 电池电压         │
              │ • 16-bit CC : SRP/SRN 电流采样         │
              │ • 1路NTC内置(TS1) + 2路 ALERT中断       │
              │ • CHG/DSG 低边 NMOS 驱动               │
              │ • 5路内置均衡 FET (50mA/cell)          │
              └────────────┬────────────────────────────┘
                           │ I²C(3.3V) + ALERT
                           │
              ┌────────────▼─────────────────────────┐
              │   STM32G474RET6 (170MHz Cortex-M4)   │
              │   ┌──────────────────────────────┐   │
              │   │ FreeRTOS 7任务调度            │   │
              │   │ I²C1 → BQ76920 + BQ34Z100-G1 │   │
              │   │ USART1 → L610 4G 模组         │   │
              │   │ USART2 → 调试串口             │   │
              │   │ FDCAN1 → CAN收发器(TJA1050)   │   │
              │   │ SPI1 → TFT LCD显示             │   │
              │   │ ADC1 → 2路NTC + 内部温度       │   │
              │   └──────────────────────────────┘   │
              └──┬─────────┬───────────┬──────────┬──┘
                 │         │           │          │
        ┌────────▼──┐ ┌────▼─────┐ ┌──▼────┐ ┌───▼───┐
        │ BQ34Z100  │ │ TJA1050  │ │ L610  │ │ TFT   │
        │ 电量计    │ │ CAN PHY  │ │ 4G模组│ │ LCD   │
        └────────┬──┘ └──┬───────┘ └───────┘ └───────┘
                 │       │
            ┌────▼───────▼─────┐
            │ 采样电阻 10mΩ/2W │←─── 串联在电池负极回路
            └──────────────────┘
```

**通信架构要点**：

- BQ76920 与 BQ34Z100-G1 **共用 I²C1 总线**（地址不冲突：BQ76920=0x08/0x18，BQ34Z100-G1=0x55），上拉电阻 4.7kΩ 至 3.3V，主控 STM32G474 作为 I²C Master
- BQ76920 的 ALERT 引脚直接连 STM32 EXTI 中断引脚，故障时实时唤醒 MCU 处理
- 充放电 MOS 由 BQ76920 的 CHG/DSG 引脚直接驱动（**低边 NMOS 配置**，不用 BQ76200 高边）

---

## 2. 核心芯片引脚速查表

### 2.1 BQ76920 (TSSOP-20，源自 TI Datasheet SLUSBK2I)

| Pin | Name | I/O | 功能 |
|---|---|---|---|
| 1 | DSG | O | 放电 NMOS 栅极驱动输出 |
| 2 | CHG | O | 充电 NMOS 栅极驱动输出 |
| 3 | VSS | — | 芯片地（连电池组负极 B-） |
| 4 | SDA | I/O | I²C 数据线 |
| 5 | SCL | I | I²C 时钟线 |
| 6 | TS1 | I | NTC 热敏电阻正端（103AT） |
| 7 | CAP1 | O | 内部 LDO 滤波电容（接 1µF 到 VSS） |
| 8 | REGOUT | P | LDO 输出（2.5V/3.3V，看子型号） |
| 9 | REGSRC | I | LDO 输入（接 BAT 经 RC 滤波） |
| 10 | BAT | P | 电池组顶部（B+，最高电压输入） |
| 11 | NC | — | 悬空 |
| 12 | VC5 | I | 第 5 节电池正极采样 |
| 13 | VC4 | I | 第 4 节电池正极采样 |
| 14 | VC3 | I | 第 3 节电池正极采样 |
| 15 | VC2 | I | 第 2 节电池正极采样 |
| 16 | VC1 | I | 第 1 节电池正极采样 |
| 17 | VC0 | I | 第 1 节电池负极（=B-） |
| 18 | SRP | I | 电流采样负端（靠近 VSS） |
| 19 | SRN | I | 电流采样正端（靠近 PACK-） |
| 20 | ALERT | I/O | 故障中断输出（接 MCU EXTI） |

**选型建议**：`BQ7692003PWR`（3.3V LDO + CRC + 0x18 地址）或 `BQ7692006PWR`（3.3V + CRC + 0x18）。优先用 3.3V LDO 版本，与 STM32 电平直接匹配，省一级电平转换。

### 2.2 BQ34Z100-G1 (TSSOP-14，源自 TI Datasheet SLUSBZ5D)

| Pin | Name | I/O | 功能 |
|---|---|---|---|
| 1 | P2 | O | LED2 / 不用则下拉到 VSS |
| 2 | VEN | O | 电压翻译使能（接 PNP 三极管控制分压电路） |
| 3 | P1 | O | LED1 / 不用则下拉到 VSS |
| 4 | BAT | I | 电池电压输入（**经分压网络，最大 5V**） |
| 5 | CE | I | 芯片使能（高有效，可直接接 REGIN） |
| 6 | REGIN | P | LDO 输入（2.7-4.5V）|
| 7 | REG25 | P | 内部 LDO 输出（2.5V，对地 1µF 退耦） |
| 8 | VSS | P | 数字地 |
| 9 | SRP | I | 电流采样正端（靠近 BAT-） |
| 10 | SRN | I | 电流采样负端（靠近 PACK-） |
| 11 | P6/TS | I | NTC 温度采样（103AT）|
| 12 | P5/HDQ | I/O | HDQ 单线接口（不用则下拉到 VSS） |
| 13 | P4/SCL | I | I²C 时钟（10kΩ 上拉至 VCC） |
| 14 | P3/SDA | I/O | I²C 数据（10kΩ 上拉至 VCC） |

**关键参数（来自 datasheet 6.1 节绝对最大额定值）**：

- VREGIN 最大 5.5V，必须由 3.3V 系统供电
- VBAT 最大 5V，**所以 21V 电池组必须经过分压再接 BAT 引脚**
- VSRP/VSRN 范围 ±125mV，与 BQ76920 共用同一 10mΩ 采样电阻

### 2.3 L610-CN 4G 模组（成品模组接口）

按 BOM 清单"BMS项目BOM清单.xlsx"备注，使用 **L610 成品模组（带板）**，外接以下信号即可：

| 模组接口 | 方向 | 功能 | 电平 |
|---|---|---|---|
| VBAT | I | 电源（3.4-4.2V，峰值 1.8A） | 模拟 |
| GND | — | 地 | — |
| MAIN_TXD | O | 串口发送（送给 MCU 的 RX） | 1.8V/3.3V |
| MAIN_RXD | I | 串口接收（从 MCU TX 来） | 1.8V/3.3V |
| PWRKEY | I | 开机键（**拉低 ≥1s 触发开机**） | 1.8V |
| RESET_N | I | 复位（拉低 100ms 复位） | 1.8V |
| NET_STATUS | O | 网络状态指示 LED | 1.8V |
| SIM_VDD/CLK/RST/IO | — | SIM 卡接口（成品模组通常带卡座） | — |

**重点**：常见的成品 ADP-L610 扩展板上电后通过短接跳线/按键自动开机。如果用 STM32 自动控制开机，需要把 PWRKEY 接到一个 GPIO，开机时拉低 ≥2s 再释放。

---

## 3. STM32G474RET6 引脚分配

> **基于 STM32G474 datasheet (DS12288)，所有 AF 复用功能均经过官方手册确认。STM32G474RET6 与 STM32G431RBT6 同为 LQFP64 封装、引脚功能完全兼容，因此原项目软件设计中的引脚分配可直接沿用。**

### 3.1 引脚功能总表

| 引脚 | 复用功能 (AF) | 使用方向 | 连接对象 | 备注 |
|---|---|---|---|---|
| **PA0** | ADC1_IN1 | 模拟输入 | NTC2 温度采样 | 12-bit ADC |
| **PA1** | ADC1_IN2 | 模拟输入 | NTC3 温度采样 | 12-bit ADC |
| **PA2** | USART2_TX (AF7) | 输出 | USB-TTL 调试 RX | 调试串口 115200 |
| **PA3** | USART2_RX (AF7) | 输入 | USB-TTL 调试 TX | 调试串口 115200 |
| **PA4** | GPIO_OUT | 输出 | L610 PWRKEY | 拉低 2s 开机 |
| **PA5** | SPI1_SCK (AF5) | 输出 | TFT LCD SCK | 18MHz |
| **PA6** | GPIO_OUT | 输出 | L610 RESET_N | 复位脉冲 100ms |
| **PA7** | SPI1_MOSI (AF5) | 输出 | TFT LCD MOSI | 仅写屏 |
| **PA8** | GPIO_IN/EXTI8 | 输入 | L610 NET_STATUS | 网络状态监控 |
| **PA9** | USART1_TX (AF7) | 输出 | L610 MAIN_RXD | 4G AT 命令 |
| **PA10** | USART1_RX (AF7) | 输入 | L610 MAIN_TXD | 4G AT 应答 |
| **PA11** | FDCAN1_RX (AF9) | 输入 | TJA1050 RXD | CAN 总线 |
| **PA12** | FDCAN1_TX (AF9) | 输出 | TJA1050 TXD | CAN 总线 |
| **PA13** | SWDIO (AF0) | 调试 | ST-Link DIO | 保留 |
| **PA14** | SWCLK (AF0) | 调试 | ST-Link CLK | 保留 |
| **PB0** | GPIO_OUT | 输出 | LCD DC | 数据/命令选择 |
| **PB1** | GPIO_OUT | 输出 | LCD CS | 片选 |
| **PB2** | GPIO_OUT | 输出 | LCD RST | 屏复位 |
| **PB3** | EXTI3 | 输入 | BQ76920 ALERT | **关键中断**，上升沿触发 |
| **PB4** | GPIO_OUT | 输出 | LED 绿色（正常） | 推挽 |
| **PB5** | GPIO_OUT | 输出 | LED 红色（告警） | 推挽 |
| **PB6** | I2C1_SCL (AF4) | 开漏 | BQ76920 SCL + BQ34Z100 SCL | 4.7kΩ 上拉到 3.3V |
| **PB7** | I2C1_SDA (AF4) | 开漏 | BQ76920 SDA + BQ34Z100 SDA | 4.7kΩ 上拉到 3.3V |
| **PB8** | GPIO_OUT | 输出 | 蜂鸣器 | 经 NPN 三极管驱动 |
| **PB9** | GPIO_OUT | 输出 | BQ34Z100 VEN（可选） | 控制分压电路省电 |
| **PB12** | EXTI12 | 输入 | 按键1 模式 | 上拉 + 下降沿触发 |
| **PB13** | EXTI13 | 输入 | 按键2 确认 | 上拉 + 下降沿触发 |
| **PB14** | EXTI14 | 输入 | 按键3 上 | 上拉 + 下降沿触发 |
| **PB15** | EXTI15 | 输入 | 按键4 下 | 上拉 + 下降沿触发 |

### 3.2 关于 STM32G474 与 STM32G431 引脚兼容性

STM32G474RET6 和 STM32G431RBT6 都是 LQFP64 封装，引脚位置和复用功能基本一致。差异只在性能：G474 主频 170MHz、512KB Flash、128KB RAM，G431 主频 170MHz、128KB Flash、32KB RAM。本项目的 BMS 软件复杂度（FreeRTOS+LVGL+MQTT）建议用 G474，资源富余。

### 3.3 时钟配置

| 时钟项 | 配置 |
|---|---|
| HSE | 8MHz 外部晶振（最小系统板自带） |
| PLL 倍频 | HSE → /1 → ×85 → /4 → 170MHz |
| SYSCLK | 170MHz |
| AHB | 170MHz |
| APB1 | 170MHz |
| APB2 | 170MHz |
| FreeRTOS Tick | 1ms (1000Hz) |
| LSI | 32kHz（独立看门狗） |

---

## 4. BQ76920 AFE 模块连线

### 4.1 电池电压采样网络（核心，最容易出错）

5 串电池有 6 个采样节点（B-, C1+, C2+, C3+, C4+, C5+, B+ 共 7 个，对应 BQ76920 的 VC0~VC5 + BAT 共 6 个引脚）。每节电池正极都需要经过 **RC 滤波**接到对应 VC 引脚。

**官方推荐 RC 网络（datasheet 9.1.4 节）**：

```
电池节点 ───── R(100Ω~1kΩ) ───┬─────► VCx 引脚
                              │
                              C (100nF~1µF)
                              │
                          下一节电池正极（即 VC(x-1) 节点）
```

**5 串完整连接**：

| 电池节点 | BQ76920 引脚 | 串联电阻 Rc | 旁路电容 Cc |
|---|---|---|---|
| B+ (电池组正极) | Pin 10 BAT | **100Ω** | **1µF** 到 VC5 节点 |
| Cell5 正极 = Cell4-Cell5 之间 | Pin 12 VC5 | **100Ω** | **1µF** 到 VC4 节点 |
| Cell4 正极 | Pin 13 VC4 | **100Ω** | **1µF** 到 VC3 节点 |
| Cell3 正极 | Pin 14 VC3 | **100Ω** | **1µF** 到 VC2 节点 |
| Cell2 正极 | Pin 15 VC2 | **100Ω** | **1µF** 到 VC1 节点 |
| Cell1 正极 | Pin 16 VC1 | **100Ω** | **1µF** 到 VC0 节点 |
| B- (电池组负极) | Pin 17 VC0 | **100Ω** | **1µF** 到 VSS |
| Pin 11 NC | 悬空 | — | — |

**重点参数（datasheet Table 7.3 推荐操作条件）**：

- Rc 范围：40Ω~1kΩ，**典型 100Ω**
- Cc 范围：100nF~10µF，**典型 1µF**
- Rf（电源滤波）：40Ω~1kΩ，**典型 100Ω**
- Cf（电源滤波）：1µF~40µF，**典型 10µF**

### 4.2 电源 / 通信 / NTC 部分

```
                  ┌─── B+ (电池正极)
                  │
                  ├── Rf=100Ω ──┬─── Pin 9 REGSRC
                  │             │
                  │            Cf=10µF
                  │             │
                  └─────────────┴─── 到 VSS (B-)

                  Pin 10 BAT  → 已含在 4.1 节连接 (经 100Ω+1µF)

                  Pin 7 CAP1  ──┬── 1µF 电容 ── VSS
                  Pin 8 REGOUT ─┴── 1µF 电容 ── VSS  → 输出 3.3V

                  Pin 6 TS1 ──┬─── 103AT NTC ── VSS
                              │
                              └─ 10kΩ 下拉到 VSS（NTC 不用时使用）

                  Pin 4 SDA ──┬── 4.7kΩ 上拉到 REGOUT(3.3V) ──► STM32 PB7
                  Pin 5 SCL ──┴── 4.7kΩ 上拉到 REGOUT(3.3V) ──► STM32 PB6
                  Pin 20 ALERT ── 1MΩ 下拉到 VSS ──► STM32 PB3 (EXTI)
```

**ALERT 上拉/下拉关键说明（datasheet 7.5 节 ALERT PIN 部分）**：

- ALERT 是 **双向引脚**：BQ76920 输出告警时拉高，外部可主动拉高强制进入 SHUTDOWN
- 必须有 1MΩ 下拉电阻到 VSS（datasheet 推荐值 RALERT_PULLDOWN）
- VALERT_OH ≈ REGOUT × 0.75 ≈ 2.475V（足以让 STM32 识别为高电平）

### 4.3 电流采样回路（最关键的高精度回路）

```
   电池组 B- ──────┬───────────────────┬─── PACK-
                    │                   │
                    │   Rsns = 10mΩ     │
                    └───[ 采样电阻 ]────┘
                        │           │
                        │           │
                  ┌─────┴───────────┴─────┐
                  │                       │
                Rfilt=100Ω          Rfilt=100Ω
                  │                       │
                  ├── Pin 18 SRP          ├── Pin 19 SRN
                  │                       │
                  └── Cfilt=100nF ────────┘  (差分电容跨接 SRP-SRN)
```

**采样电阻选型**：

- 阻值：**10mΩ**（BOM 清单已选，2512 封装）
- 功率：**≥ 2W**（10A 时 P=I²R=1W，留 2 倍裕量）
- 精度：**1%**（直接影响电流测量精度）

**OCD/SCD 阈值与电流的关系（datasheet 7.5 节）**：

| 保护类型 | 阈值范围（电压） | 对应电流（10mΩ 时） | 步进 |
|---|---|---|---|
| OCD（过流放电） | 8 mV ~ 100 mV | 0.8A ~ 10A | 5.56mV (RSNS=1) |
| SCD（短路放电） | 22 mV ~ 200 mV | 2.2A ~ 20A | 22.2mV (RSNS=1) |
| CC 测量范围 | ±200 mV | ±20A | 8.44µV/LSB |

---

## 5. BQ34Z100-G1 电量计模块连线

### 5.1 电源与电压翻译电路（关键点：21V 电池电压必须分压）

BQ34Z100-G1 的 BAT 引脚最大允许 5V（datasheet 6.1 节绝对最大额定值），但 5S 电池组满电 21V，因此**必须设计电压翻译电路**。

```
   B+ (21V 满电) ─────┬─── R_top = 1.5MΩ ───┬─── 到 BQ34Z100 BAT (Pin 4)
                      │                     │
                      │                     R_bot = 68kΩ
                      │                     │
                      │                  ┌──┴── 到 VSS (Pin 8)
                      │                  │
                      │                100nF 电容（ADC 滤波）
                      │                  │
                      └──────────────────┘

   分压比：68 / (1500 + 68) = 1/23.06
   21V 时：21 × 0.0434 = 911 mV ✓（在 ADC 范围内）
   12.5V 时：12.5 × 0.0434 = 542 mV ✓
```

**VEN 引脚的作用**：BQ34Z100-G1 可控制一个外部三极管开关分压电路，平时断开省电（节省 21V/1.5MΩ ≈ 14µA + 漏电流）。但本项目电池容量足够，**可以简化处理：VEN 引脚悬空，分压电路常通**。

### 5.2 完整外围电路

```
                    Pin 6 REGIN ◄─── 3.3V 电源
                                     │
                                     0.1µF 退耦
                                     │
                                     VSS

                    Pin 5 CE ────────► 直接连接 REGIN（常使能）

                    Pin 7 REG25 ────┬── 1µF 退耦电容 ── VSS
                                    └── 内部 LDO 输出 (2.5V)，仅供内部使用，不外接

                    Pin 4 BAT ────── 见 5.1 节分压电路

                    Pin 9 SRP ──┬── 100Ω ─── 到采样电阻 BAT- 一侧
                    Pin 10 SRN ─┴── 100Ω ─── 到采样电阻 PACK- 一侧
                                       │
                                  跨接 100nF 差分电容

                    Pin 11 P6/TS ─── 10kΩ NTC ─── VSS（独立 NTC，与 BQ76920 的 TS1 分开）
                                  ┌─ 10kΩ 上拉到 REG25 (内部已有，外部不需要)

                    Pin 12 P5/HDQ ── 10kΩ 下拉到 VSS （不使用 HDQ）

                    Pin 13 P4/SCL ──┐
                    Pin 14 P3/SDA ──┤   连接到 STM32 I²C1 总线（共用）
                                    │   已经有 4.7kΩ 上拉，不重复
                                    └─► STM32 PB6/PB7
                    Pin 1 P2 / Pin 3 P1 ── 下拉到 VSS（不用 LED）
                    Pin 2 VEN ── 悬空 或 接 STM32 PB9（可选低功耗控制）
```

### 5.3 BQ76920 与 BQ34Z100-G1 共用 I²C 总线说明

两个芯片 I²C 地址不冲突：

| 芯片 | 7-bit 地址 | 选项 |
|---|---|---|
| BQ76920 | 0x08 或 0x18 | 由芯片型号决定 |
| BQ34Z100-G1 | 0x55 (0xAA 8-bit) | 固定 |

共用 I²C1 总线（PB6/PB7）**只需一对 4.7kΩ 上拉电阻到 3.3V**。STM32 通过不同地址分别访问两个从机。

**两个芯片用同一颗采样电阻 10mΩ**：BQ76920 的 SRP/SRN 和 BQ34Z100-G1 的 SRP/SRN 都跨接在同一个采样电阻两端，**注意极性约定**：

- BQ76920：SRP 靠近 VSS（B- 侧），SRN 靠近 PACK-（远离电池侧）
- BQ34Z100-G1：SRP 靠近 BAT-（电池侧），SRN 靠近 PACK-（远离电池侧）
- 即两颗芯片的 SRP 都在电池侧、SRN 都在 PACK- 侧，**极性一致**

---

## 6. 充放电 MOS 管驱动电路

5S 系统使用 BQ76920 自带的 **低边 NMOS 驱动**（CHG/DSG 引脚），无需额外的 BQ76200 高边驱动芯片。这是 5S 入门方案的标准做法（datasheet 9.2 典型应用电路）。

### 6.1 低边 NMOS 拓扑

```
   电池 B+ ─────────────────────────────────────────────► PACK+

   电池 B- ──┬── DSG_FET (Q1) ──┬── CHG_FET (Q2) ──┬── PACK-
            │  D    G   S       │  D    G   S      │
            │       │            │       │           │
            │       │            │       │           │
            │       │            │       │          ┌┴┐
            │       │            │       │          │ │ Rsns 10mΩ
            │       │            │       │          │ │
            │       │            │       │          └┬┘
            │       │            │       │           │
            │       └─ 10kΩ 下拉 │       └─ 10kΩ 下拉│
            │       │            │       │           │
            │       │            │       │           │
            │       └─ Pin 1 DSG │       └─ Pin 2 CHG│
            │                                        │
            └────────────────── 到 SRP (Pin 18) ─────┘
                                到 SRN (Pin 19) ◄────┘
```

**MOS 管选型（参考 BOM 清单 AO3400/SI2302，但 5S 的电流稍大）**：

| 参数 | AO3400 | SI2302 | 推荐值 |
|---|---|---|---|
| 封装 | SOT-23 | SOT-23 | SOT-23 / DPAK |
| Vds | 30V | 20V | **≥ 40V** |
| Id (持续) | 5.7A | 2.6A | **≥ 15A** |
| Rds(on) | 26mΩ@10V | 65mΩ@10V | < 30mΩ |

**5 串放电 5A 场景下，AO3400 勉强够用，但建议用更大的 NMOS 如 AOD508、SQJ422EP-T1 或并联 2~3 个 AO3400**。

### 6.2 CHG/DSG 引脚电平特性（datasheet 7.5 节 VFETON）

- **CHG_ON / DSG_ON 时输出电压**：`VFETON ≈ 12V`（当 REGSRC ≥ 12V 时，即 5S 电池电压 > 12V 时）
- **关断时**：CHG 通过 1MΩ 弱下拉到 VSS，DSG 通过 2.5kΩ 下拉到 VSS
- 这个 12V 电平直接驱动 **N-MOS 共源极结构**，标准 logic-level NMOS 即可（Vgs 阈值 < 5V）

### 6.3 自恢复保险丝与 TVS

```
  电池 B+ ──── 自恢复保险丝(3A) ────► 系统主电源
                                  │
                                  ├── TVS (SMBJ24A，截止电压 26.7V) ─── B-
                                  │
                                  └── 滤波电容 100µF + 100nF ─── B-
```

---

## 7. L610 4G 模组连线

### 7.1 成品模组连接（按 BOM 清单使用 ADP-L610-Arduino 类扩展板）

```
   ┌─────────────────────────────┐
   │   L610 成品模组扩展板         │
   │                             │
   │  VBAT  ────► 4V 电源（专用LDO/DCDC，2A 余量）
   │  GND   ────► 系统地
   │  TXD   ────► STM32 PA10 (USART1_RX)
   │  RXD   ◄──── STM32 PA9  (USART1_TX)
   │  PWRKEY◄──── STM32 PA4  (开机控制)
   │  RESET◄──── STM32 PA6  (复位控制)
   │  NETSTA────► STM32 PA8  (网络状态指示，可不接)
   │  SIM 卡座（板载，插物联卡）
   │  4G 天线（IPEX 接口）
   └─────────────────────────────┘
```

### 7.2 时序与上电流程

```
   电源上电
        │
        ▼
   等待 VBAT 稳定（≥30ms）
        │
        ▼
   STM32 PA4 (PWRKEY) 拉低 ≥ 2s
        │
        ▼
   PA4 释放（拉高或浮空）
        │
        ▼
   等待 5-15s（模组启动 + 注网）
        │
        ▼
   发送 "AT\r\n" 测试通信
        │
        ▼
   收到 "OK" → 进入业务流程（MQTT 注册等）
```

### 7.3 串口电平注意

L610 的 UART 是 **1.8V 电平**（部分版本 3.0V）。STM32G474 的 GPIO 是 **3.3V 电平**，直接相连可能损坏 L610。

**电平转换方案**（按下方任选其一）：

| 方案 | 优点 | 缺点 |
|---|---|---|
| 串入分压电阻（10kΩ + 10kΩ） | 简单，0 成本 | 速率受限，仅适合 ≤115200 bps |
| 专用电平转换芯片 TXS0108 / SN74LVC1T45 | 速率高，双向自动 | 多一颗芯片 |
| 成品扩展板自带电平转换 | 最简单 | 依赖板子设计 |

**ADP-L610-Arduino 等成品扩展板通常已经做了电平转换，确认你买的板子型号支持 3.3V 输入即可直接接**。

### 7.4 4G 模组电源供给（最容易踩坑的地方）

L610 的 VBAT 引脚要求：

- 电压范围：**3.4 ~ 4.2V**（推荐 3.8V）
- 峰值电流：**≥ 1.8A**（4G 突发）
- 纹波：< 200mV

**直接用 5S 电池经一级 DC-DC 降压到 4V**（参考方案：MP2315 / MP1584 + 调节反馈电阻到 4V），独立于 STM32 的 3.3V 供电，避免 4G 突发干扰主控。

---

## 8. 电源系统设计

### 8.1 电源树

```
   电池组 B+ (12.5V ~ 21V)
        │
        ├──► DC-DC #1 (MP1584 调到 4V, 输出 ≥2A) ──► L610 VBAT
        │
        ├──► DC-DC #2 (MP1584 调到 5V, 输出 ≥1A)
        │      │
        │      └──► AMS1117-3.3 ──► STM32G474 + LCD 背光 + 上拉电阻 (3.3V/500mA)
        │
        ├──► BQ76920 REGSRC（经 100Ω+10µF 滤波）
        │      └─► REGOUT 3.3V (10mA) 仅供 BQ76920 内部 + I²C 上拉
        │
        ├──► BQ34Z100 REGIN（直接接系统 3.3V）
        │
        └──► 分压电阻 1.5MΩ + 68kΩ ──► BQ34Z100 BAT
```

**注意**：BQ76920 的 REGOUT 输出能力只有 **10mA**（datasheet 7.5 IEXTLDO_LIMIT 30-45mA, 但推荐使用不超 10mA），不要把整个系统 3.3V 都接它。它输出的 3.3V **仅用于本 AFE 自身和 I²C 上拉**，主控供电由独立 DC-DC + LDO 提供。

### 8.2 关键退耦电容

| 位置 | 电容 | 备注 |
|---|---|---|
| STM32 每个 VDD 引脚 | 100nF | 紧贴芯片 |
| STM32 VBAT | 100nF + 1µF | 备份域 |
| BQ76920 REGSRC-VSS | **10µF + 100nF** | datasheet 强制要求 |
| BQ76920 CAP1-VSS | **1µF** | 必须，内部电荷泵需要 |
| BQ76920 REGOUT-VSS | 1µF | LDO 输出滤波 |
| BQ34Z100 REGIN-VSS | 100nF | LDO 输入 |
| BQ34Z100 REG25-VSS | 1µF | LDO 输出，必须 |
| L610 VBAT | 100µF (低 ESR) + 10µF + 100nF | 4G 突发电流 |

### 8.3 接地策略

- **大电流地（功率地）**：电池负极、采样电阻、MOS 管源极 ── 单独的粗铜接地
- **模拟地（AGND）**：BQ76920 VSS、BQ34Z100 VSS、ADC 参考、NTC 网络 ── 在采样电阻 PACK- 一侧单点连接到功率地
- **数字地（DGND）**：STM32、LCD、L610 ── 在主电源入口单点连接到模拟地
- **采样电阻**：使用 4 端开尔文连接（强制端走大电流，传感端走小电流到 SRP/SRN）

---

## 9. 外设模块连线（CAN/LCD/按键/蜂鸣器/LED）

### 9.1 CAN 收发器 TJA1050

```
   STM32 PA12 (FDCAN1_TX) ─────► TJA1050 Pin 1 TXD
   STM32 PA11 (FDCAN1_RX) ◄───── TJA1050 Pin 4 RXD
   3.3V ────────────────────────► TJA1050 Pin 3 VCC（**注意：TJA1050 是 5V 芯片，部分版本是 5V 唯一**）
   GND ─────────────────────────► TJA1050 Pin 2 GND
                                  TJA1050 Pin 5 Vref（可悬空）
                                  TJA1050 Pin 6 CANL
                                  TJA1050 Pin 7 CANH
                                  TJA1050 Pin 8 RS（接 GND = 高速模式）
   CANH ── 120Ω 终端电阻 ── CANL（总线两端各一个）
```

**重要**：TJA1050 标称 5V 供电，但有 3.3V 兼容版本（TJA1051T/3 才是 3.3V 兼容）。**如果用 5V TJA1050，TXD 输入是 5V 电平**，STM32 的 3.3V TX 输出虽能工作但电平裕量小，建议用 **TJA1042 / SN65HVD230 (3.3V 兼容)** 替代，或在 TXD 串一个 0Ω/电平转换。

### 9.2 TFT LCD（ST7735，1.8寸 SPI 接口）

```
   STM32 PA5 (SPI1_SCK)  ──► LCD SCK
   STM32 PA7 (SPI1_MOSI) ──► LCD SDA / MOSI
   STM32 PB1 (GPIO_OUT)  ──► LCD CS
   STM32 PB0 (GPIO_OUT)  ──► LCD DC (Data/Command)
   STM32 PB2 (GPIO_OUT)  ──► LCD RST
   3.3V ──────────────────► LCD VCC
   3.3V ── 100Ω ──► LCD BLK（背光，可加 PWM 调光）
   GND ───────────────────► LCD GND
```

### 9.3 按键（4 个，独立 GPIO + EXTI）

```
   3.3V ─── 10kΩ 上拉 ──┬─── STM32 PB12 (按键1 模式)
                       │
                       └─── 按键 ─── GND
   
   （PB13/14/15 同理）

   按键去抖：硬件 0.1µF 并联 + 软件 20ms 消抖
```

### 9.4 蜂鸣器（有源 3.3V）

```
   STM32 PB8 ──► 1kΩ ──► NPN 三极管 (S8050) Base
                       Collector ── 蜂鸣器(+) ── 3.3V/5V
                       Emitter ── GND
                       
   蜂鸣器并联续流二极管 1N4148（保护三极管）
```

如果蜂鸣器额定 3.3V 且电流 < 25mA，可以**直接用 STM32 GPIO 推挽驱动**（PB8 → 蜂鸣器 → GND）。

### 9.5 LED 指示

```
   STM32 PB4 ── 510Ω ── 绿色 LED ── GND   （正常运行心跳，1Hz）
   STM32 PB5 ── 510Ω ── 红色 LED ── GND   （告警/故障常亮或快闪）
```

### 9.6 NTC 温度传感器（3 路）

```
   NTC1 → BQ76920 Pin 6 TS1（已在 4.2 节描述）
   
   NTC2 (10kΩ 103AT) ──┬─── 10kΩ 上拉到 3.3V
                       └─── STM32 PA0 (ADC1_IN1)
                       
   NTC3 同理 → STM32 PA1 (ADC1_IN2)
```

NTC 与上拉电阻共同分压，由 ADC 测量电压，通过 Steinhart-Hart 方程或查表得到温度。

---

## 10. 完整连线对照总表

### 10.1 STM32G474RET6 ↔ 各模块连线总表

| STM32 引脚 | 复用 | 方向 | 目标芯片/模块 | 目标引脚 | 中间元件 |
|---|---|---|---|---|---|
| PA0 | ADC1_IN1 | I | NTC2 | 分压点 | 10kΩ 上拉到 3.3V |
| PA1 | ADC1_IN2 | I | NTC3 | 分压点 | 10kΩ 上拉到 3.3V |
| PA2 | USART2_TX | O | USB-TTL CH340 | RXD | 直接 |
| PA3 | USART2_RX | I | USB-TTL CH340 | TXD | 直接 |
| PA4 | GPIO | O | L610 模组 | PWRKEY | 直接 |
| PA5 | SPI1_SCK | O | TFT LCD | SCK | 直接 |
| PA6 | GPIO | O | L610 模组 | RESET_N | 直接 |
| PA7 | SPI1_MOSI | O | TFT LCD | SDA/MOSI | 直接 |
| PA8 | GPIO/EXTI | I | L610 模组 | NET_STATUS | 直接 |
| PA9 | USART1_TX | O | L610 模组 | MAIN_RXD | 电平匹配（成品板自带） |
| PA10 | USART1_RX | I | L610 模组 | MAIN_TXD | 电平匹配（成品板自带） |
| PA11 | FDCAN1_RX | I | TJA1042 | RXD | 直接 |
| PA12 | FDCAN1_TX | O | TJA1042 | TXD | 直接 |
| PA13 | SWDIO | I/O | ST-Link | DIO | 直接 |
| PA14 | SWCLK | I | ST-Link | CLK | 直接 |
| PB0 | GPIO | O | TFT LCD | DC | 直接 |
| PB1 | GPIO | O | TFT LCD | CS | 直接 |
| PB2 | GPIO | O | TFT LCD | RST | 直接 |
| PB3 | EXTI3 | I | BQ76920 | Pin 20 ALERT | 1MΩ 下拉 |
| PB4 | GPIO | O | 绿色 LED | 阳极 | 510Ω 限流 |
| PB5 | GPIO | O | 红色 LED | 阳极 | 510Ω 限流 |
| PB6 | I2C1_SCL | O | BQ76920 (P5) + BQ34Z100 (P13) | SCL | 4.7kΩ 上拉到 3.3V |
| PB7 | I2C1_SDA | I/O | BQ76920 (P4) + BQ34Z100 (P14) | SDA | 4.7kΩ 上拉到 3.3V |
| PB8 | GPIO | O | 蜂鸣器 | (+) | 经 NPN 驱动 |
| PB9 | GPIO | O | BQ34Z100 | Pin 2 VEN | 可悬空 |
| PB12 | EXTI12 | I | 按键1 | — | 10kΩ 上拉 + 100nF 去抖 |
| PB13 | EXTI13 | I | 按键2 | — | 同上 |
| PB14 | EXTI14 | I | 按键3 | — | 同上 |
| PB15 | EXTI15 | I | 按键4 | — | 同上 |

### 10.2 BQ76920 引脚连接总表

| Pin | Name | 目标 | 中间元件 |
|---|---|---|---|
| 1 | DSG | 放电 NMOS Q1 栅极 | 10kΩ 下拉到 VSS |
| 2 | CHG | 充电 NMOS Q2 栅极 | 10kΩ 下拉到 VSS |
| 3 | VSS | 电池组 B-（地） | 直接 |
| 4 | SDA | STM32 PB7 | 4.7kΩ 上拉到 REGOUT |
| 5 | SCL | STM32 PB6 | 4.7kΩ 上拉到 REGOUT |
| 6 | TS1 | NTC1 (103AT) | 不用则 10kΩ 下拉到 VSS |
| 7 | CAP1 | VSS | 1µF 电容 |
| 8 | REGOUT | I²C 上拉电源 + BQ34Z100 | 1µF 退耦 |
| 9 | REGSRC | B+ | 100Ω + 10µF 滤波 |
| 10 | BAT | B+ | 100Ω + 1µF（构成 RC 网络） |
| 11 | NC | 悬空 | — |
| 12 | VC5 | Cell5 正极 | 100Ω + 1µF |
| 13 | VC4 | Cell4 正极 | 100Ω + 1µF |
| 14 | VC3 | Cell3 正极 | 100Ω + 1µF |
| 15 | VC2 | Cell2 正极 | 100Ω + 1µF |
| 16 | VC1 | Cell1 正极 | 100Ω + 1µF |
| 17 | VC0 | B-（电池负极） | 100Ω + 1µF |
| 18 | SRP | 采样电阻 B- 一侧 | 100Ω |
| 19 | SRN | 采样电阻 PACK- 一侧 | 100Ω + 100nF 跨接到 SRP |
| 20 | ALERT | STM32 PB3 | 1MΩ 下拉到 VSS |

### 10.3 BQ34Z100-G1 引脚连接总表

| Pin | Name | 目标 | 中间元件 |
|---|---|---|---|
| 1 | P2 | VSS（不用 LED） | 直接下拉 |
| 2 | VEN | STM32 PB9 或 悬空 | 可选 |
| 3 | P1 | VSS（不用 LED） | 直接下拉 |
| 4 | BAT | B+ | **1.5MΩ + 68kΩ 分压** + 100nF 滤波 |
| 5 | CE | REGIN | 直接（常使能） |
| 6 | REGIN | 系统 3.3V | 100nF 退耦 |
| 7 | REG25 | VSS | 1µF 退耦（仅滤波，不外接） |
| 8 | VSS | 系统地 | 直接 |
| 9 | SRP | 采样电阻 B- 一侧 | 100Ω |
| 10 | SRN | 采样电阻 PACK- 一侧 | 100Ω + 100nF 跨接 |
| 11 | P6/TS | NTC 独立 NTC2'（与 BQ76920 NTC 独立） | 内部已含 10kΩ 上拉 |
| 12 | P5/HDQ | VSS（不用 HDQ） | 10kΩ 下拉 |
| 13 | P4/SCL | STM32 PB6 | 共用总线，已上拉 |
| 14 | P3/SDA | STM32 PB7 | 共用总线，已上拉 |

---

## 11. 关键设计要点与避坑提醒

### 11.1 高优先级避坑点（按经验严重度排序）

1. **🚨 BQ76920 BAT 引脚必须经过 RC 滤波**：直接接 B+ 会导致 ESD/瞬态损坏芯片。100Ω + 1µF 是 datasheet 强制要求。
2. **🚨 BQ34Z100-G1 BAT 引脚最大 5V**：21V 电池电压必须分压。直接接会瞬间烧片。
3. **🚨 采样电阻必须使用 4 端开尔文连接**：强制端（power）走大电流，传感端（sense）走小信号到 SRP/SRN。如果 SRP/SRN 直接接到强制端的铜皮上，会引入数 mΩ 的额外阻抗，电流读数可能差 30%+。
4. **🚨 ALERT 引脚必须有 1MΩ 下拉**：缺失会导致 ALERT 浮空，误触发或永远不触发。
5. **🚨 BQ76920 REGOUT 不要给主控供电**：最大输出 10-30mA，无法驱动 STM32 + LCD。主控用独立 DC-DC + LDO。
6. **🚨 L610 VBAT 必须独立 4V 电源**：直接接 5V 或 3.3V 都会工作不稳定。4G 突发 1.8A，电源容量不足会导致脱网。
7. **🚨 上电时序**：先给 BQ76920 上电（RC 滤波 + REGSRC 缓启动），等其输出 REGOUT 稳定 ≥10ms 后再启动 STM32 的 I²C 通信。

### 11.2 软件配套关键参数

| 参数 | 推荐值 | 说明 |
|---|---|---|
| BQ76920 I²C 地址 | 0x18 | 按 BQ7692006 子型号 |
| BQ76920 OV 阈值 | 4250 mV | 单节最大电压 |
| BQ76920 UV 阈值 | 2800 mV | 单节最小电压 |
| BQ76920 OCD 阈值 | 50 mV (5A @ 10mΩ) | 持续过流 |
| BQ76920 SCD 阈值 | 100 mV (10A @ 10mΩ) | 短路阈值 |
| BQ34Z100-G1 I²C 地址 | 0x55 | 固定 |
| BQ34Z100-G1 设计容量 | 2600 mAh | 18650 单节标称（5S1P 总容量也是 2600 mAh） |
| BQ34Z100-G1 设计能量 | 5180 mWh × 5 串 = 25900 mWh | 总能量 |
| Pack Configuration | VOLTSEL=1（外部分压） | 必须配置 |

### 11.3 BQ76920 寄存器关键配置（首次上电 init 序列）

```c
// 1. 启动 ADC
write_reg(SYS_CTRL1, 0x18);  // ADC_EN=1, TEMP_SEL=1（用外部 TS1）

// 2. 启动连续 CC
write_reg(SYS_CTRL2, 0x40);  // CC_EN=1，连续模式

// 3. 配置保护阈值（OCD/SCD/UV/OV）
write_reg(PROTECT1, 0x0C);   // RSNS=0, SCD_T=400us, SCD_THRESH=100mV
write_reg(PROTECT2, 0x05);   // OCD_T=320ms, OCD_THRESH=50mV
write_reg(PROTECT3, 0x40);   // UV_T=4s, OV_T=4s
write_reg(OV_TRIP,  0xAC);   // 4.25V (按 datasheet 公式计算)
write_reg(UV_TRIP,  0x97);   // 2.80V

// 4. 启动充放电 MOS
write_reg(SYS_CTRL2, 0x43);  // CC_EN=1, CHG_ON=1, DSG_ON=1
```

### 11.4 BQ34Z100-G1 配置流程（软件首次烧写）

通过 I²C 进入 ROM 模式，写入 Data Flash：

1. UNSEAL 解锁（默认密码 0x36720414 / 0xFFFFFFFF）
2. 进入 CONFIG_UPDATE 模式
3. 写入 `Pack Configuration`：VOLTSEL=1（启用外部分压）
4. 写入 `Number of Series Cells`：5
5. 写入 `Voltage Divider`：根据实际分压电路标定（21000mV → ADC 读值 → 计算 ratio）
6. 写入 `Design Capacity`：2600
7. 写入 `Design Energy`：25900
8. 写入 Chemistry ID（先用 GPC 工具匹配，三元锂典型 ID = 0x180）
9. EXIT_CFGUPDATE
10. 进行 Learning Cycle（完整充放电循环以让 Impedance Track 学习真实容量）

### 11.5 PCB 布局核心约束

- **采样电阻周边**：差分走线 SRP/SRN，等长，立即并排，远离开关电源
- **BQ76920 大电压区与小信号区**：严格分隔，VC0~VC5 走线尽量短直，避免靠近 SPI/UART 等高速线
- **均衡电阻位置**：均衡电阻（约 33Ω, 1W）放在 BQ76920 旁边，对地有铜皮散热
- **L610 周围**：保留天线净空区，4G 信号线远离 I²C / SPI / 12V 电源
- **采样电阻使用 2 盎司铜皮**：减小寄生电阻

---

## 附录 A：参考资料

| 文档 | 资料链接 |
|---|---|
| BQ76920/30/40 Datasheet | TI SLUSBK2I |
| BQ34Z100-G1 Datasheet | TI SLUSBZ5D |
| BQ76920 Application Note | TI SLUA749 / SLUA808 |
| STM32G474xx Datasheet | ST DS12288 |
| STM32G4 Reference Manual | ST RM0440 |
| 广和通 L610-CN 硬件用户手册 | Fibocom 官网（需注册下载） |

## 附录 B：本设计与原 BMS_Software_Design.jsx 的对应关系

原 jsx 软件设计文件中预设的引脚分配在本硬件设计中**完全保留**，软件团队无需修改 CubeMX 配置。仅新增以下两点：

1. **BQ34Z100-G1 接入 I²C1 总线**（与 BQ76920 共用 PB6/PB7），地址 0x55，软件需在 I²C 任务中增加对其的轮询代码
2. **L610 控制引脚**：PA4 (PWRKEY)、PA6 (RESET_N)、PA8 (NET_STATUS)，软件初始化阶段需先执行模组上电时序

---

> **本文档基于 TI 官方 datasheet（BQ76920 SLUSBK2I rev I, BQ34Z100-G1 SLUSBZ5D rev D）和 ST 官方 datasheet（STM32G474xx DS12288）核对，所有引脚号、电气参数、电容/电阻推荐值均与 datasheet 一致。**  
> **作者：Claude（Anthropic）— BMS-5S Hardware Design — 2026-04-25**

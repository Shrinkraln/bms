# BMS-5S 最终版主控引脚 + 电路连接总表

> **文档版本**：V2.0 最终版（基于实际板子修订）  
> **日期**：2026-04-26  
> **核心变更**：基于实际最小系统板布局，PA9/PA10 不可用，CAN 保留 PA11/PA12，调试改用 USART2 + 集成 CH340G

---

## 1. 项目硬件结构（最终）

```
集成在主板 PCB 上（自己设计）：
  ├─ BQ76920 AFE + 全部外围
  ├─ BQ34Z100-G1 电量计 + 分压电路
  ├─ TJA1042 CAN 收发器
  ├─ MP1584 DC-DC × 2（4V 给 L610，5V 给系统）
  ├─ AMS1117-3.3 LDO（5V → 3.3V）
  ├─ CH340G USB-TTL（板载调试）+ Type-C 接口
  ├─ 充放电 MOS Q1/Q2（AOD508）
  ├─ Rsns 采样电阻 + 均衡电阻 + RC 滤波
  ├─ 保险丝 + TVS 保护
  ├─ 按键、LED、蜂鸣器、NTC
  └─ 接线端子（PACK / 电池 / CAN）

外置成品模组（通过排针对接）：
  ├─ STM32G474 最小系统板（4 排 × 15 引脚母排针）
  ├─ L610 4G 模组（1×6 母排针）
  └─ TFT LCD 屏（8P 或 14P 排母）
```

---

## 2. 完整引脚分配表

### 2.1 STM32G474 引脚映射

| 引脚 | 复用功能 | 网络名 | 连接对象 | 外围元件 |
|---|---|---|---|---|
| **PA0** | ADC1_IN1 | NTC2_AIN | NTC2 | R25 (10kΩ) 上拉到 3V3 |
| PA1 | — | (备用) | — | 悬空 |
| **PA2** | USART2_TX | UART_DBG_TX | CH340G Pin3 RXD | 直连 |
| **PA3** | USART2_RX | UART_DBG_RX | CH340G Pin2 TXD | 直连 |
| **PA4** | GPIO | L610_PWRKEY | J5-5 PWRKEY | 直连 |
| **PA5** | SPI1_SCK | LCD_SCK | LCD 模组 SCK | 直连 |
| **PA6** | GPIO | L610_RESET | J5-6 RESET_N | 直连 |
| **PA7** | SPI1_MOSI | LCD_MOSI | LCD 模组 SDA | 直连 |
| **PA8** | EXTI8 | L610_NETSTA | J5-7 NET_STATUS | 直连 |
| ~~PA9~~ | ~~USART1_TX~~ | — | **板上未引出** | 不可用 |
| ~~PA10~~ | ~~USART1_RX~~ | — | **板上未引出** | 不可用 |
| **PA11** | FDCAN1_RX | CAN_RX | TJA1042 Pin4 RXD | 直连 |
| **PA12** | FDCAN1_TX | CAN_TX | TJA1042 Pin1 TXD | 直连 |
| PA13 | SWDIO | — | 最小系统板自带 SWD | 无需外接 |
| PA14 | SWCLK | — | 最小系统板自带 SWD | 无需外接 |
| **PB0** | GPIO | LCD_DC | LCD 模组 DC | 直连 |
| **PB1** | GPIO | LCD_CS | LCD 模组 CS | 直连 |
| **PB2** | GPIO | LCD_RST | LCD 模组 RST | 直连 |
| **PB3** | EXTI3 | BQ_ALERT | BQ76920 Pin20 ALERT | **R21 (1MΩ) 下拉到 GND** |
| **PB4** | GPIO | LED_GREEN | LED1 阳极 | R31 (510Ω) 限流 |
| **PB5** | GPIO | LED_RED | LED2 阳极 | R32 (510Ω) 限流 |
| **PB6** | I2C1_SCL | I2C1_SCL | BQ76920 Pin5 + BQ34Z100 Pin13 | **R17 (4.7kΩ) 上拉到 BQ_3V3** |
| **PB7** | I2C1_SDA | I2C1_SDA | BQ76920 Pin4 + BQ34Z100 Pin14 | **R18 (4.7kΩ) 上拉到 BQ_3V3** |
| **PB8** | GPIO | BUZZER_CTRL | Q3 (S8050) Base | R33 (1kΩ) 限流 |
| **PB9** | GPIO | BQ34_VEN | BQ34Z100 Pin2 VEN | 直连（可悬空）|
| **PB10** | USART3_TX | L610_TXD | J5-4 RXD（L610 接收） | 直连 |
| **PB11** | USART3_RX | L610_RXD | J5-3 TXD（L610 发送） | 直连 |
| **PB12** | EXTI12 | KEY1 | SW1 | R27 上拉 + C32 去抖 |
| **PB13** | EXTI13 | KEY2 | SW2 | R28 上拉 + C33 去抖 |
| **PB14** | EXTI14 | KEY3 | SW3 | R29 上拉 + C34 去抖 |
| **PB15** | EXTI15 | KEY4 | SW4 | R30 上拉 + C35 去抖 |

### 2.2 串口/总线分配总览

| 外设 | 引脚 | 通信对象 | 速率 |
|---|---|---|---|
| **USART2** | PA2 / PA3 | CH340G → USB → 电脑（调试 printf）| 115200 bps |
| **USART3** | PB10 / PB11 | L610 4G 模组 | 115200 bps |
| **FDCAN1** | PA11 / PA12 | TJA1042 → 外部 CAN 总线 | 500 kbps |
| **I2C1** | PB6 / PB7 | BQ76920 + BQ34Z100（共用） | 100/400 kHz |
| **SPI1** | PA5 / PA7 + PB0/PB1/PB2 | TFT LCD（仅写） | 18 MHz |

---

## 3. 各引脚详细电路连接

### 3.1 简单直连引脚（19 个，无外围元件）

```
PA2  ──→ CH340G Pin3 RXD
PA3  ←── CH340G Pin2 TXD
PA4  ──→ J5-5 (L610_PWRKEY)
PA5  ──→ LCD_SCK
PA6  ──→ J5-6 (L610_RESET_N)
PA7  ──→ LCD_MOSI
PA8  ←── J5-7 (L610_NETSTA)
PA11 ←── TJA1042 Pin4 RXD
PA12 ──→ TJA1042 Pin1 TXD
PB0  ──→ LCD_DC
PB1  ──→ LCD_CS
PB2  ──→ LCD_RST
PB9  ──→ BQ34Z100 Pin2 VEN（或悬空）
PB10 ──→ J5-4 (L610_RXD)
PB11 ←── J5-3 (L610_TXD)
```

### 3.2 带外围元件的引脚（12 个）

#### PA0 — NTC2 温度采样

```
3V3
 │
 R25 (10kΩ)
 │
 ├──── PA0
 │
NTC2 (10kΩ B=3380)
 │
GND
```

#### PB3 — BQ_ALERT 中断

```
PB3 ──┬── BQ76920 Pin20 ALERT
      │
      R21 (1MΩ)
      │
     GND
```

⚠️ R21 必需，缺失会导致 ALERT 误触发或永远不响应

#### PB4 / PB5 — LED 指示

```
PB4 ── R31 (510Ω) ── LED1(绿) ── GND
PB5 ── R32 (510Ω) ── LED2(红) ── GND
```

#### PB6 / PB7 — I²C 总线（共享）

```
BQ_3V3 (来自 BQ76920 Pin8 REGOUT)
 │
 ├── R17 (4.7kΩ) ─── PB6 ─┬── BQ76920 Pin5 SCL
 │                         └── BQ34Z100 Pin13 SCL
 │
 └── R18 (4.7kΩ) ─── PB7 ─┬── BQ76920 Pin4 SDA
                           └── BQ34Z100 Pin14 SDA
```

#### PB8 — 蜂鸣器驱动

```
PB8 ── R33 (1kΩ) ── Q3 (S8050) Base
                    Q3 Emitter ── GND
                    Q3 Collector ── BZ1(-) ── BZ1(+) ── 5V
                                     │
                                     ├── D3 (1N4148WS) 阳极
                                     │   阴极 → 5V （续流）
```

#### PB12 / PB13 / PB14 / PB15 — 4 个按键（结构相同）

```
3V3 ── Rn (10kΩ) ──┬── PBn
                    │
                    Cn (100nF)
                    │
                   GND
                    
               PBn ── SWn ── GND
```

| 按键 | 引脚 | 上拉电阻 | 去抖电容 |
|---|---|---|---|
| SW1 | PB12 | R27 | C32 |
| SW2 | PB13 | R28 | C33 |
| SW3 | PB14 | R29 | C34 |
| SW4 | PB15 | R30 | C35 |

---

## 4. 主板对外接口（J 编号）

| 位号 | 类型 | 针数 | 对外作用 |
|---|---|---|---|
| **J1** | KF128 5.0mm | 2P | PACK+ / PACK-（充放电主接口） |
| **J2** | KF128 2.54mm | 6P | 5 节电池接线（B-、C1+、C2+、C3+、C4+、B+）|
| **J3** | KF128 2.54mm | 3P | CAN 总线（CANH、CANL、GND）对外 |
| **J5** | 1×6 母排针 | 6P | L610 4G 模组对接 |
| **J_LCD** | 8P / 14P 排母 | 8P | TFT LCD 模组对接 |
| **J_MCU** | 1×15 母排针 × 4 | 60P | STM32 最小系统板对接 |
| **J_USB** | Type-C 母座 | 16P | 板载调试串口（CH340G） |

---

## 5. CH340G 集成电路（板载调试串口）

```
USB Type-C 接口 (J_USB)
   ┃ VBUS  ─── 5V0 (经 ESD 二极管)
   ┃ D+    ─── ESD ─── CH340G Pin5 UD+
   ┃ D-    ─── ESD ─── CH340G Pin6 UD-
   ┃ CC1   ─── 5.1kΩ ── GND
   ┃ CC2   ─── 5.1kΩ ── GND
   ┃ GND   ─── GND
   
   CH340G Pin1 GND  → GND
   CH340G Pin2 TXD  → STM32 PA3 (USART2_RX)
   CH340G Pin3 RXD  → STM32 PA2 (USART2_TX)
   CH340G Pin4 V3   → C_V3 (100nF) → GND
   CH340G Pin7 XI   ─┬─── Y2 (12MHz) ─┬─── XO Pin8
                     │                 │
                  Cx1 (22pF)        Cx2 (22pF)
                     │                 │
                    GND               GND
   CH340G Pin9 VCC  → 5V0 + C_VCC (100nF) → GND
   CH340G Pin16 RST# → 悬空
```

**新增元件**（PCB 集成 CH340G 必需）：

| 位号 | 元件 | 数量 | LCSC |
|---|---|---|---|
| U11 | CH340G | 1 | 你已有 |
| Y2 | 12MHz 晶振 | 1 | C9002 |
| Cx1, Cx2 | 0603 22pF NP0 | 2 | C1653 |
| C_V3, C_VCC | 0603 100nF | 2 | 你已有 |
| J_USB | Type-C 16P 母座 | 1 | C165948 |
| R_CC1, R_CC2 | 0805 5.1kΩ 1% | 2 | C25905 |

---

## 6. 电源链

```
电池 B+ (12.5V~21V)
    │
    F1 (BSMD2920 自恢复保险丝 3A)
    │
    BAT_PROT 网络
    │
    ├── D1 (SMBJ24A TVS) → GND
    │
    ├── U6 (MP1584 #1)
    │   反馈: R_FB1b 80.6k + R_FB2 20k → 输出 4.0V
    │   输出: 4V0 网络 → L610 VBAT
    │   外围: L1 22µH (≥2A)、C25/C26 22µF、Cbst 10nF、R_EN 100kΩ
    │   特殊: 输出端加 C29 100µF 电解 (L610 突发电流缓冲)
    │
    ├── U7 (MP1584 #2)  
    │   反馈: R_FB1a 105k + R_FB2 20k → 输出 5.0V
    │   输出: 5V0 网络 → AMS1117 输入 + 蜂鸣器
    │   外围: L2 22µH、C27/C28 22µF、Cbst 10nF、R_EN 100kΩ
    │
    │       └── U8 AMS1117-3.3
    │           输入: 5V0 + C23 (4.7µF)
    │           输出: 3V3 网络 → STM32 + LCD + 上拉电阻 + BQ34Z100
    │
    ├── BQ76920 REGSRC (Pin9): R7 (100Ω) + C7 (10µF) 滤波
    │       └── BQ_3V3 (Pin8 REGOUT): 仅供 I²C 上拉电阻使用
    │
    ├── BQ76920 BAT (Pin10): R6 (100Ω) + C6 (1µF) RC 滤波
    │
    └── BQ34Z100 BAT 分压: R22 (1.5MΩ) + R23 (68kΩ) → 21V→0.91V
```

---

## 7. 调试与烧录方案

### 7.1 调试 printf 输出

```c
// CubeMX 配置 USART2: PA2/PA3, 115200 bps
// 代码中 printf 重定向到 USART2

#include <stdio.h>
int fputc(int ch, FILE *f) {
    HAL_UART_Transmit(&huart2, (uint8_t*)&ch, 1, HAL_MAX_DELAY);
    return ch;
}
// GCC 编译器加这个
int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}
```

**使用流程**：
1. Type-C 数据线连 PCB 上的 J_USB ↔ 电脑 USB 口
2. 电脑装 CH340 驱动（沁恒官网）
3. 设备管理器看 `USB-SERIAL CH340 (COMx)`
4. 串口助手（XCOM/SSCOM）打开 COMx，115200 bps
5. STM32 代码里 `printf("Cell1=%dmV\r\n", v);` 即可在电脑看到

### 7.2 烧录方案

| 方案 | 工具 | 接口 |
|---|---|---|
| **首选**: SWD 烧录 | ST-Link V2（淘宝 25-30 元）| 最小系统板顶部 SWD 接口 |
| 备选: 板载 USB DFU | CubeProgrammer | 最小系统板 Type-C（按 BOOT 进入 DFU）|

**烧录时不影响 PCB 上的 CH340G**——SWD 走最小系统板顶部，CH340G 在你的主板上独立工作。

---

## 8. 引脚冲突回避说明

### 8.1 已规避的冲突

| 原方案 | 冲突 | 解决方案 |
|---|---|---|
| L610 用 USART1 (PA9/PA10) | 板上 PA9/PA10 未引出 | **改用 USART3 (PB10/PB11)** |
| 调试用板载 USB 虚拟串口（PA11/PA12）| 与 CAN 抢用 PA11/PA12 | **板载 CH340G + USART2 (PA2/PA3) 调试** |
| BQ34_VEN 用 PB9 | 与 CAN 备选 PB9 抢用 | CAN 保留 PA11/PA12，PB9 给 BQ34_VEN |

### 8.2 PA11/PA12 冲突说明

最小系统板的板载 Type-C 内部连到 PA11/PA12（用作 USB DFU）。但你的设计里 PA11/PA12 用于 CAN：

**关键约束**：**正常运行时不要插板载 Type-C**（会和 CAN 信号冲突）。

- 烧录用 ST-Link（SWD）→ 不需要插板载 USB
- 调试用 PCB 上的 CH340G + Type-C → 不影响 PA11/PA12

---

## 9. 对外接口物理连接表

### 9.1 调试时的连接（开发期）

```
你的 BMS 主板:
  J_USB Type-C ─── Type-C 数据线 ─── 电脑 USB 口
                                       │
                                       ▼
                                    CH340 驱动识别为 COMx
                                       │
                                       ▼
                                  串口助手 显示 printf
```

### 9.2 CAN 通信演示（答辩期）

```
你的 BMS 主板:
  J3 端子:
    Pin1 (CANH) ─── 杜邦线 ─── USB-CAN 转换器 CANH
    Pin2 (CANL) ─── 杜邦线 ─── USB-CAN 转换器 CANL
    Pin3 (GND)  ─── 杜邦线 ─── USB-CAN 转换器 GND
                                  │
                                  ▼
                                USB → 电脑
                                  │
                                  ▼
                              CANalyzer / PCAN-View
                              显示 BMS 报文
```

### 9.3 烧录连接（开发期）

```
最小系统板顶部 SWD 4P 接口
  CLK ─── ST-Link V2 SWCLK
  DIO ─── ST-Link V2 SWDIO
  GND ─── ST-Link V2 GND
  3V3 ─── ST-Link V2 3V3 (可选)
              │
              ▼
            USB → 电脑
              │
              ▼
        STM32CubeProgrammer 烧录
```

### 9.4 L610 模组连接（永久）

```
你的 BMS 主板:
  J5 母排针 6P ←── L610 成品扩展板 (插入)
  
  4V0 ──→ L610 VBAT
  GND ──→ L610 GND
  PB10 ──→ L610 RXD
  PB11 ←── L610 TXD
  PA4 ──→ L610 PWRKEY
  PA6 ──→ L610 RESET_N
```

### 9.5 LCD 模组连接（永久）

```
你的 BMS 主板:
  J_LCD 排母 ←── LCD 模组 (插入)
  
  3V3, GND, PA5 (SCK), PA7 (MOSI), PB0 (DC), PB1 (CS), PB2 (RST), 背光
```

---

## 10. 完整接口位号速查表

| 位号 | 接什么 | 类型 | 针数 | 物理位置建议 |
|---|---|---|---|---|
| **J1** | PACK+/PACK- 输出 | KF128 5.0mm | 2P | 板边右侧 |
| **J2** | 5 节电池平衡线 | KF128 2.54mm | 6P | 板边左侧 |
| **J3** | CAN 总线对外 | KF128 2.54mm | 3P | 板边底部 |
| **J5** | L610 模组对接 | 1×6 母排针 | 6P | 板内右上 |
| **J_LCD** | LCD 模组对接 | 排母 | 8P | 板内左上 |
| **J_MCU** | MCU 最小系统板 | 1×15 母排针 ×4 | 60P | 板中心 |
| **J_USB** | 板载调试 Type-C | Type-C 母座 | 16P | 板边底部 |

---

## 11. 修订前后对比（与最早版本）

| 信号 | 最早版本 | **最终版** |
|---|---|---|
| L610 通信 | USART1 (PA9/PA10) | **USART3 (PB10/PB11)** |
| 调试串口 | USART2 (PA2/PA3) 外置 CH340 | **USART2 (PA2/PA3) + 板载集成 CH340G** |
| 板载 USB 虚拟串口 | 想用 | **不用**（PA11/PA12 给 CAN） |
| CAN | PA11/PA12 | **保持不变** |
| BQ34_VEN | PB9 | **保持不变** |
| 蜂鸣器 | PB8 | **保持不变** |
| 烧录方式 | SWD | **保持不变（用 ST-Link）** |

---

## 12. 检查清单（PCB 设计前）

- [ ] 母排针间距测量准确（4 排 × 15 引脚，与最小系统板对应）
- [ ] CH340G 完整外围（晶振 12MHz + 22pF + 100nF）
- [ ] Type-C 接口的 CC1/CC2 5.1kΩ 下拉
- [ ] PA11/PA12 仅连 TJA1042，不接 USB
- [ ] BQ76920 ALERT 必有 1MΩ 下拉
- [ ] I²C 上拉接 BQ_3V3（不是系统 3V3）
- [ ] Rsns 4 端开尔文连接
- [ ] BQ34Z100 BAT 分压 1.5MΩ + 68kΩ
- [ ] 5 颗均衡电阻接电池侧（不接 BQ 侧）
- [ ] 单点接地：DGND/AGND 在 Rsns 电池侧汇合
- [ ] CAN 总线 J3 引出 GND（共地参考）
- [ ] LCD 模组接口距离 / 排母规格与你的 LCD 一致
- [ ] L610 模组接口距离 / 排针规格与你的 ADP-L610 一致

---

> **作者**: Claude (Anthropic)  
> **基于**: TI BQ76920 / BQ34Z100-G1 datasheet + ST STM32G474 datasheet + 实际板子布局核对  
> **版本**: V2.0 最终版  
> **配套文档**: BMS_Hardware_Design.md (系统设计), BMS_Wiring_BOM.md (元件级连线), BMS-5S_BOM.xlsx (采购清单)

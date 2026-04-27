import { useState } from "react";

const peripherals = [
  {
    name: "I2C1 — BQ76920通信",
    icon: "🔌",
    config: {
      "外设": "I2C1",
      "模式": "I2C Master",
      "速率": "100KHz (Standard Mode)",
      "SCL引脚": "PB6 (AF4)",
      "SDA引脚": "PB7 (AF4)",
      "从机地址": "0x18 (BQ7692003)",
      "地址模式": "7-bit",
      "上拉电阻": "外部4.7KΩ到3.3V",
    },
    notes: "BQ76920通过I2C读写寄存器，采集电压/电流/温度，配置保护阈值和均衡控制。250ms轮询一次。",
  },
  {
    name: "USART1 — L610模组",
    icon: "📡",
    config: {
      "外设": "USART1",
      "波特率": "115200 bps",
      "数据位": "8",
      "停止位": "1",
      "校验": "None",
      "TX引脚": "PA9 (AF7)",
      "RX引脚": "PA10 (AF7)",
      "模式": "异步 + DMA接收",
      "DMA": "DMA1_Channel3 (RX)",
    },
    notes: "通过AT指令控制L610 4G模组，MQTT协议上传电池数据到云平台。DMA+空闲中断接收不定长数据。",
  },
  {
    name: "USART2 — 调试串口",
    icon: "🖥️",
    config: {
      "外设": "USART2",
      "波特率": "115200 bps",
      "数据位": "8",
      "停止位": "1",
      "校验": "None",
      "TX引脚": "PA2 (AF7)",
      "RX引脚": "PA3 (AF7)",
      "模式": "异步",
      "功能": "printf重定向调试输出",
    },
    notes: "连接USB转TTL模块，用于开发调试阶段的数据打印和指令交互。",
  },
  {
    name: "FDCAN1 — CAN总线",
    icon: "🚗",
    config: {
      "外设": "FDCAN1",
      "模式": "Classic CAN 2.0B",
      "波特率": "500Kbps",
      "TX引脚": "PA12 (AF9)",
      "RX引脚": "PA11 (AF9)",
      "外接收发器": "TJA1050 模块",
      "帧格式": "标准帧 (11-bit ID)",
      "FIFO": "RX FIFO0, 深度8",
      "滤波": "ID=0x100-0x1FF 范围过滤",
    },
    notes: "上报电池组状态数据，接收上位机控制指令。协议自定义：0x100电压帧，0x101电流温度帧，0x102 SOC状态帧。",
  },
  {
    name: "SPI1 — TFT LCD屏",
    icon: "📺",
    config: {
      "外设": "SPI1",
      "模式": "Master, Full-Duplex",
      "速率": "18MHz (Prescaler=4)",
      "CPOL": "0 (空闲低电平)",
      "CPHA": "0 (第一边沿采样)",
      "SCK引脚": "PA5 (AF5)",
      "MOSI引脚": "PA7 (AF5)",
      "DC引脚": "PB0 (GPIO Output)",
      "CS引脚": "PB1 (GPIO Output)",
      "RST引脚": "PB2 (GPIO Output)",
      "驱动IC": "ST7735 / ILI9341",
    },
    notes: "驱动1.8寸或2.4寸TFT屏，运行LVGL图形库显示电池电压柱状图、SOC仪表盘、告警信息。",
  },
  {
    name: "ADC1 — 模拟采集",
    icon: "📊",
    config: {
      "外设": "ADC1",
      "分辨率": "12-bit",
      "采样时间": "47.5 cycles",
      "触发": "TIM3 TRGO (1Hz)",
      "通道1": "PA0 (IN1) — NTC温度2",
      "通道2": "PA1 (IN2) — NTC温度3",
      "通道16": "内部温度传感器",
      "DMA": "DMA1_Channel1",
      "模式": "扫描+连续+DMA循环",
    },
    notes: "BQ76920只支持1路NTC，额外2路NTC通过MCU ADC采集，监测电池组不同位置温度。内部温度传感器监测MCU自身温度。",
  },
  {
    name: "TIM2 — 系统定时基准",
    icon: "⏱️",
    config: {
      "外设": "TIM2",
      "时钟源": "APB1 (170MHz)",
      "预分频": "17000-1",
      "计数周期": "10000-1",
      "中断周期": "1s",
      "用途": "SOC计算定时触发",
    },
    notes: "1秒中断触发SOC安时积分计算，同时作为系统时间戳基准。",
  },
  {
    name: "TIM3 — ADC触发",
    icon: "🔄",
    config: {
      "外设": "TIM3",
      "时钟源": "APB1 (170MHz)",
      "预分频": "17000-1",
      "计数周期": "10000-1",
      "TRGO": "Update Event",
      "用途": "触发ADC1定时采样",
    },
    notes: "每秒触发一次ADC采样温度数据，避免CPU轮询浪费资源。",
  },
  {
    name: "GPIO — 开关与指示",
    icon: "💡",
    config: {
      "ALERT输入": "PB3 — 外部中断(EXTI3) 上升沿",
      "LED指示-正常": "PB4 — 推挽输出 绿色",
      "LED指示-告警": "PB5 — 推挽输出 红色",
      "蜂鸣器": "PB8 — 推挽输出",
      "按键1-模式": "PB12 — 上拉输入+EXTI",
      "按键2-确认": "PB13 — 上拉输入+EXTI",
      "按键3-上": "PB14 — 上拉输入+EXTI",
      "按键4-下": "PB15 — 上拉输入+EXTI",
    },
    notes: "ALERT引脚最高优先级，BQ76920检测到故障时立即触发MCU中断进入保护处理。",
  },
];

const tasks = [
  {
    name: "采集任务",
    priority: 3,
    stack: "512B",
    period: "250ms",
    color: "#2563eb",
    desc: "I2C读取BQ76920电压/电流/温度数据，存入全局数据结构",
    detail: [
      "每250ms通过I2C读取5节电芯电压",
      "读取库仑计数器计算充放电电流",
      "读取NTC温度（AFE 1路+ADC 2路）",
      "数据写入FreeRTOS队列供其他任务使用",
    ],
  },
  {
    name: "保护任务",
    priority: 5,
    stack: "256B",
    period: "事件驱动",
    color: "#dc2626",
    desc: "最高优先级，ALERT中断触发或定时检查，执行保护动作",
    detail: [
      "ALERT中断→信号量→立即执行",
      "过压/欠压：关断充电/放电MOS",
      "过流/短路：立即关断+蜂鸣器报警",
      "过温：降低均衡电流或停止充放电",
      "传感器断线检测（电压读0或满量程）",
      "MOS粘连检测（关断后检查电流是否归零）",
    ],
  },
  {
    name: "均衡任务",
    priority: 2,
    stack: "256B",
    period: "1s",
    color: "#16a34a",
    desc: "基于SOC差异的动态被动均衡策略",
    detail: [
      "计算5节电芯的电压极差",
      "极差>30mV时启动均衡",
      "对最高电压电芯开启均衡放电",
      "相邻电芯不同时均衡（硬件限制）",
      "充电/静置状态才执行，放电时禁止",
    ],
  },
  {
    name: "SOC计算任务",
    priority: 2,
    stack: "512B",
    period: "1s",
    color: "#9333ea",
    desc: "安时积分法+OCV校准的融合SOC估算",
    detail: [
      "安时积分：SOC += I×Δt/Qn",
      "静置>30min时用OCV查表校准SOC",
      "电流<50mA判定为静置状态",
      "满充(电流<截止值)时SOC校准为100%",
      "记录累计充放电安时数用于SOH估算",
    ],
  },
  {
    name: "通信任务",
    priority: 1,
    stack: "1024B",
    period: "5s",
    color: "#ea580c",
    desc: "CAN总线上报+L610 4G远程上传",
    detail: [
      "每1s发送CAN帧（电压/电流/SOC）",
      "每5s通过L610 MQTT上传云平台",
      "JSON格式：{v:[3.8,3.9,...],i:1.2,soc:85,t:25}",
      "接收CAN/云端下发的控制指令",
      "通信超时检测与自动重连",
    ],
  },
  {
    name: "显示任务",
    priority: 1,
    stack: "1024B",
    period: "500ms",
    color: "#0891b2",
    desc: "LVGL界面刷新，显示电池状态",
    detail: [
      "主界面：5节电压柱状图+总SOC仪表盘",
      "详情页：电流/温度/均衡状态",
      "告警页：故障代码+时间戳",
      "按键切换页面，长按进入设置",
      "lv_timer驱动LVGL心跳（5ms）",
    ],
  },
];

const stateMachine = [
  { state: "初始化", next: "预充", condition: "上电自检通过", color: "#6b7280" },
  { state: "预充", next: "待机", condition: "预充完成/电压正常", color: "#eab308" },
  { state: "待机", next: "充电/放电", condition: "检测到充电器/负载", color: "#3b82f6" },
  { state: "充电", next: "待机", condition: "满充/充电器拔出", color: "#22c55e" },
  { state: "放电", next: "待机", condition: "负载断开/SOC过低", color: "#22c55e" },
  { state: "告警", next: "待机/故障", condition: "告警消除→待机/持续→故障", color: "#f97316" },
  { state: "故障", next: "初始化", condition: "手动复位/ALERT清除", color: "#ef4444" },
];

const clockConfig = {
  "HSE": "8MHz 外部晶振",
  "PLL": "HSE→PLL→SYSCLK",
  "SYSCLK": "170MHz",
  "AHB": "170MHz",
  "APB1": "170MHz",
  "APB2": "170MHz",
  "FreeRTOS Tick": "1000Hz (1ms)",
  "功耗模式": "运行/Sleep（空闲任务进入Sleep）",
};

const pinMap = [
  { pin: "PA0", func: "ADC1_IN1", use: "NTC温度2" },
  { pin: "PA1", func: "ADC1_IN2", use: "NTC温度3" },
  { pin: "PA2", func: "USART2_TX", use: "调试串口TX" },
  { pin: "PA3", func: "USART2_RX", use: "调试串口RX" },
  { pin: "PA5", func: "SPI1_SCK", use: "LCD时钟" },
  { pin: "PA7", func: "SPI1_MOSI", use: "LCD数据" },
  { pin: "PA9", func: "USART1_TX", use: "L610 TX" },
  { pin: "PA10", func: "USART1_RX", use: "L610 RX" },
  { pin: "PA11", func: "FDCAN1_RX", use: "CAN接收" },
  { pin: "PA12", func: "FDCAN1_TX", use: "CAN发送" },
  { pin: "PB0", func: "GPIO_OUT", use: "LCD DC引脚" },
  { pin: "PB1", func: "GPIO_OUT", use: "LCD CS引脚" },
  { pin: "PB2", func: "GPIO_OUT", use: "LCD RST引脚" },
  { pin: "PB3", func: "EXTI3", use: "BQ76920 ALERT" },
  { pin: "PB4", func: "GPIO_OUT", use: "LED绿" },
  { pin: "PB5", func: "GPIO_OUT", use: "LED红" },
  { pin: "PB6", func: "I2C1_SCL", use: "BQ76920 SCL" },
  { pin: "PB7", func: "I2C1_SDA", use: "BQ76920 SDA" },
  { pin: "PB8", func: "GPIO_OUT", use: "蜂鸣器" },
  { pin: "PB12", func: "EXTI12", use: "按键1-模式" },
  { pin: "PB13", func: "EXTI13", use: "按键2-确认" },
  { pin: "PB14", func: "EXTI14", use: "按键3-上" },
  { pin: "PB15", func: "EXTI15", use: "按键4-下" },
];

export default function BMSDesign() {
  const [activeTab, setActiveTab] = useState("arch");
  const [expandedPeripheral, setExpandedPeripheral] = useState(null);
  const [expandedTask, setExpandedTask] = useState(null);

  const tabs = [
    { id: "arch", label: "系统架构" },
    { id: "periph", label: "外设配置" },
    { id: "tasks", label: "RTOS任务" },
    { id: "state", label: "状态机" },
    { id: "pins", label: "引脚分配" },
    { id: "clock", label: "时钟树" },
  ];

  return (
    <div style={{ fontFamily: "'Noto Sans SC', 'Source Han Sans', sans-serif", background: "#0f172a", color: "#e2e8f0", minHeight: "100vh", padding: "20px" }}>
      <div style={{ maxWidth: 900, margin: "0 auto" }}>
        {/* Header */}
        <div style={{ textAlign: "center", marginBottom: 28, padding: "24px 0", borderBottom: "2px solid #1e3a5f" }}>
          <h1 style={{ fontSize: 22, fontWeight: 700, color: "#60a5fa", margin: 0, letterSpacing: 1 }}>
            BMS 软件工程设计
          </h1>
          <p style={{ color: "#94a3b8", fontSize: 13, margin: "8px 0 0" }}>
            STM32G431RBT6 | FreeRTOS | BQ76920 AFE | CAN + 4G远程
          </p>
        </div>

        {/* Tabs */}
        <div style={{ display: "flex", gap: 4, marginBottom: 20, flexWrap: "wrap" }}>
          {tabs.map((t) => (
            <button
              key={t.id}
              onClick={() => setActiveTab(t.id)}
              style={{
                padding: "8px 16px",
                border: "none",
                borderRadius: 6,
                fontSize: 13,
                fontWeight: 600,
                cursor: "pointer",
                background: activeTab === t.id ? "#2563eb" : "#1e293b",
                color: activeTab === t.id ? "#fff" : "#94a3b8",
                transition: "all 0.2s",
              }}
            >
              {t.label}
            </button>
          ))}
        </div>

        {/* Architecture */}
        {activeTab === "arch" && (
          <div>
            <div style={{ background: "#1e293b", borderRadius: 10, padding: 20, marginBottom: 16 }}>
              <h3 style={{ color: "#60a5fa", fontSize: 15, margin: "0 0 16px" }}>系统架构总览</h3>
              <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr 1fr", gap: 12, textAlign: "center" }}>
                {/* Sensors */}
                <div style={{ background: "#0f172a", borderRadius: 8, padding: 14, border: "1px solid #334155" }}>
                  <div style={{ fontSize: 24, marginBottom: 6 }}>🔋</div>
                  <div style={{ color: "#fbbf24", fontWeight: 700, fontSize: 13 }}>感知层</div>
                  <div style={{ color: "#94a3b8", fontSize: 11, marginTop: 6, lineHeight: 1.6 }}>
                    BQ76920 AFE<br />NTC×3<br />采样电阻
                  </div>
                </div>
                {/* MCU */}
                <div style={{ background: "#172554", borderRadius: 8, padding: 14, border: "2px solid #2563eb" }}>
                  <div style={{ fontSize: 24, marginBottom: 6 }}>⚡</div>
                  <div style={{ color: "#60a5fa", fontWeight: 700, fontSize: 13 }}>控制层 (G431)</div>
                  <div style={{ color: "#94a3b8", fontSize: 11, marginTop: 6, lineHeight: 1.6 }}>
                    FreeRTOS 6任务<br />SOC算法<br />保护状态机<br />均衡策略
                  </div>
                </div>
                {/* Communication */}
                <div style={{ background: "#0f172a", borderRadius: 8, padding: 14, border: "1px solid #334155" }}>
                  <div style={{ fontSize: 24, marginBottom: 6 }}>🌐</div>
                  <div style={{ color: "#34d399", fontWeight: 700, fontSize: 13 }}>通信层</div>
                  <div style={{ color: "#94a3b8", fontSize: 11, marginTop: 6, lineHeight: 1.6 }}>
                    CAN 500K<br />L610 4G MQTT<br />UART调试
                  </div>
                </div>
              </div>
              {/* Data flow */}
              <div style={{ marginTop: 16, padding: 12, background: "#0f172a", borderRadius: 8, border: "1px dashed #334155" }}>
                <div style={{ color: "#94a3b8", fontSize: 11, textAlign: "center", lineHeight: 2 }}>
                  <span style={{ color: "#fbbf24" }}>电池组5S1P</span> → <span style={{ color: "#f97316" }}>BQ76920(I2C)</span> → <span style={{ color: "#60a5fa" }}>采集任务</span> → <span style={{ color: "#a78bfa" }}>SOC/保护/均衡</span> → <span style={{ color: "#34d399" }}>CAN+4G上报</span> → <span style={{ color: "#0891b2" }}>LCD显示</span>
                </div>
              </div>
            </div>

            <div style={{ background: "#1e293b", borderRadius: 10, padding: 20 }}>
              <h3 style={{ color: "#60a5fa", fontSize: 15, margin: "0 0 12px" }}>软件分层架构</h3>
              {[
                { layer: "应用层", color: "#8b5cf6", items: "SOC算法 | 均衡策略 | 保护状态机 | 故障诊断" },
                { layer: "服务层", color: "#3b82f6", items: "FreeRTOS任务调度 | 队列/信号量 | 软件定时器" },
                { layer: "驱动层", color: "#06b6d4", items: "BQ76920驱动 | CAN驱动 | L610 AT驱动 | LCD驱动 | ADC驱动" },
                { layer: "HAL层", color: "#10b981", items: "STM32 HAL库 | I2C | SPI | USART | FDCAN | ADC | TIM | GPIO" },
                { layer: "硬件层", color: "#6b7280", items: "STM32G431RBT6 | BQ76920 | TJA1050 | L610 | ST7735 | NTC" },
              ].map((l, i) => (
                <div key={i} style={{ display: "flex", alignItems: "center", marginBottom: 6 }}>
                  <div style={{ width: 70, fontSize: 11, fontWeight: 700, color: l.color, textAlign: "right", paddingRight: 12, flexShrink: 0 }}>
                    {l.layer}
                  </div>
                  <div style={{ flex: 1, background: l.color + "20", border: `1px solid ${l.color}50`, borderRadius: 6, padding: "8px 12px", fontSize: 11, color: "#cbd5e1" }}>
                    {l.items}
                  </div>
                </div>
              ))}
            </div>
          </div>
        )}

        {/* Peripherals */}
        {activeTab === "periph" && (
          <div style={{ display: "flex", flexDirection: "column", gap: 10 }}>
            {peripherals.map((p, i) => (
              <div
                key={i}
                style={{ background: "#1e293b", borderRadius: 10, overflow: "hidden", border: expandedPeripheral === i ? "1px solid #2563eb" : "1px solid #334155" }}
              >
                <div
                  onClick={() => setExpandedPeripheral(expandedPeripheral === i ? null : i)}
                  style={{ padding: "12px 16px", cursor: "pointer", display: "flex", justifyContent: "space-between", alignItems: "center" }}
                >
                  <div style={{ display: "flex", alignItems: "center", gap: 10 }}>
                    <span style={{ fontSize: 18 }}>{p.icon}</span>
                    <span style={{ fontWeight: 700, fontSize: 13, color: "#e2e8f0" }}>{p.name}</span>
                  </div>
                  <span style={{ color: "#64748b", fontSize: 12 }}>{expandedPeripheral === i ? "▲" : "▼"}</span>
                </div>
                {expandedPeripheral === i && (
                  <div style={{ padding: "0 16px 14px" }}>
                    <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr", gap: 4, marginBottom: 10 }}>
                      {Object.entries(p.config).map(([k, v]) => (
                        <div key={k} style={{ display: "flex", fontSize: 11, padding: "4px 0", borderBottom: "1px solid #1e293b" }}>
                          <span style={{ color: "#64748b", width: 100, flexShrink: 0 }}>{k}</span>
                          <span style={{ color: "#fbbf24", fontFamily: "monospace", fontSize: 11 }}>{v}</span>
                        </div>
                      ))}
                    </div>
                    <div style={{ fontSize: 11, color: "#94a3b8", background: "#0f172a", padding: 10, borderRadius: 6, lineHeight: 1.6 }}>
                      💡 {p.notes}
                    </div>
                  </div>
                )}
              </div>
            ))}
          </div>
        )}

        {/* Tasks */}
        {activeTab === "tasks" && (
          <div style={{ display: "flex", flexDirection: "column", gap: 10 }}>
            <div style={{ background: "#1e293b", borderRadius: 10, padding: 16, marginBottom: 4 }}>
              <h3 style={{ color: "#60a5fa", fontSize: 14, margin: "0 0 8px" }}>FreeRTOS配置</h3>
              <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr 1fr", gap: 8, fontSize: 11 }}>
                <div><span style={{ color: "#64748b" }}>内核: </span><span style={{ color: "#fbbf24" }}>CMSIS-RTOS V2</span></div>
                <div><span style={{ color: "#64748b" }}>Tick: </span><span style={{ color: "#fbbf24" }}>1ms (1000Hz)</span></div>
                <div><span style={{ color: "#64748b" }}>堆大小: </span><span style={{ color: "#fbbf24" }}>8KB (heap_4)</span></div>
                <div><span style={{ color: "#64748b" }}>最大优先级: </span><span style={{ color: "#fbbf24" }}>7</span></div>
                <div><span style={{ color: "#64748b" }}>空闲钩子: </span><span style={{ color: "#fbbf24" }}>进入Sleep省电</span></div>
                <div><span style={{ color: "#64748b" }}>队列: </span><span style={{ color: "#fbbf24" }}>3个 (采集/告警/通信)</span></div>
              </div>
            </div>
            {tasks.map((t, i) => (
              <div
                key={i}
                style={{ background: "#1e293b", borderRadius: 10, overflow: "hidden", borderLeft: `4px solid ${t.color}` }}
              >
                <div
                  onClick={() => setExpandedTask(expandedTask === i ? null : i)}
                  style={{ padding: "12px 16px", cursor: "pointer", display: "flex", justifyContent: "space-between", alignItems: "center" }}
                >
                  <div>
                    <span style={{ fontWeight: 700, fontSize: 13, color: t.color }}>{t.name}</span>
                    <span style={{ fontSize: 11, color: "#64748b", marginLeft: 10 }}>优先级{t.priority} | {t.period} | {t.stack}</span>
                  </div>
                  <span style={{ color: "#64748b", fontSize: 12 }}>{expandedTask === i ? "▲" : "▼"}</span>
                </div>
                {expandedTask === i && (
                  <div style={{ padding: "0 16px 14px" }}>
                    <p style={{ fontSize: 11, color: "#94a3b8", margin: "0 0 8px" }}>{t.desc}</p>
                    <div style={{ background: "#0f172a", borderRadius: 6, padding: 10 }}>
                      {t.detail.map((d, j) => (
                        <div key={j} style={{ fontSize: 11, color: "#cbd5e1", padding: "3px 0", display: "flex", gap: 6 }}>
                          <span style={{ color: t.color }}>▸</span> {d}
                        </div>
                      ))}
                    </div>
                  </div>
                )}
              </div>
            ))}
          </div>
        )}

        {/* State Machine */}
        {activeTab === "state" && (
          <div style={{ background: "#1e293b", borderRadius: 10, padding: 20 }}>
            <h3 style={{ color: "#60a5fa", fontSize: 15, margin: "0 0 16px" }}>BMS安全状态机</h3>
            <div style={{ display: "flex", flexDirection: "column", gap: 8 }}>
              {stateMachine.map((s, i) => (
                <div key={i} style={{ display: "flex", alignItems: "center", gap: 10 }}>
                  <div style={{
                    width: 70, height: 44, borderRadius: 22, background: s.color + "30", border: `2px solid ${s.color}`,
                    display: "flex", alignItems: "center", justifyContent: "center", fontSize: 12, fontWeight: 700, color: s.color, flexShrink: 0
                  }}>
                    {s.state}
                  </div>
                  {s.next && (
                    <>
                      <div style={{ flex: 1, textAlign: "center" }}>
                        <div style={{ fontSize: 10, color: "#94a3b8", marginBottom: 2 }}>{s.condition}</div>
                        <div style={{ borderTop: "1px dashed #475569", position: "relative" }}>
                          <span style={{ position: "absolute", right: 0, top: -5, color: "#475569" }}>→</span>
                        </div>
                      </div>
                      <div style={{
                        width: 70, height: 44, borderRadius: 22, background: "#1e293b", border: "1px solid #475569",
                        display: "flex", alignItems: "center", justifyContent: "center", fontSize: 11, color: "#94a3b8", flexShrink: 0
                      }}>
                        {s.next}
                      </div>
                    </>
                  )}
                </div>
              ))}
            </div>
            <div style={{ marginTop: 16, fontSize: 11, color: "#94a3b8", background: "#0f172a", padding: 12, borderRadius: 8, lineHeight: 1.8 }}>
              <div style={{ color: "#fbbf24", fontWeight: 700, marginBottom: 4 }}>状态转换规则：</div>
              <div>▸ 任何状态下检测到过流/短路 → 立即跳转<span style={{ color: "#ef4444" }}>故障</span>状态</div>
              <div>▸ 过压/欠压/过温 → 先进入<span style={{ color: "#f97316" }}>告警</span>，持续3s未消除则进入<span style={{ color: "#ef4444" }}>故障</span></div>
              <div>▸ <span style={{ color: "#ef4444" }}>故障</span>状态切断所有MOS，只有手动复位或ALERT清除后回到<span style={{ color: "#6b7280" }}>初始化</span></div>
              <div>▸ 所有状态转换记录到Flash日志，含时间戳+故障码，供事后分析</div>
            </div>
          </div>
        )}

        {/* Pin Map */}
        {activeTab === "pins" && (
          <div style={{ background: "#1e293b", borderRadius: 10, padding: 20 }}>
            <h3 style={{ color: "#60a5fa", fontSize: 15, margin: "0 0 14px" }}>STM32G431RBT6 引脚分配表</h3>
            <div style={{ display: "grid", gridTemplateColumns: "70px 110px 1fr", gap: 0, fontSize: 11 }}>
              <div style={{ padding: "6px 8px", fontWeight: 700, color: "#64748b", borderBottom: "2px solid #334155" }}>引脚</div>
              <div style={{ padding: "6px 8px", fontWeight: 700, color: "#64748b", borderBottom: "2px solid #334155" }}>功能</div>
              <div style={{ padding: "6px 8px", fontWeight: 700, color: "#64748b", borderBottom: "2px solid #334155" }}>用途</div>
              {pinMap.map((p, i) => (
                <>
                  <div key={`pin${i}`} style={{ padding: "5px 8px", color: "#60a5fa", fontFamily: "monospace", borderBottom: "1px solid #1e293b", background: i % 2 ? "#1e293b" : "#172033" }}>{p.pin}</div>
                  <div key={`func${i}`} style={{ padding: "5px 8px", color: "#fbbf24", fontFamily: "monospace", borderBottom: "1px solid #1e293b", background: i % 2 ? "#1e293b" : "#172033" }}>{p.func}</div>
                  <div key={`use${i}`} style={{ padding: "5px 8px", color: "#cbd5e1", borderBottom: "1px solid #1e293b", background: i % 2 ? "#1e293b" : "#172033" }}>{p.use}</div>
                </>
              ))}
            </div>
            <div style={{ marginTop: 12, fontSize: 11, color: "#94a3b8" }}>
              已使用 {pinMap.length} 个引脚，G431RBT6共64引脚(48 GPIO)，剩余充足可扩展
            </div>
          </div>
        )}

        {/* Clock */}
        {activeTab === "clock" && (
          <div style={{ background: "#1e293b", borderRadius: 10, padding: 20 }}>
            <h3 style={{ color: "#60a5fa", fontSize: 15, margin: "0 0 14px" }}>时钟树配置</h3>
            <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr", gap: 8 }}>
              {Object.entries(clockConfig).map(([k, v]) => (
                <div key={k} style={{ background: "#0f172a", borderRadius: 8, padding: "10px 14px", border: "1px solid #334155" }}>
                  <div style={{ fontSize: 10, color: "#64748b", marginBottom: 2 }}>{k}</div>
                  <div style={{ fontSize: 13, color: "#fbbf24", fontWeight: 600, fontFamily: "monospace" }}>{v}</div>
                </div>
              ))}
            </div>
            <div style={{ marginTop: 16, background: "#0f172a", borderRadius: 8, padding: 14, border: "1px solid #334155" }}>
              <div style={{ color: "#60a5fa", fontWeight: 700, fontSize: 12, marginBottom: 8 }}>CubeMX配置步骤</div>
              {[
                "1. RCC → HSE选Crystal，PLL Source选HSE",
                "2. PLL配置：M=/1, N=x42, P=/2 → SYSCLK=170MHz",
                "3. I2C1 → Enable, Standard Mode 100KHz",
                "4. USART1 → Async 115200, 开启DMA RX",
                "5. USART2 → Async 115200",
                "6. FDCAN1 → Classic Mode, 500Kbps, Auto Retransmit",
                "7. SPI1 → Master, 8bit, Prescaler=8",
                "8. ADC1 → 12bit, Scan+Continuous+DMA, 3通道",
                "9. TIM2 → 1s中断, TIM3 → 1s TRGO触发ADC",
                "10. FreeRTOS → CMSIS_V2, 创建6个任务",
                "11. NVIC → EXTI3优先级=1(ALERT最高)",
              ].map((s, i) => (
                <div key={i} style={{ fontSize: 11, color: "#cbd5e1", padding: "3px 0" }}>{s}</div>
              ))}
            </div>
          </div>
        )}
      </div>
    </div>
  );
}

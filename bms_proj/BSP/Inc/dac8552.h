#ifndef __DAC8552_H__
#define __DAC8552_H__

#include <stdint.h>
#include <stdbool.h>

/* DAC8552：双通道 16-bit DAC，SPI 模式 1 或 0 均可（CPOL=0,CPHA=1 标准）
 * 帧 24-bit：
 *   bit23..20 = Control (LD1 LD0 - BUF):
 *     0x10 → Write to channel A buffer
 *     0x24 → Write to channel B buffer, update both A&B
 *     0x30 → Write to A buffer, update both
 *   bit19..16 = 0
 *   bit15..0  = 16-bit data
 * 简化封装：
 */
bool dac8552_init(void);
bool dac8552_set_a(uint16_t code);    // 写 A，A 立即更新
bool dac8552_set_b(uint16_t code);    // 写 B，B 立即更新
bool dac8552_set_both(uint16_t a, uint16_t b);

/* ===== 高层封装：电压 / 恒流设定 ===== */
#define DAC_CH_A            0
#define DAC_CH_B            1
#define DAC_VREF_V          3.3f       // REF3033 基准
#define DAC_FULL_SCALE      65535U

/* 恒流环增益: V_dac = I[A] × Rsns × 增益。Rsns=10mΩ。
 * 增益由 OPA2188 反馈电阻决定，初值为占位，必须上板用
 * “设定电流 vs INA226 实测”标定后修正。 */
#define DAC_RSNS_OHM        0.010f
#define DAC_CC_LOOP_GAIN    50.0f

/* 设定通道输出电压 (0 ~ DAC_VREF_V)。 */
bool dac8552_set_voltage(uint8_t ch, float volts);

/* 设定通道恒流目标 (mA)，依赖 DAC_CC_LOOP_GAIN（需标定）。 */
bool dac8552_set_current(uint8_t ch, float current_mA);

#endif

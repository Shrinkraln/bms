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

#endif

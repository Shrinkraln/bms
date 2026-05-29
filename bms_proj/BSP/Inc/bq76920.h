#ifndef __BQ76920_H__
#define __BQ76920_H__

#include <stdint.h>
#include <stdbool.h>

/* BQ7692003PWR：默认 7-bit 地址 0x08，带 CRC 校验 */
#define BQ76_ADDR7      0x08

/* 寄存器（节选） */
#define BQ76_REG_SYS_STAT       0x00
#define BQ76_REG_CELLBAL1       0x01
#define BQ76_REG_SYS_CTRL1      0x04
#define BQ76_REG_SYS_CTRL2      0x05
#define BQ76_REG_PROTECT1       0x06
#define BQ76_REG_PROTECT2       0x07
#define BQ76_REG_PROTECT3       0x08
#define BQ76_REG_OV_TRIP        0x09
#define BQ76_REG_UV_TRIP        0x0A
#define BQ76_REG_CC_CFG         0x0B
#define BQ76_REG_VC1_HI         0x0C   /* 之后每节占 2 字节，连续到 VC15 */
#define BQ76_REG_BAT_HI         0x2A
#define BQ76_REG_TS1_HI         0x2C
#define BQ76_REG_ADCGAIN1       0x50
#define BQ76_REG_ADCOFFSET      0x51
#define BQ76_REG_ADCGAIN2       0x59

/* 测试结果结构 */
typedef struct {
    bool     present;           // 是否能识别
    uint8_t  sys_stat;          // 状态寄存器
    int16_t  adc_gain_uV;       // 增益 (μV/LSB)
    int8_t   adc_offset_mV;     // 偏移 (mV)
    uint16_t vc_raw[5];         // 5节原始 ADC
    uint16_t vc_mV[5];          // 5节折算电压（mV）
    uint16_t bat_mV;            // 整组电池电压（mV，简化估算）
    uint16_t ts1_raw;           // NTC 原始 ADC
} bq76_info_t;

/* SYS_STAT 位定义 (与数据手册一致) */
#define BQ76_STAT_CC_READY      (1u << 7)
#define BQ76_STAT_DEVICE_XREADY (1u << 5)
#define BQ76_STAT_OVRD_ALERT    (1u << 4)
#define BQ76_STAT_UV            (1u << 3)
#define BQ76_STAT_OV            (1u << 2)
#define BQ76_STAT_SCD           (1u << 1)
#define BQ76_STAT_OCD           (1u << 0)
#define BQ76_STAT_FAULT_MASK    (BQ76_STAT_DEVICE_XREADY | BQ76_STAT_OVRD_ALERT | \
                                 BQ76_STAT_UV | BQ76_STAT_OV |                    \
                                 BQ76_STAT_SCD | BQ76_STAT_OCD)

/* 必须先调用 init，发起 ADC_EN 和清错 */
bool bq76_init(void);
bool bq76_read_info(bq76_info_t *info);

/* 轻量 ALERT 处理用: 只读/写 SYS_STAT, 避免 read_info 那么重 (~16 笔 I2C) */
bool bq76_read_status(uint8_t *stat);
bool bq76_clear_status(uint8_t bits_to_clear);   /* 写 1 清 0; 传 0xFF 清所有 */

#endif

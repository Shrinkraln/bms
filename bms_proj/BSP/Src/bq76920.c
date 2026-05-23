#include "bq76920.h"
#include "i2c_bus.h"
#include "stm32g4xx_hal.h"
#include <string.h>

extern I2C_HandleTypeDef hi2c1;

/* ===== CRC-8 / poly=0x07, init=0x00, 标准 ATmel/SMBus 风格 =====
 * BQ7692003 在 7-bit 地址 0x08 下默认强制 CRC，写/读每个数据字节都带 CRC：
 * 写：[ (Addr<<1|W), Data0, CRC(Addr,Data0), Data1, CRC(Data1), ... ]
 * 读：[ (Addr<<1|W), RegAddr, CRC(Addr,Reg) ] 然后重启 + (Addr<<1|R)
 *      接着每个数据字节后跟 CRC：第一个 CRC 覆盖 (Addr<<1|R)+Data0，
 *      后续 CRC 只覆盖 Data。
 *
 * 因为 HAL 不直接支持这种自定义 CRC 协议，我们在用户层手动拼报文，
 * 并通过 HAL_I2C_Master_Transmit/Receive 走"无 STOP 的组合传输"。
 */

static uint8_t crc8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; ++b) {
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x07) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

#define BQ76_W_ADDR  ((BQ76_ADDR7 << 1) | 0)
#define BQ76_R_ADDR  ((BQ76_ADDR7 << 1) | 1)

/* 单字节读：reg → val */
static bool bq76_read8(uint8_t reg, uint8_t *val)
{
    /* 写 reg 阶段：[W_ADDR, reg, crc(W_ADDR, reg)]，但 HAL 会自动发 W_ADDR，
     * 所以我们只需发送 [reg]，CRC 由我们附加？——错，BQ 要求 CRC 紧跟
     * 第一个数据字节，且 CRC 计算覆盖前面的 7bit addr<<1|W 与该字节。
     *
     * 实际：写 reg 时只发 1 个数据字节 reg，无需 CRC（写阶段第一个数据
     * 字节后才需要 CRC，而我们这里只有 1 字节用于寻址；某些资料把这
     * 一字节也算作"数据"并要求 CRC）。
     *
     * 为兼容 BQ7692003 的"每个数据字节都要 CRC"，我们采用：
     * 写阶段发 [reg, crc(W_ADDR,reg)]，然后重启读阶段，
     * 接收 [data, crc(R_ADDR,data)] 并校验。
     */
    uint8_t tx[2];
    tx[0] = reg;
    {
        uint8_t buf[2] = { BQ76_W_ADDR, reg };
        tx[1] = crc8(buf, 2);
    }

    if (HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)BQ76_W_ADDR,
                                tx, 2, 20) != HAL_OK) return false;

    uint8_t rx[2] = {0};
    if (HAL_I2C_Master_Receive(&hi2c1, (uint16_t)BQ76_W_ADDR,
                               rx, 2, 20) != HAL_OK) return false;

    uint8_t chk_buf[2] = { BQ76_R_ADDR, rx[0] };
    if (crc8(chk_buf, 2) != rx[1]) return false;

    *val = rx[0];
    return true;
}

/* 连续读：reg 起读 n 字节，每字节后跟一个 CRC */
static bool bq76_read_block(uint8_t reg, uint8_t *out, uint8_t n)
{
    uint8_t tx[2];
    tx[0] = reg;
    {
        uint8_t buf[2] = { BQ76_W_ADDR, reg };
        tx[1] = crc8(buf, 2);
    }
    if (HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)BQ76_W_ADDR,
                                tx, 2, 20) != HAL_OK) return false;

    /* 每个 data 字节后都有 1 字节 CRC，所以接收 2n 字节 */
    uint8_t rx[64];
    if (n * 2 > sizeof(rx)) return false;
    if (HAL_I2C_Master_Receive(&hi2c1, (uint16_t)BQ76_W_ADDR,
                               rx, (uint16_t)(n * 2), 50) != HAL_OK) return false;

    /* 第一个 CRC 覆盖 (R_ADDR, data0) */
    {
        uint8_t chk[2] = { BQ76_R_ADDR, rx[0] };
        if (crc8(chk, 2) != rx[1]) return false;
        out[0] = rx[0];
    }
    /* 后续 CRC 只覆盖 data */
    for (uint8_t i = 1; i < n; ++i) {
        if (crc8(&rx[i*2], 1) != rx[i*2 + 1]) return false;
        out[i] = rx[i*2];
    }
    return true;
}

/* 单字节写：reg = val */
static bool bq76_write8(uint8_t reg, uint8_t val)
{
    /* 写帧：[reg, crc(W_ADDR,reg), val, crc(val)]
     * 第二个 CRC 只覆盖 val（不包含地址）。 */
    uint8_t tx[4];
    tx[0] = reg;
    {
        uint8_t buf[2] = { BQ76_W_ADDR, reg };
        tx[1] = crc8(buf, 2);
    }
    tx[2] = val;
    tx[3] = crc8(&val, 1);

    return HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)BQ76_W_ADDR,
                                   tx, 4, 30) == HAL_OK;
}

/* ===== 上层 API ===== */
bool bq76_init(void)
{
    /* 先 probe：用一个不带 CRC 的轻量探测看看是否 ACK */
    if (HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)BQ76_W_ADDR, 2, 5) != HAL_OK)
        return false;

    /* 清 SYS_STAT 上电状态位（写 1 清 0） */
    if (!bq76_write8(BQ76_REG_SYS_STAT, 0xFF)) return false;

    /* SYS_CTRL1: ADC_EN=1(bit4), TEMP_SEL=1(bit3 用 TS1 外部 NTC) */
    if (!bq76_write8(BQ76_REG_SYS_CTRL1, 0x18)) return false;

    /* SYS_CTRL2: CC_EN=1(bit6) 持续库仑计数 */
    if (!bq76_write8(BQ76_REG_SYS_CTRL2, 0x40)) return false;

    HAL_Delay(250);  // 等 ADC 转换稳定
    return true;
}

bool bq76_read_info(bq76_info_t *info)
{
    memset(info, 0, sizeof(*info));
    uint8_t v;

    if (!bq76_read8(BQ76_REG_SYS_STAT, &info->sys_stat)) return false;

    /* 读 ADC 增益和偏移 */
    uint8_t g1 = 0, g2 = 0, off = 0;
    if (!bq76_read8(BQ76_REG_ADCGAIN1, &g1)) return false;
    if (!bq76_read8(BQ76_REG_ADCGAIN2, &g2)) return false;
    if (!bq76_read8(BQ76_REG_ADCOFFSET, &off)) return false;

    /* ADCGAIN 拼接：gain = 365 + (G1<3:2>:G2<7:5>) μV/LSB */
    uint8_t gain_code = (uint8_t)(((g1 & 0x0C) << 1) | (g2 >> 5));
    info->adc_gain_uV = (int16_t)(365 + gain_code);
    info->adc_offset_mV = (int8_t)off;

    /* 读 5 节电池 VC1..VC5 的 ADC，每个 2 字节，大端 */
    uint8_t vc_buf[10];
    if (!bq76_read_block(BQ76_REG_VC1_HI, vc_buf, 10)) return false;

    for (int i = 0; i < 5; ++i) {
        uint16_t raw = ((uint16_t)vc_buf[i*2] << 8) | vc_buf[i*2+1];
        raw &= 0x3FFF;  // 14-bit
        info->vc_raw[i] = raw;
        /* V_cell = gain*raw + offset */
        int32_t uv = (int32_t)raw * info->adc_gain_uV;
        int32_t mv = uv / 1000 + info->adc_offset_mV;
        if (mv < 0) mv = 0;
        info->vc_mV[i] = (uint16_t)mv;
    }
    (void)v;
    return true;
}

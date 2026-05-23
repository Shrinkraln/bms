#include "periph_tests.h"
#include "i2c_bus.h"
#include "bsp.h"
#include "stm32g4xx_hal.h"

/* ===== BQ34Z100-G1 =====
 * 它是 SBS 风格，通过 Control() 子命令读 DeviceType：
 *   写 0x00 = 0x0001  (低字节先)
 *   再读 0x00 两字节 (低字节先)，应为 0x0100
 */
bool bq34_test(uint16_t *device_type_out)
{
    bq34_enable(true);
    HAL_Delay(250);              // BQ34 上电启动需要时间

    /* 先做 IsDeviceReady */
    if (!i2c_probe(BQ34_ADDR7)) {
        return false;
    }

    uint8_t cmd[2] = { 0x01, 0x00 };          // 0x0001 LE → DeviceType
    if (!i2c_write_buf(BQ34_ADDR7, 0x00, cmd, 2)) return false;
    HAL_Delay(2);

    uint16_t dt_le = 0;
    if (!i2c_read_reg16_le(BQ34_ADDR7, 0x00, &dt_le)) return false;

    if (device_type_out) *device_type_out = dt_le;
    return dt_le == 0x0100;
}

/* ===== INA226 ===== */
bool ina226_test(uint16_t *mfg_id, uint16_t *die_id, int16_t *shunt_uV, uint16_t *bus_mV)
{
    if (!i2c_probe(INA226_ADDR7)) return false;

    uint16_t mid = 0, did = 0;
    if (!i2c_read_reg16_be(INA226_ADDR7, INA226_REG_MFG_ID, &mid)) return false;
    if (!i2c_read_reg16_be(INA226_ADDR7, INA226_REG_DIE_ID, &did)) return false;
    if (mfg_id) *mfg_id = mid;
    if (die_id) *die_id = did;
    if (mid != 0x5449) return false;          // 'TI'
    /* die_id 不强校验，因为有 0x2260 / 0x2261 等批次差异 */

    /* 配置：avg=16, VBUS=1.1ms, VSH=1.1ms, mode=shunt+bus 连续 */
    uint8_t cfg[3] = { INA226_REG_CFG, 0x45, 0x27 };  // 0x4527
    if (HAL_I2C_Master_Transmit(&hi2c1, INA226_ADDR7<<1, cfg, 3, 20) != HAL_OK)
        return false;
    HAL_Delay(20);

    uint16_t s_raw = 0, v_raw = 0;
    if (!i2c_read_reg16_be(INA226_ADDR7, INA226_REG_SHUNT_V, &s_raw)) return false;
    if (!i2c_read_reg16_be(INA226_ADDR7, INA226_REG_BUS_V,   &v_raw)) return false;
    /* shunt LSB = 2.5 μV，bus LSB = 1.25 mV */
    if (shunt_uV) *shunt_uV = (int16_t)((int32_t)(int16_t)s_raw * 25 / 10);
    if (bus_mV)   *bus_mV   = (uint16_t)((uint32_t)v_raw * 125 / 100);
    return true;
}

/* ===== TMP117 ===== */
bool tmp117_test(uint16_t *dev_id_out, float *temp_C)
{
    if (!i2c_probe(TMP117_ADDR7)) return false;

    uint16_t id = 0;
    if (!i2c_read_reg16_be(TMP117_ADDR7, TMP117_REG_DEV_ID, &id)) return false;
    if (dev_id_out) *dev_id_out = id;
    if ((id & 0x0FFF) != 0x117) return false;

    /* 默认上电就在持续转换模式，等一次转换 */
    HAL_Delay(20);
    uint16_t t_raw = 0;
    if (!i2c_read_reg16_be(TMP117_ADDR7, TMP117_REG_TEMP, &t_raw)) return false;
    if (temp_C) *temp_C = (float)((int16_t)t_raw) * 0.0078125f;
    return true;
}

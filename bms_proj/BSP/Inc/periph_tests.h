#ifndef __PERIPH_TESTS_H__
#define __PERIPH_TESTS_H__

#include <stdint.h>
#include <stdbool.h>

/* ===== BQ34Z100-G1 ===== */
#define BQ34_ADDR7      0x55
/* DeviceType 命令通过 Manufacturer Access：写 0x00=0x0001，再读 0x00 16-bit */
bool bq34_test(uint16_t *device_type_out);

/* ===== INA226 ===== */
#define INA226_ADDR7    0x40
#define INA226_REG_CFG          0x00
#define INA226_REG_SHUNT_V      0x01
#define INA226_REG_BUS_V        0x02
#define INA226_REG_POWER        0x03
#define INA226_REG_CURRENT      0x04
#define INA226_REG_MFG_ID       0xFE   // = 0x5449 ('TI')
#define INA226_REG_DIE_ID       0xFF   // = 0x2260
bool ina226_test(uint16_t *mfg_id, uint16_t *die_id, int16_t *shunt_uV, uint16_t *bus_mV);

/* INA226 连续测量：先 ina226_init() 配置一次，之后反复 ina226_read()。
 * 比 ina226_test() 高效（不每次重写 CONFIG）。Rsns=10mΩ。 */
bool    ina226_init(void);
bool    ina226_read(int16_t *shunt_uV, uint16_t *bus_mV);
int32_t ina226_current_mA(void);   /* 有符号；失败返回 0 */

/* ===== TMP117 ===== */
#define TMP117_ADDR7    0x48
#define TMP117_REG_TEMP         0x00
#define TMP117_REG_CFG          0x01
#define TMP117_REG_DEV_ID       0x0F   // = 0x0117
bool tmp117_test(uint16_t *dev_id, float *temp_C);

#endif

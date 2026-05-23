#include "i2c_bus.h"
#include <stddef.h>

#define I2C_TIMEOUT 50

bool i2c_probe(uint8_t addr7)
{
    return HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(addr7 << 1), 2, 5) == HAL_OK;
}

bool i2c_write_reg8(uint8_t addr7, uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { reg, val };
    return HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)(addr7 << 1),
                                   buf, 2, I2C_TIMEOUT) == HAL_OK;
}

bool i2c_read_reg8(uint8_t addr7, uint8_t reg, uint8_t *val)
{
    if (HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)(addr7 << 1),
                                &reg, 1, I2C_TIMEOUT) != HAL_OK) return false;
    return HAL_I2C_Master_Receive(&hi2c1, (uint16_t)(addr7 << 1),
                                  val, 1, I2C_TIMEOUT) == HAL_OK;
}

bool i2c_read_buf(uint8_t addr7, uint8_t reg, uint8_t *buf, uint16_t n)
{
    if (HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)(addr7 << 1),
                                &reg, 1, I2C_TIMEOUT) != HAL_OK) return false;
    return HAL_I2C_Master_Receive(&hi2c1, (uint16_t)(addr7 << 1),
                                  buf, n, I2C_TIMEOUT) == HAL_OK;
}

bool i2c_write_buf(uint8_t addr7, uint8_t reg, const uint8_t *buf, uint16_t n)
{
    uint8_t tmp[32];
    if ((size_t)(n + 1) > sizeof(tmp)) return false;
    tmp[0] = reg;
    for (uint16_t i = 0; i < n; ++i) tmp[1+i] = buf[i];
    return HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)(addr7 << 1),
                                   tmp, (uint16_t)(n+1), I2C_TIMEOUT) == HAL_OK;
}

bool i2c_read_reg16_be(uint8_t addr7, uint8_t reg, uint16_t *val)
{
    uint8_t b[2];
    if (!i2c_read_buf(addr7, reg, b, 2)) return false;
    *val = ((uint16_t)b[0] << 8) | b[1];
    return true;
}

bool i2c_read_reg16_le(uint8_t addr7, uint8_t reg, uint16_t *val)
{
    uint8_t b[2];
    if (!i2c_read_buf(addr7, reg, b, 2)) return false;
    *val = ((uint16_t)b[1] << 8) | b[0];
    return true;
}

uint8_t i2c_scan(uint8_t *found, uint8_t max_n)
{
    uint8_t n = 0;
    for (uint8_t a = 0x08; a <= 0x77; ++a) {
        if (i2c_probe(a)) {
            if (n < max_n) found[n] = a;
            n++;
        }
    }
    return n;
}

#ifndef __I2C_BUS_H__
#define __I2C_BUS_H__

#include "stm32g4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

extern I2C_HandleTypeDef hi2c1;     // CubeMX 生成的 I2C1 句柄（PB6/PB7）

/* 7-bit 地址；内部会自动 <<1 */
bool i2c_probe(uint8_t addr7);
bool i2c_write_reg8(uint8_t addr7, uint8_t reg, uint8_t val);
bool i2c_read_reg8 (uint8_t addr7, uint8_t reg, uint8_t *val);
bool i2c_read_buf  (uint8_t addr7, uint8_t reg, uint8_t *buf, uint16_t n);
bool i2c_write_buf (uint8_t addr7, uint8_t reg, const uint8_t *buf, uint16_t n);

/* 16-bit 寄存器值（大端/小端按器件文档而定，这里按字节给出） */
bool i2c_read_reg16_be(uint8_t addr7, uint8_t reg, uint16_t *val);
bool i2c_read_reg16_le(uint8_t addr7, uint8_t reg, uint16_t *val);

/* 总线扫描：把找到的 7bit 地址写入 found[]，返回个数。max_n 是 found 容量。 */
uint8_t i2c_scan(uint8_t *found, uint8_t max_n);

#endif

#ifndef __MAIN_H__
#define __MAIN_H__

#include "stm32g4xx_hal.h"
#include <stdarg.h>


#include "bsp.h"
#include "i2c_bus.h"
#include "bq76920.h"
#include "periph_tests.h"
#include "dac8552.h"
#include "lcd_st7796.h"
#include "can_test.h"
/* CubeMX 生成的引脚宏，如果有自定义命名，可在这里追加。
 * 这里我们直接使用 HAL 标准 GPIO_PIN_x，故不再 redef。
 */

void Error_Handler(void);
void set_up(void);
void loop(void);
#endif

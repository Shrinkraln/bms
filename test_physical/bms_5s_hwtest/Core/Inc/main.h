#ifndef __MAIN_H__
#define __MAIN_H__

#include "stm32g4xx_hal.h"
#include <stdarg.h>

/* CubeMX 生成的引脚宏，如果有自定义命名，可在这里追加。
 * 这里我们直接使用 HAL 标准 GPIO_PIN_x，故不再 redef。
 */

void Error_Handler(void);

#endif

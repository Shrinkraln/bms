#ifndef __CAN_TEST_H__
#define __CAN_TEST_H__

#include <stdbool.h>

/* 在 FDCAN1 上做内部回环 (Loopback) 测试：
 *  - 不依赖收发器和外部网络
 *  - 发 1 帧标准 11-bit ID，收回来比对
 * 返回 true 表示 CAN 控制器 + 时钟 + 引脚复用功能正常。
 */
bool can_loopback_test(void);

#endif

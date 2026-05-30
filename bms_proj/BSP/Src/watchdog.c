#include "watchdog.h"
#include "stm32g4xx.h"   /* CMSIS device header: IWDG, IWDG_BASE, RCC, ... */
#include <stdio.h>

/* IWDG 直接寄存器操作 — 不依赖 HAL_IWDG (避免 CubeMX 没勾该外设导致
 * stm32g4xx_hal_iwdg.h/.c 没生成的问题)。
 *
 * 计数频率 = LSI(~32kHz) / Prescaler
 *   Prescaler = 32  -> 1kHz, reload=1600 -> 1.6s 超时
 * IWDG 由 LSI 喂时钟; LSI 上电后会自动跑, 但有些资料建议软件确认。
 *
 * Key 寄存器 (IWDG->KR) 命令:
 *   0xAAAA  喂狗 (reload counter)
 *   0x5555  允许写 PR/RLR/WINR
 *   0xCCCC  启动 IWDG (启动后 KR 任何非 0xAAAA 写入即停止权限)
 *
 * 注: IWDG 一旦启动无法关闭, 这里 init 完直接就跑, 后续靠 watchdog_feed 维持。 */

#define WDG_PR_VAL     0x03U   /* IWDG_PR: 0=4, 1=8, 2=16, 3=32, ... 7=256 */
#define WDG_RELOAD     1600U   /* 12-bit, 范围 0..4095 */

static bool s_started = false;

bool watchdog_init(void)
{
    /* G4 LSI 默认上电会跑, 但为了保险显式开一下 (LSION) */
    RCC->CSR |= RCC_CSR_LSION;
    /* 等 LSI 就绪 (LSIRDY); 不会超过几个 LSI 周期, 设个超时防卡死 */
    uint32_t to = 100000;
    while (!(RCC->CSR & RCC_CSR_LSIRDY)) {
        if (--to == 0) {
            printf("[WDG] LSI not ready\r\n");
            return false;
        }
    }

    IWDG->KR  = 0x5555U;       /* unlock PR/RLR */
    IWDG->PR  = WDG_PR_VAL;
    IWDG->RLR = WDG_RELOAD;

    /* 等寄存器更新完成 (SR 的 PVU/RVU 都清 0 后才能启动) */
    to = 100000;
    while (IWDG->SR != 0) {
        if (--to == 0) {
            printf("[WDG] SR didn't clear\r\n");
            return false;
        }
    }

    IWDG->KR = 0xAAAAU;        /* 先喂一口 */
    IWDG->KR = 0xCCCCU;        /* GO */

    s_started = true;
    printf("[WDG] IWDG armed: ~1.6s timeout (LSI/32, reload=%u)\r\n", (unsigned)WDG_RELOAD);
    return true;
}

void watchdog_feed(void)
{
    if (!s_started) return;
    IWDG->KR = 0xAAAAU;
}

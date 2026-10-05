#pragma once
#include <stdint.h>
#include <utils/Timekeeping.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Delays the execution for a specified number of milliseconds.
 *
 * @param msec The number of milliseconds to delay. If the value is less than 2048,
 * it delays using microseconds accordingly; otherwise, it delays in 1-millisecond steps.
 */
void delaymS(duration_t msec);
/**
 * Brief: Delays execution for a specified number of microseconds.
 *
 * This function uses a busy-wait loop to pause the execution of the program for the given
 * number of microseconds. It calculates the number of loops required based on the system
 * core clock frequency to achieve the desired delay. On the Cortex-M0+ families this method
 * is not accurate below 7uS. On STM32F1 it uses the DWT cycle counter and is accurate
 * to a few cycles. SystemCoreClock must be up to date.
 *
 * @param usec The number of microseconds to delay. The actual delay may vary slightly
 *             depending on the system core clock frequency and the overhead of the delay
 *             loop.
 */
void delayuS(duration_t usec);

/**
 * Delays execution for a number of core clock cycles.
 *
 * On STM32F1 (Cortex-M3) the delay uses the DWT cycle counter and is cycle
 * accurate, apart from a few cycles of call overhead. On the Cortex-M0+
 * families it is a busy loop of about 3 cycles per iteration and only
 * approximate, as in delayuS().
 *
 * @param cycles The number of core clock cycles to delay.
 */
void delayCycles(uint32_t cycles);

#ifdef __cplusplus
}
#endif

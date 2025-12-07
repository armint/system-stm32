#pragma once
#include <stdint.h>
#include <utils/systick.h>
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
 * core clock frequency to achieve the desired delay. This method is not accurate below 7uS.
 * For shorter delays, consider creating your own delay loop.
 *
 * @param usec The number of microseconds to delay. The actual delay may vary slightly
 *             depending on the system core clock frequency and the overhead of the delay
 *             loop.
 */
void delayuS(duration_t usec);

#ifdef __cplusplus
}
#endif

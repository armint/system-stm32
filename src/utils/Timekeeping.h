#pragma once

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */
typedef int64_t timestamp_t;
typedef uint32_t duration_t;

/*
 * These functions count SysTick_Handler invocations; the unit is whatever
 * period SysTick_Config() was given elsewhere in the application (e.g. the
 * RTOS port) — this file does not configure or verify that period, so it
 * has no way to know it in real time units. They only mean milliseconds if
 * that period happens to be 1ms (1000Hz).
 */

/**
 * Returns the system uptime in ticks.
 *
 * @return The number of SysTick_Handler invocations since system start.
 */
timestamp_t getUptimeTicks();

/**
 * @brief Retrieves the system uptime in thousandths of a tick.
 *
 * Combines the tick counter with the current SysTick peripheral value at a
 * resolution of 1/1000 of a tick, i.e. the return value is getUptimeTicks()
 * scaled by 1000 with the current tick's fractional progress added.
 *
 * @return The uptime in milli-ticks (ticks * 1000).
 */
timestamp_t getUptimeMilliTicks();

void sysTickUpdateHook(timestamp_t sysTick);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

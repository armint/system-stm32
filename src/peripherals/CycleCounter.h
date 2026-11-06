#pragma once

#include <cmsis_device.h>
#include <cstdint>

// A 32-bit core clock cycle counter: the DWT cycle counter (DWT->CYCCNT). The interface is the one
// of hw::CycleCounter in ch32v00x-system (the SysTick counter there). The count wraps every 2^32
// cycles (59.6 s at 72 MHz): use it for differences only. See docs/cycle-counter.md.
//
// Cortex-M0+ (STM32C0, STM32G0, STM32L0) has no DWT cycle counter.
#if !defined(DWT_CTRL_CYCCNTENA_Msk)
#error "hw::CycleCounter: this core has no DWT cycle counter (Cortex-M3, M4 and M33 only)"
#endif

namespace hw::CycleCounter {

/**
 * @brief Starts the counter. Call it before a series of count() calls.
 *
 * A debugger or other code can clear TRCENA or CYCCNTENA, and the counter then stops. TRCENA gates
 * the whole DWT. The function sets both bits each time, so it is safe to call before each timed
 * operation. It does not reset the count.
 */
[[gnu::always_inline]] inline void start() {
#if defined(DCB)
   // Armv8-M (Cortex-M33): the CMSIS core header has DCB, not CoreDebug.
   DCB->DEMCR |= DCB_DEMCR_TRCENA_Msk;
#else
   CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
#endif
   DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

/**
 * @brief Returns the core clock cycles. The counter is free-running and wraps at 2^32.
 */
[[gnu::always_inline]] inline uint32_t count() {
   return DWT->CYCCNT;
}

} // namespace hw::CycleCounter

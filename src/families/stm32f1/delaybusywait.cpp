#include "delaybusywait.h"
#include <cmsis_device.h>

// Cortex-M3 implementation of the delay functions. It counts core clock
// cycles with the DWT cycle counter (DWT->CYCCNT), so the result does not
// depend on flash wait states or on the instruction timing of the loop.
// The Cortex-M0+ implementation in src/delaybusywait.cpp is not used here.

extern uint32_t SystemCoreClock;

namespace {

// Estimated cycles that the function call, the counter check and the loop
// exit add to a delay, counted from the call until the return. The code
// subtracts it from the requested cycles. Tune with a logic analyzer.
constexpr uint32_t kOverheadCycles = 12;

// Enable the DWT cycle counter if it is not running. Every delay calls
// this. That keeps the delays working after a debugger session or other
// code has cleared TRCENA or CYCCNTENA. TRCENA gates the whole DWT: when it
// is 0, CYCCNT stays frozen even if CYCCNTENA reads 1. So the function
// tests both bits. When the counter runs, the cost is two loads and two
// branches.
__attribute__((always_inline)) inline void enableCycleCounter() {
   if(!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk) || !(DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk)) {
      CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
      DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
   }
}

// Wait until CYCCNT reaches the deadline. The signed difference makes the
// comparison correct across the 32-bit wrap of the counter.
inline void waitUntil(uint32_t deadline) {
   while(static_cast<int32_t>(DWT->CYCCNT - deadline) < 0) {
   }
}

// Wait for a number of cycles, counted from a CYCCNT value that the caller
// read as early as possible. Work that the caller did after reading start
// (for example the cycle count calculation) is part of the delay.
inline void waitCyclesFrom(uint32_t start, uint32_t cycles) {
   cycles = cycles > kOverheadCycles ? cycles - kOverheadCycles : 0;
   // Cap each step below 2^31 so the signed compare in waitUntil() holds.
   uint32_t deadline = start;
   while(cycles > 0x40000000u) {
      deadline += 0x40000000u;
      waitUntil(deadline);
      cycles -= 0x40000000u;
   }
   waitUntil(deadline + cycles);
}

} // namespace

void delayCycles(uint32_t cycles) {
   enableCycleCounter();
   waitCyclesFrom(DWT->CYCCNT, cycles);
}

void delayuS(uint32_t usec) {
   enableCycleCounter();
   // Read the start value before the calculation. The udiv time depends on
   // the data, so it must be inside the measured window.
   const uint32_t start = DWT->CYCCNT;
   uint32_t cycles = SystemCoreClock;
   if(cycles >= 2000000) {
      // Exact for whole-MHz core clocks, and no 64-bit math in the hot path.
      cycles /= 1000000;
      cycles *= usec;
   } else {
      cycles *= usec;
      cycles /= 1000000;
   }
   waitCyclesFrom(start, cycles);
}

void delaymS(uint32_t msec) {
   if(msec == 0)
      return;
   enableCycleCounter();
   const uint32_t cyclesPerMs = SystemCoreClock / 1000;
   // Advance an absolute deadline by one millisecond per step. The
   // per-step overhead then does not add up over long delays.
   uint32_t deadline = DWT->CYCCNT;
   while(msec--) {
      deadline += cyclesPerMs;
      waitUntil(deadline);
   }
}

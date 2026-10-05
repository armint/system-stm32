#include "delaybusywait.h"

extern uint32_t SystemCoreClock;

void delaymS(uint32_t msec) {
   if(msec < 0x800) {
      delayuS(msec * 1000);
   } else {
      uint32_t counter = msec;
      while(counter > 0) {
         delayuS(1000);
         counter--;
      }
   }
}

void delayuS(uint32_t usec) {
   uint32_t delayCycles = SystemCoreClock;
   if(delayCycles >= 2000000) {
      delayCycles /= 1000000;
      delayCycles *= usec;
   } else {
      delayCycles *= usec;
      delayCycles /= 1000000;
   }

   if(delayCycles <= 170)
      return;
   delayCycles -= 170; // cycle count for calculations and method entry & exit
   delayCycles /= 3;   // cycle count per iteration - should be 3 on Cortex M0+

   if(!delayCycles)
      return;

   // Delay loop for Cortex M0 thumb2
   asm volatile(
   // Load loop count to register
   " mov r3, %[loops]\n"

   // loop start- subtract 1 from r3
   "loop: sub r3, #1\n"
   // test for zero, loop if not
   " bne loop\n\n"

   :                          // No output registers
   : [loops] "r"(delayCycles) // Input registers
   : "r3"                     // clobbered registers
   );
}

void delayCycles(uint32_t cycles) {
   // Estimated cycle count for method entry & exit. There is no division
   // here, unlike in delayuS(), so this is much less than 170.
   constexpr uint32_t kOverheadCycles = 10;
   if(cycles <= kOverheadCycles)
      return;
   cycles -= kOverheadCycles;
   cycles /= 3;   // cycle count per iteration - should be 3 on Cortex M0+

   if(!cycles)
      return;

   // Delay loop for Cortex M0 thumb2
   asm volatile(
   " mov r3, %[loops]\n"
   "1: sub r3, #1\n"
   " bne 1b\n\n"
   :
   : [loops] "r"(cycles)
   : "r3"
   );
}

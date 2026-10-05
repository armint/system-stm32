
#include "Rcc.h"
namespace Rcc {
constexpr uint32_t AHBDivTab[] = { 2, 4, 8, 16, 64, 128, 256 };
constexpr uint32_t APBDivTab[] = { 2, 4, 8, 16 };

void enableHSI() {
   /* HSI configuration and activation */
   RCC->CR |= RCC_CR_HSION;
   while(!(RCC->CR & RCC_CR_HSIRDY)) {
   }
}

void setSystemClockSource(SystemClockSource source) {
   uint32_t tmp = (static_cast<uint32_t>(source) >> RCC_CFGR_SWS_Pos) << RCC_CFGR_SW_Pos;
   RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | tmp;
   while((RCC->CFGR & RCC_CFGR_SWS) != static_cast<uint32_t>(source)) {
   }
}

uint32_t getAPBClock() {
   uint32_t clock = getAHBClock();
#if defined(RCC_CFGR_PPRE1)
   uint32_t apbDif = (RCC->CFGR & RCC_CFGR_PPRE1) >> RCC_CFGR_PPRE1_Pos;
#else
   uint32_t apbDif = static_cast<uint32_t>(getAPBPrescaler()) >> RCC_CFGR_PPRE_Pos;
#endif
   if(apbDif & 0x4) {
      clock = clock / APBDivTab[apbDif & 0x3];
   }

   return clock;
}

#if defined(RCC_CFGR_PPRE2)
uint32_t getAPB2Clock() {
   uint32_t clock = getAHBClock();
   uint32_t apbDif = (RCC->CFGR & RCC_CFGR_PPRE2) >> RCC_CFGR_PPRE2_Pos;
   if(apbDif & 0x4) {
      clock = clock / APBDivTab[apbDif & 0x3];
   }

   return clock;
}
#endif

#if defined(RCC_CR_HSIDIVEN)
uint32_t getHSI16Clock() {
   if(RCC->CR & RCC_CR_HSIDIVEN) {
      return 16000000UL / 4;
   }
   return 16000000UL;
}
#elif defined(RCC_CR_HSION) && !defined(RCC_CR_HSIDIV)
uint32_t getHSI16Clock() {
   return 16000000UL;
}
#endif

uint32_t getAHBClock() {
   uint32_t clock = SystemCoreClock;
   uint32_t ahbDiv = static_cast<uint32_t>(getAHBPrescaler()) >> RCC_CFGR_HPRE_Pos;
   if(ahbDiv & 0x8) {
      clock = clock / AHBDivTab[ahbDiv & 0x7];
   }
   return clock;
}

#if defined(RCC_CRRCR_HSI48ON) || defined(RCC_CR_HSI48ON)
// HSI48 startup time is 6 us max (STM32L073 and STM32G0B1 datasheets). At
// 64 MHz and a few cycles per iteration this bound is still about 0.5 ms, so
// it only expires if the oscillator does not start at all.
constexpr uint32_t HSI48ReadyLoops = 10000;

bool enableHSI48() {
#if defined(SYSCFG_CFGR3_ENREF_HSI48)
   enableSYSCFGClock();
   if(!(SYSCFG->CFGR3 & SYSCFG_CFGR3_ENREF_HSI48)) {
      if(SYSCFG->CFGR3 & SYSCFG_CFGR3_REF_LOCK) {
         return false;
      }
      SYSCFG->CFGR3 |= SYSCFG_CFGR3_ENREF_HSI48;
   }
#endif
#if defined(RCC_CRRCR_HSI48ON)
   RCC->CRRCR |= RCC_CRRCR_HSI48ON;
#else
   RCC->CR |= RCC_CR_HSI48ON;
#endif
   for(uint32_t i = 0; i < HSI48ReadyLoops; ++i) {
      if(isHSI48Ready()) {
         return true;
      }
   }
   return false;
}

void disableHSI48() {
#if defined(RCC_CRRCR_HSI48ON)
   RCC->CRRCR &= ~RCC_CRRCR_HSI48ON;
#else
   RCC->CR &= ~RCC_CR_HSI48ON;
#endif
#if defined(SYSCFG_CFGR3_ENREF_HSI48)
   if(!(SYSCFG->CFGR3 & SYSCFG_CFGR3_REF_LOCK)) {
      SYSCFG->CFGR3 &= ~SYSCFG_CFGR3_ENREF_HSI48;
   }
#endif
}
#endif

#if defined(RCC_CR_HSIKERDIV)
uint32_t getHSIKerClock() {
   return HSI_VALUE / ((static_cast<uint32_t>(getHSIKerDivider()) >> RCC_CR_HSIKERDIV_Pos) + 1);
}
#elif defined(RCC_CR_HSIDIV)
uint32_t getHSIKerClock() {
   uint32_t pre = (RCC->CR & RCC_CR_HSIDIV) >> RCC_CR_HSIDIV_Pos;
   return HSI_VALUE / (1 << pre);
}
#endif
} // namespace Rcc

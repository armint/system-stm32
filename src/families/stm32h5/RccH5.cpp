#include "peripherals/Rcc.h"

namespace Rcc {
namespace {
// PPREx: 0xx = /1, 100 = /2, 101 = /4, 110 = /8, 111 = /16.
constexpr uint8_t APBShift[8] = { 0, 0, 0, 0, 1, 2, 3, 4 };

uint32_t apbClock(uint32_t ppre) {
   return getAHBClock() >> APBShift[ppre & 0x7];
}

bool waitForSet(const volatile uint32_t &reg, uint32_t mask, uint32_t loops) {
   for(uint32_t i = 0; i < loops; ++i) {
      if((reg & mask) == mask) {
         return true;
      }
   }
   return false;
}

// Increments VOS one level at a time up to VOS0 (RM0481 section 10.4.2)
// and waits for VOSRDY after each step.
bool setVOS0() {
   uint32_t vos = (PWR->VOSCR & PWR_VOSCR_VOS) >> PWR_VOSCR_VOS_Pos;
   while(vos < 3) {
      ++vos;
      PWR->VOSCR = (PWR->VOSCR & ~PWR_VOSCR_VOS) | (vos << PWR_VOSCR_VOS_Pos);
      if(!waitForSet(PWR->VOSSR, PWR_VOSSR_VOSRDY, detail::VOSReadyLoops)) {
         return false;
      }
   }
   return true;
}

// Moves sys_ck to HSI. PLL1 cannot change while it drives sys_ck.
void switchToHSI() {
   RCC->CR |= RCC_CR_HSION;
   while((RCC->CR & RCC_CR_HSIRDY) == 0) {
   }
   RCC->CFGR1 &= ~RCC_CFGR1_SW;
   while((RCC->CFGR1 & RCC_CFGR1_SWS) != 0) {
   }
}

// pllSource: RCC_PLL1CFGR.PLL1SRC. PLL1SRC is the source of all three PLLs
// and the hardware ignores a write while PLL2 or PLL3 is on.
bool setClock250Mhz(uint32_t pllSource, uint32_t inputHz) {
   const auto pll = detail::pll1For250Mhz(inputHz);
   if(!pll) {
      return false;
   }
   switchToHSI();
   // HSI is sys_ck now and the flash latency stays at its old (higher or
   // reset) value, so all exits below leave a valid clock.
   SystemCoreClockUpdate();

   if(!setVOS0()) {
      return false;
   }

   if(pllSource == RCC_PLL1CFGR_PLL1SRC) {
      RCC->CR |= RCC_CR_HSEON;
      if(!waitForSet(RCC->CR, RCC_CR_HSERDY, detail::HSEReadyLoops)) {
         RCC->CR &= ~RCC_CR_HSEON;
         return false;
      }
   }

   RCC->CR &= ~RCC_CR_PLL1ON;
   while(RCC->CR & RCC_CR_PLL1RDY) {
   }
   RCC->PLL1CFGR = pll->cfgr | pllSource;
   if((RCC->PLL1CFGR & RCC_PLL1CFGR_PLL1SRC) != pllSource) {
      return false; // PLL2 or PLL3 is on: the hardware kept the old PLL1SRC.
   }
   RCC->PLL1DIVR = pll->divr;
   RCC->PLL1FRACR = 0;
   RCC->CR |= RCC_CR_PLL1ON;
   if(!waitForSet(RCC->CR, RCC_CR_PLL1RDY, detail::PLLReadyLoops)) {
      RCC->CR &= ~RCC_CR_PLL1ON;
      return false;
   }

   // Flash first, then the faster clock (RM0481 section 7.3.4): 5 wait
   // states and WRHIGHFREQ = 2 for 210 to 250 MHz at VOS0 (table 48).
   constexpr uint32_t acrMask = FLASH_ACR_LATENCY | FLASH_ACR_WRHIGHFREQ | FLASH_ACR_PRFTEN;
   constexpr uint32_t acr = (5UL << FLASH_ACR_LATENCY_Pos) | FLASH_ACR_WRHIGHFREQ_1 | FLASH_ACR_PRFTEN;
   FLASH->ACR = (FLASH->ACR & ~acrMask) | acr;
   while((FLASH->ACR & acrMask) != acr) {
   }

   // hclk and pclk1..3 = sys_ck: all H5 buses run at up to 250 MHz.
   RCC->CFGR2 &= ~(RCC_CFGR2_HPRE | RCC_CFGR2_PPRE1 | RCC_CFGR2_PPRE2 | RCC_CFGR2_PPRE3);
   RCC->CFGR1 |= RCC_CFGR1_SW;
   while((RCC->CFGR1 & RCC_CFGR1_SWS) != RCC_CFGR1_SWS) {
   }
   SystemCoreClockUpdate();
   return true;
}
} // namespace

uint32_t getAPBClock() {
   return apbClock(RCC->CFGR2 >> RCC_CFGR2_PPRE1_Pos);
}

uint32_t getAPB2Clock() {
   return apbClock(RCC->CFGR2 >> RCC_CFGR2_PPRE2_Pos);
}

uint32_t getAPB3Clock() {
   return apbClock(RCC->CFGR2 >> RCC_CFGR2_PPRE3_Pos);
}

uint32_t getPLL1QClock() {
   const uint32_t cfgr = RCC->PLL1CFGR;
   const uint32_t m = (cfgr & RCC_PLL1CFGR_PLL1M) >> RCC_PLL1CFGR_PLL1M_Pos;
   if(!(RCC->CR & RCC_CR_PLL1RDY) || !(cfgr & RCC_PLL1CFGR_PLL1QEN) || m == 0) {
      return 0;
   }
   uint32_t inputHz = 0;
   switch(cfgr & RCC_PLL1CFGR_PLL1SRC) {
   case RCC_PLL1CFGR_PLL1SRC_0:
      inputHz = getHSIClock();
      break;
   case RCC_PLL1CFGR_PLL1SRC_1:
      inputHz = 4000000UL; // CSI
      break;
   case RCC_PLL1CFGR_PLL1SRC:
      inputHz = HSE_VALUE;
      break;
   default:
      return 0;
   }
   // Ignores PLL1FRACN: setClock250Mhz*() sets it to 0.
   const uint32_t divr = RCC->PLL1DIVR;
   const uint32_t n = ((divr & RCC_PLL1DIVR_PLL1N) >> RCC_PLL1DIVR_PLL1N_Pos) + 1;
   const uint32_t q = ((divr & RCC_PLL1DIVR_PLL1Q) >> RCC_PLL1DIVR_PLL1Q_Pos) + 1;
   return inputHz / m * n / q;
}

bool enableHSI48() {
   RCC->CR |= RCC_CR_HSI48ON;
   // Start-up is 4 us max (DS14539). At 250 MHz this bound is still about 0.2 ms.
   return waitForSet(RCC->CR, RCC_CR_HSI48RDY, 10000);
}

bool setClock250MhzHSE() {
   return setClock250Mhz(RCC_PLL1CFGR_PLL1SRC, HSE_VALUE);
}

bool setClock250MhzHSI() {
   return setClock250Mhz(RCC_PLL1CFGR_PLL1SRC_0, getHSIClock());
}
} // namespace Rcc

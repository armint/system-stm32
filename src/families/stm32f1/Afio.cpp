#include "Afio.h"
#include "utils/Interrupt.h"
#include <cassert>

namespace hw::Afio {

namespace {

// SWJ_CFG is write-only: a read returns an undefined value (RM0008 section
// 9.4.2). A read-modify-write of MAPR would write that value back and can
// enable JTAG again, which takes PA15, PB3 and PB4 away from the
// application. Keep the last written value here and write it with every
// MAPR update.
//
// The start value is the reset state. Call setSwjConfig() before any other
// remap if a bootloader can have changed SWJ_CFG.
uint32_t swjConfig = AFIO_MAPR_SWJ_CFG_RESET;

void enableClock() {
   RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
   // Read back, so the clock is on before the first AFIO access.
   (void)RCC->APB2ENR;
}

} // namespace

void setSwjConfig(SwjConfig config) {
   enableClock();
   Interrupt::CriticalSection cs;
   swjConfig = static_cast<uint32_t>(config);
   AFIO->MAPR = (AFIO->MAPR & ~AFIO_MAPR_SWJ_CFG) | swjConfig;
}

void setRemap(uint32_t mask, uint32_t value) {
   assert((mask & AFIO_MAPR_SWJ_CFG) == 0);
   mask &= ~AFIO_MAPR_SWJ_CFG;
   enableClock();
   Interrupt::CriticalSection cs;
   uint32_t mapr = AFIO->MAPR;
   mapr &= ~(mask | AFIO_MAPR_SWJ_CFG);
   mapr |= (value & mask) | swjConfig;
   AFIO->MAPR = mapr;
}

} // namespace hw::Afio

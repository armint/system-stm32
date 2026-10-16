#include "Usart.h"
#include "Rcc.h"

namespace hw {

void Usart::setBaudRate(uint32_t baud_rate) const {
   uint32_t fclk;

   fclk = getClockFrequency();
   if(fclk == 0) {
      return; // kernel clock unknown to getClockFrequency()
   }
   uint32_t brr = fclk;
   const bool over8 = (usart_.CR1 & USART_CR1_OVER8);
   if(over8) {
      brr *= 2;
   }
   brr /= baud_rate;
   /* Make sure we round properly */
   if((fclk % baud_rate) > baud_rate / 2) {
      brr++;
   }
   if(over8) {
      uint16_t temp = (brr & 0x000F) >> 1;
      brr = (brr & 0xFFF0) | temp;
   }
   usart_.BRR = brr;
}

uint32_t Usart::getClockFrequency() const {
   uint32_t clk;
#if defined(RCC_CCIPR1_USART1SEL)
   // STM32H5: one 3-bit kernel clock mux per USART in RCC_CCIPR1, with the
   // same encoding for each USART. USART1 is on APB2, USART2/3 on APB1.
   const auto base = reinterpret_cast<uintptr_t>(&usart_);
   uint32_t sel;
   uint32_t pclk;
   if(base == USART1_BASE) {
      sel = (RCC->CCIPR1 & RCC_CCIPR1_USART1SEL) >> RCC_CCIPR1_USART1SEL_Pos;
      pclk = Rcc::getAPB2Clock();
   } else if(base == USART2_BASE) {
      sel = (RCC->CCIPR1 & RCC_CCIPR1_USART2SEL) >> RCC_CCIPR1_USART2SEL_Pos;
      pclk = Rcc::getAPBClock();
   } else if(base == USART3_BASE) {
      sel = (RCC->CCIPR1 & RCC_CCIPR1_USART3SEL) >> RCC_CCIPR1_USART3SEL_Pos;
      pclk = Rcc::getAPBClock();
   } else {
      return 0; // UART4/5, USART6, UART12: not used by system-stm32 yet
   }
   switch(static_cast<ClockSource>(sel << RCC_CCIPR1_USART1SEL_Pos)) {
   case ClockSource::PCLK:
      clk = pclk;
      break;
   case ClockSource::HSIKER:
      clk = Rcc::getHSIClock();
      break;
   case ClockSource::CSIKER:
      clk = Rcc::CSIHz;
      break;
   case ClockSource::LSE:
      clk = LSE_VALUE;
      break;
   default:
      return 0; // PLL2Q, PLL3Q or reserved
   }
#else
   if(reinterpret_cast<uint32_t>(&usart_) == USART1_BASE) {
      switch(getUSART1ClockSource()) {
      case ClockSource::SYSCLK:
         clk = Rcc::getAHBClock();
         break;
      case ClockSource::HSIKER:
         clk = Rcc::getHSIKerClock();
         break;
      case ClockSource::LSE:
         clk = LSE_VALUE;
         break;
      default:
      case ClockSource::PCLK:
#if defined(RCC_CFGR_PPRE2)
         // USART1 is on APB2 on parts that split APB1/APB2 (e.g. STM32WL).
         clk = Rcc::getAPB2Clock();
#else
         clk = Rcc::getAPBClock();
#endif
         break;
      }
   } else {
      // USART2
      clk = Rcc::getAPBClock();
   }
#endif
   if(usart_.CR1 & USART_CR1_OVER8) {
      clk *= 2;
   }
   return clk;
}

uint32_t Usart::getBaudRate() const {
   uint32_t clk = getClockFrequency();
   // TODO compensate for OVER8
   return clk / usart_.BRR;
}

void Usart::configure(uint32_t baudRate, Parity parity, StopBits stopBits) const {
   setParity(parity);
   setStopBits(stopBits);
   setBaudRate(baudRate);
}


void Usart::setParity(Parity parity) const {
   usart_.CR1 &= ~(USART_CR1_PCE | USART_CR1_PS | USART_CR1_M0 | USART_CR1_M1);
   usart_.CR1 |= static_cast<uint32_t>(parity);

   if(parity != Parity::None) {
      // 9 data bits total (8 data + 1 parity)
      usart_.CR1 |= USART_CR1_M0;
   }
}

bool Usart::setFrameFormat(DataBits dataBits, Parity parity, StopBits stopBits) const {
   // Frame = data bits + parity bit. M1:M0 = 10: 7 bits, 00: 8 bits, 01: 9 bits
   uint32_t cr1Bits = static_cast<uint32_t>(parity);
   if(dataBits == DataBits::Seven && parity == Parity::None) {
      cr1Bits |= USART_CR1_M1;
   } else if(dataBits == DataBits::Eight && parity != Parity::None) {
      cr1Bits |= USART_CR1_M0;
   }

   const bool wasEnabled = isEnabled();
   disable();
   usart_.CR1 = (usart_.CR1 & ~(USART_CR1_PCE | USART_CR1_PS | USART_CR1_M0 | USART_CR1_M1)) | cr1Bits;
   setStopBits(stopBits);
   if(wasEnabled) {
      enable();
   }
   return true;
}

void Usart::configureRs485DriverEnable(bool activeHigh, uint8_t assertionTime, uint8_t deassertionTime) const {
   // Must disable USART to change these settings
   bool wasEnabled = (usart_.CR1 & USART_CR1_UE);
   if(wasEnabled)
      disable();

   // 1. Set Polarity (DEP bit in CR3)
   // DEP=0: DE signal is active high. DEP=1: DE signal is active low.
   if(activeHigh) {
      usart_.CR3 &= ~USART_CR3_DEP;
   } else {
      usart_.CR3 |= USART_CR3_DEP;
   }

   // 2. Set Assertion (DEAT) and Deassertion (DEDT) times in CR1
   // These are 5-bit fields. Limit value to 31.
   uint32_t cr1 = usart_.CR1;
   cr1 &= ~(USART_CR1_DEAT | USART_CR1_DEDT);

   cr1 |= ((assertionTime & 0x1F) << USART_CR1_DEAT_Pos);
   cr1 |= ((deassertionTime & 0x1F) << USART_CR1_DEDT_Pos);
   usart_.CR1 = cr1;

   // 3. Enable Driver Enable Mode (DEM in CR3)
   usart_.CR3 |= USART_CR3_DEM;

   if(wasEnabled)
      enable();
}

#if defined(USART_CR1_FIFOEN)
void Usart::setFifoEnable(bool on) const {
   const bool wasEnabled = isEnabled();
   disable();
   if(on) {
      usart_.CR1 |= USART_CR1_FIFOEN;
   } else {
      usart_.CR1 &= ~USART_CR1_FIFOEN;
   }
   if(wasEnabled) {
      enable();
   }
}
#endif

void Usart::setReceiverTimeout(uint32_t bitDuration) const {
   // RTOR register takes a 24-bit value
   // In standard 16x oversampling, this is the number of baud bits.
   usart_.RTOR = (bitDuration & 0xFFFFFF);

   // Note: You must manually call enableReceiverTimeout() (sets RTOEN in CR2)
   // for this to take effect.
}

} // namespace hw

#include "Usart.h"
#include "Rcc.h"

namespace hw {

void Usart::setBaudRate(uint32_t baud_rate) const {
   uint32_t fclk;

   fclk = getClockFrequency();
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

void Usart::setReceiverTimeout(uint32_t bitDuration) const {
   // RTOR register takes a 24-bit value
   // In standard 16x oversampling, this is the number of baud bits.
   usart_.RTOR = (bitDuration & 0xFFFFFF);

   // Note: You must manually call enableReceiverTimeout() (sets RTOEN in CR2)
   // for this to take effect.
}

} // namespace hw

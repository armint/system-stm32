#include "peripherals/Rcc.h"
#include "peripherals/Usart.h"

namespace hw {

void Usart::setBaudRate(uint32_t baudRate) const {
   // 16x oversampling: BRR = DIV_Mantissa << 4 | DIV_Fraction = fCK / baud,
   // so the fractional baud rate generator is one rounded division.
   const uint32_t fclk = getClockFrequency();
   usart_.BRR = (fclk + baudRate / 2) / baudRate;
}

uint32_t Usart::getClockFrequency() const {
   if(reinterpret_cast<uint32_t>(&usart_) == USART1_BASE) {
      return Rcc::getAPB2Clock();
   }
   return Rcc::getAPBClock();
}

uint32_t Usart::getBaudRate() const {
   const uint32_t brr = usart_.BRR;
   if(brr == 0) {
      return 0;
   }
   return getClockFrequency() / brr;
}

void Usart::configure(uint32_t baudRate, Parity parity, StopBits stopBits) const {
   setParity(parity);
   setStopBits(stopBits);
   setBaudRate(baudRate);
}

void Usart::setParity(Parity parity) const {
   usart_.CR1 &= ~(USART_CR1_PCE | USART_CR1_PS | USART_CR1_M);
   usart_.CR1 |= static_cast<uint32_t>(parity);

   if(parity != Parity::None) {
      // 9 data bits total (8 data + 1 parity)
      usart_.CR1 |= USART_CR1_M;
   }
}

bool Usart::setFrameFormat(DataBits dataBits, Parity parity, StopBits stopBits) const {
   uint32_t cr1Bits = static_cast<uint32_t>(parity);
   if(dataBits == DataBits::Eight && parity != Parity::None) {
      cr1Bits |= USART_CR1_M;
   } else if(dataBits == DataBits::Seven && parity == Parity::None) {
      return false;
   }

   const bool wasEnabled = isEnabled();
   disable();
   usart_.CR1 = (usart_.CR1 & ~(USART_CR1_PCE | USART_CR1_PS | USART_CR1_M)) | cr1Bits;
   setStopBits(stopBits);
   if(wasEnabled) {
      enable();
   }
   return true;
}

} // namespace hw

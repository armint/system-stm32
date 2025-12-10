#include "Uart.h"

void Uart::setBaudRate(uint32_t baud_rate) const {
   // Note: Ensure this matches your actual peripheral clock (PCLK1/PCLK2)
   uint32_t fclk = SystemCoreClock;
   uint64_t brr = fclk;
   const bool over8 = (usart.CR1 & USART_CR1_OVER8);
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
   usart.BRR = brr;
}

void Uart::setParity(USARTParity parity) const {
   usart.CR1 &= ~(USART_CR1_PCE | USART_CR1_PS | USART_CR1_M0 | USART_CR1_M1);
   usart.CR1 |= static_cast<uint32_t>(parity);

   if(parity != USARTParity::None) {
      // 9 data bits total (8 data + 1 parity)
      usart.CR1 |= USART_CR1_M1;
   }
}

void Uart::setStopBits(UsartStopBits stopBits) const {
   usart.CR2 &= ~USART_CR2_STOP;
   usart.CR2 |= static_cast<uint32_t>(stopBits);
}

void Uart::configureRs485DriverEnable(bool activeHigh, uint8_t assertionTime, uint8_t deassertionTime) const {
   // Must disable USART to change these settings
   bool wasEnabled = (usart.CR1 & USART_CR1_UE);
   if(wasEnabled)
      disable();

   // 1. Set Polarity (DEP bit in CR3)
   // DEP=0: DE signal is active high. DEP=1: DE signal is active low.
   if(activeHigh) {
      usart.CR3 &= ~USART_CR3_DEP;
   } else {
      usart.CR3 |= USART_CR3_DEP;
   }

   // 2. Set Assertion (DEAT) and Deassertion (DEDT) times in CR1
   // These are 5-bit fields. Limit value to 31.
   uint32_t cr1 = usart.CR1;
   cr1 &= ~(USART_CR1_DEAT | USART_CR1_DEDT);

   cr1 |= ((assertionTime & 0x1F) << USART_CR1_DEAT_Pos);
   cr1 |= ((deassertionTime & 0x1F) << USART_CR1_DEDT_Pos);
   usart.CR1 = cr1;

   // 3. Enable Driver Enable Mode (DEM in CR3)
   usart.CR3 |= USART_CR3_DEM;

   if(wasEnabled)
      enable();
}

void Uart::setReceiverTimeout(uint32_t bitDuration) const {
   // RTOR register takes a 24-bit value
   // In standard 16x oversampling, this is the number of baud bits.
   usart.RTOR = (bitDuration & 0xFFFFFF);

   // Note: You must manually call enableReceiverTimeout() (sets RTOEN in CR2)
   // for this to take effect.
}

void Uart::configure(uint32_t baudRate, USARTParity parity, UsartStopBits stopBits) const {
   disable();
   setParity(parity);
   setStopBits(stopBits);
   setBaudRate(baudRate);
   enableTx();
   enableRx();
   enable();
}
#include "Uart.h"

void Uart::setBaudRate(uint32_t baudRate) const {
   // Note: Ensure this matches your actual peripheral clock (PCLK1/PCLK2)
   uint32_t fclk = SystemCoreClock;
   uint32_t brr = 0;

   // 1. Check if Over8 mode is enabled in Control Register 1 (CR1)
   const bool over8 = (usart.CR1 & USART_CR1_OVER8) != 0;

   // N = Oversampling factor (16 or 8)
   const uint32_t N = over8 ? 8 : 16;

   // Use uint64_t to prevent overflow during the intermediate calculation
   // formula: DIV = (fCLK * 1000) / (N * BaudRate)
   uint64_t div_base = (uint64_t)fclk * 1000 / (N * baudRate);

   // 2. Calculate Mantissa (Integer part)
   uint32_t mantissa = (uint32_t)(div_base / 1000);

   // 3. Calculate Fraction
   // We get the remainder (0-999) and scale it to the number of sampling steps (8 or 16)
   uint32_t fraction_scaled = (uint32_t)(div_base % 1000);
   uint32_t fraction;

   if(over8) {
      // 8x Oversampling: 3 bits for fraction
      // We multiply by 8 to map 0-1000 to 0-8.
      // Adding 500 provides rounding to the nearest integer.
      fraction = (fraction_scaled * 8 + 500) / 1000;

      // In Over8 mode, bits [2:0] are shifted right by 1 bit in the BRR,
      // or effectively we map them to bits [3:0] where bit 3 must be 0.
      // However, the easier mental model for STM32 BRR in Over8 is:
      // BRR[3] must be 0.
      // BRR[2:0] = fraction.
      // Mantissa goes to BRR[15:4].

      // Mask to ensure 3 bits, then combined with mantissa
      brr = (mantissa << 4) | (fraction & 0x7);

   } else {
      // 16x Oversampling: 4 bits for fraction
      // Map 0-1000 to 0-16.
      fraction = (fraction_scaled * 16 + 500) / 1000;

      // Standard layout: Mantissa in [15:4], Fraction in [3:0]
      brr = (mantissa << 4) | (fraction & 0xF);
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
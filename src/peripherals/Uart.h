#pragma once

#include <cmsis_device.h>
#include <cstdint>

// The Stop Bits values map to the STOP[1:0] field in USART_CR2
enum class UsartStopBits {
   StopBits_1 = 0,
   StopBits_0_5 = USART_CR2_STOP_0,
   StopBits_2 = USART_CR2_STOP_1,
   StopBits_1_5 = USART_CR2_STOP_0 | USART_CR2_STOP_1,
};

// The Parity values map to the PCE (Parity Control Enable) and PS (Parity Selection) bits in USART_CR1
enum class USARTParity {
   None = 0,
   Even = USART_CR1_PCE,
   Odd = (USART_CR1_PCE | USART_CR1_PS),
};

class Uart {
   public:
   USART_TypeDef &usart;

   Uart(USART_TypeDef *uartBase)
      : usart(*uartBase) {
   }
   // =============================================================
   // Basic Enable/Disable
   // =============================================================
   void enable() const {
      usart.CR1 |= USART_CR1_UE;
   }
   void disable() const {
      usart.CR1 &= ~USART_CR1_UE;
   }

   void enableTx() const {
      usart.CR1 |= USART_CR1_TE;
   }
   void disableTx() const {
      usart.CR1 &= ~USART_CR1_TE;
   }

   void enableRx() const {
      usart.CR1 |= USART_CR1_RE;
   }
   void disableRx() const {
      usart.CR1 &= ~USART_CR1_RE;
   }

   // =============================================================
   // Interrupts
   // =============================================================
   // RXNE: Receive Data Register Not Empty (Byte received)
   void enableRxNotEmptyInterrupt() const {
      usart.CR1 |= USART_CR1_RXNEIE;
   }
   void disableRxNotEmptyInterrupt() const {
      usart.CR1 &= ~USART_CR1_RXNEIE;
   }
   bool isRxNotEmptyInterruptEnabled() const {
      return (usart.CR1 & USART_CR1_RXNEIE) != 0;
   }

   // TXE: Transmit Data Register Empty (Ready for next byte)
   void enableTxEmptyInterrupt() const {
      usart.CR1 |= USART_CR1_TXEIE;
   }
   void disableTxEmptyInterrupt() const {
      usart.CR1 &= ~USART_CR1_TXEIE;
   }
   bool isTxEmptyInterruptEnabled() const {
      return (usart.CR1 & USART_CR1_TXEIE) != 0;
   }

   // TC: Transmission Complete (All data shifted out)
   void enableTxCompleteInterrupt() const {
      usart.CR1 |= USART_CR1_TCIE;
   }
   void disableTxCompleteInterrupt() const {
      usart.CR1 &= ~USART_CR1_TCIE;
   }
   bool isTxCompleteInterruptEnabled() const {
      return (usart.CR1 & USART_CR1_TCIE) != 0;
   }

   // IDLE: Idle Line Detected (Useful for packet end detection)
   void enableIdleLineInterrupt() const {
      usart.CR1 |= USART_CR1_IDLEIE;
   }
   void disableIdleLineInterrupt() const {
      usart.CR1 &= ~USART_CR1_IDLEIE;
   }
   bool isIdleLineInterruptEnabled() const {
      return (usart.CR1 & USART_CR1_IDLEIE) != 0;
   }

   // PE: Parity Error
   void enableParityErrorInterrupt() const {
      usart.CR1 |= USART_CR1_PEIE;
   }
   void disableParityErrorInterrupt() const {
      usart.CR1 &= ~USART_CR1_PEIE;
   }
   bool isParityErrorInterruptEnabled() const {
      return (usart.CR1 & USART_CR1_PEIE) != 0;
   }

   // EIE: Error Interrupt (Frame Error, Noise Error, Overrun Error)
   void enableErrorInterrupt() const {
      usart.CR3 |= USART_CR3_EIE;
   }
   void disableErrorInterrupt() const {
      usart.CR3 &= ~USART_CR3_EIE;
   }
   bool isErrorInterruptEnabled() const {
      return (usart.CR3 & USART_CR3_EIE) != 0;
   }

   // =============================================================
   // DMA Support
   // =============================================================
   void enableDmaTx() const {
      usart.CR3 |= USART_CR3_DMAT;
   }
   void disableDmaTx() const {
      usart.CR3 &= ~USART_CR3_DMAT;
   }

   void enableDmaRx() const {
      usart.CR3 |= USART_CR3_DMAR;
   }
   void disableDmaRx() const {
      usart.CR3 &= ~USART_CR3_DMAR;
   }

   // =============================================================
   // Hardware Flow Control & Modes
   // =============================================================
   void enableRts() const {
      usart.CR3 |= USART_CR3_RTSE;
   }
   void disableRts() const {
      usart.CR3 &= ~USART_CR3_RTSE;
   }

   void enableCts() const {
      usart.CR3 |= USART_CR3_CTSE;
   }
   void disableCts() const {
      usart.CR3 &= ~USART_CR3_CTSE;
   }

   void enableHalfDuplex() const {
      usart.CR3 |= USART_CR3_HDSEL;
   }
   void disableHalfDuplex() const {
      usart.CR3 &= ~USART_CR3_HDSEL;
   }

   void enable8xOversampling() const {
      usart.CR1 |= USART_CR1_OVER8;
   }

   void disable8xOversampling() const {
      usart.CR1 &= ~USART_CR1_OVER8;
   }

   /**
    * @brief Configures Hardware Driver Enable for RS485.
    * @param activeHigh If true, DE pin is high when transmitting.
    * @param assertionTime Time (in sample clocks) DE is active before start bit. (0-31)
    * @param deassertionTime Time (in sample clocks) DE remains active after stop bit. (0-31)
    */
   void configureRs485DriverEnable(bool activeHigh = true, uint8_t assertionTime = 0, uint8_t deassertionTime = 0) const;

   /**
    * @brief Sets the Receiver Timeout.
    * @param bitDuration Timeout duration in bit times.
    */
   void setReceiverTimeout(uint32_t bitDuration) const;

   void enableReceiverTimeout() const {
      usart.CR2 |= USART_CR2_RTOEN;
   }
   void disableReceiverTimeout() const {
      usart.CR2 &= ~USART_CR2_RTOEN;
   }

   // =============================================================
   // Data & Status
   // =============================================================
   void write(uint8_t data) const {
      usart.TDR = data;
   }
   uint8_t read() const {
      return usart.RDR;
   }
   // Generic data accessors
   void setData(uint8_t data) const {
      write(data);
   }
   uint8_t getData() const {
      return read();
   }

   bool getStatusFlag(uint32_t flag) const {
      return (usart.ISR & flag) == flag;
   }
   void clearStatusFlag(uint32_t flag) const {
      usart.ICR = flag;
   } // W1C (Write 1 to Clear)

   // Clears all known status flags that are cleared by writing 1 to USART_ICR
   void clearAllStatusFlags() const {
      uint32_t mask = 0;
#ifdef USART_ICR_PECF
      mask |= USART_ICR_PECF;   // Parity error clear
#endif
#ifdef USART_ICR_FECF
      mask |= USART_ICR_FECF;   // Framing error clear
#endif
#ifdef USART_ICR_NECF
      mask |= USART_ICR_NECF;   // Noise error clear
#endif
#ifdef USART_ICR_ORECF
      mask |= USART_ICR_ORECF;  // Overrun error clear
#endif
#ifdef USART_ICR_IDLECF
      mask |= USART_ICR_IDLECF; // IDLE line detected clear
#endif
#ifdef USART_ICR_TCCF
      mask |= USART_ICR_TCCF;   // Transmission complete clear
#endif
#ifdef USART_ICR_EOBCF
      mask |= USART_ICR_EOBCF;  // End of block clear (Smartcard/Receiver timeout end)
#endif
#ifdef USART_ICR_RTOCF
      mask |= USART_ICR_RTOCF;  // Receiver timeout clear
#endif
#ifdef USART_ICR_CMCF
      mask |= USART_ICR_CMCF;   // Character match clear
#endif
#ifdef USART_ICR_WUCF
      mask |= USART_ICR_WUCF;   // Wake-up from Stop mode clear
#endif
#ifdef USART_ICR_CTSCF
      mask |= USART_ICR_CTSCF;  // CTS flag clear (if present)
#endif
      if (mask != 0) {
         usart.ICR = mask;
      }
   }

   // Convenience status helpers
   bool isTxRegisterEmpty() const {
#if defined(USART_ISR_TXE)
      return getStatusFlag(USART_ISR_TXE);
#elif defined(USART_ISR_TXE_TXFNF)
      return getStatusFlag(USART_ISR_TXE_TXFNF);
#else
      return getStatusFlag(1U << 7); // Fallback to TXE bit position
#endif
   }

   bool isRxRegisterNotEmpty() const {
#if defined(USART_ISR_RXNE)
      return getStatusFlag(USART_ISR_RXNE);
#elif defined(USART_ISR_RXNE_RXFNE)
      return getStatusFlag(USART_ISR_RXNE_RXFNE);
#else
      return getStatusFlag(1U << 5); // Fallback to RXNE bit position
#endif
   }

   void sendBreak() const {
      usart.RQR |= USART_RQR_SBKRQ;
   }

   // Configuration
   void setBaudRate(uint32_t baud_rate) const;
   void setParity(USARTParity parity) const;
   void setStopBits(UsartStopBits stopBits) const;
   void configure(uint32_t baudRate = 115200, USARTParity parity = USARTParity::None, UsartStopBits stopBits = UsartStopBits::StopBits_1) const;
};
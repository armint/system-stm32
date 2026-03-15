#pragma once

#include <cmsis_device.h>

#if defined(STM32C0)
#define USART_CR1_RXNEIE USART_CR1_RXNEIE_RXFNEIE
#define USART_CR1_TXEIE USART_CR1_TXEIE_TXFNFIE
#define USART_ISR_RXNE USART_ISR_RXNE_RXFNE
#define USART_ISR_TXE USART_ISR_TXE_TXFNF
#endif

namespace hw {

struct Usart {
   enum class StopBits {
      StopBits_1 = 0,
      StopBits_0_5 = USART_CR2_STOP_0,
      StopBits_2 = USART_CR2_STOP_1,
      StopBits_1_5 = USART_CR2_STOP_0 | USART_CR2_STOP_1,

   };

   enum class Parity {
      None = 0,
      Even = USART_CR1_PCE,
      Odd = (USART_CR1_PCE | USART_CR1_PS),
   };

#if defined(USART_PRESC_PRESCALER)
   enum class Prescaler : uint32_t {
      Div1 = 0,
      Div2 = (USART_PRESC_PRESCALER_0),
      Div4 = (USART_PRESC_PRESCALER_1),
      Div6 = (USART_PRESC_PRESCALER_1 | USART_PRESC_PRESCALER_0),
      Div8 = (USART_PRESC_PRESCALER_2),
      Div10 = (USART_PRESC_PRESCALER_2 | USART_PRESC_PRESCALER_0),
      Div12 = (USART_PRESC_PRESCALER_2 | USART_PRESC_PRESCALER_1),
      Div16 = (USART_PRESC_PRESCALER_2 | USART_PRESC_PRESCALER_1 | USART_PRESC_PRESCALER_0),
      Div32 = (USART_PRESC_PRESCALER_3),
      Div64 = (USART_PRESC_PRESCALER_3 | USART_PRESC_PRESCALER_0),
      Div128 = (USART_PRESC_PRESCALER_3 | USART_PRESC_PRESCALER_1),
      Div256 = (USART_PRESC_PRESCALER_3 | USART_PRESC_PRESCALER_1 | USART_PRESC_PRESCALER_0),
   };
#endif
   enum class ClockSource : uint32_t {
      PCLK = 0,
      SYSCLK = RCC_CCIPR_USART1SEL_0,
      HSIKER = RCC_CCIPR_USART1SEL_1,
      LSE = RCC_CCIPR_USART1SEL_1 | RCC_CCIPR_USART1SEL_0,
   };

   enum class AutoBaudRateMode : uint32_t {
      StartBit = 0x0,    // Detect baud rate based on start bit only
      EdgeToEdge = 0x1,  // Measure from falling edge to falling edge
      TestFrame7F = 0x2, // Use frame 0x7F as measurement
      TestFrame55 = 0x3  // Use frame 0x55 as measurement
   };


   USART_TypeDef &usart_;

   Usart(USART_TypeDef *uartBase)
      : usart_(*uartBase) {}

   Usart(const Usart &) = default;
   Usart(Usart &&) = default;
   Usart &operator=(const Usart &) = delete;
   Usart &operator=(Usart &&) = delete;

   void configure(uint32_t baudRate = 115200, Parity parity = Parity::None, StopBits stopBits = StopBits::StopBits_1) const;

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
      usart_.CR2 |= USART_CR2_RTOEN;
   }
   void disableReceiverTimeout() const {
      usart_.CR2 &= ~USART_CR2_RTOEN;
   }

   void enableReceiverTimeoutInterrupt() const {
      usart_.CR1 |= USART_CR1_RTOIE;
   }
   void disableReceiverTimeoutInterrupt() const {
      usart_.CR1 &= ~USART_CR1_RTOIE;
   }
   bool isReceiverTimeoutInterruptEnabled() const {
      return (usart_.CR1 & USART_CR1_RTOIE) != 0;
   }

   inline bool isReceiverTimeoutFlag() const {
      return usart_.ISR & USART_ISR_RTOF;
   }

   inline void clearReceiverTimeoutFlag() const {
#ifdef USART_ICR_RTOCF
      usart_.ICR = USART_ICR_RTOCF;
#endif
   }

   inline void enableTx() const {
      usart_.CR1 |= USART_CR1_TE;
   }

   inline void disableTx() const {
      usart_.CR1 &= ~USART_CR1_TE;
   }

   inline void enableRx() const {
      usart_.CR1 |= USART_CR1_RE;
   }

   inline void disableRx() const {
      usart_.CR1 &= ~USART_CR1_RE;
   }

   inline void setPolarityHigh() const {
      usart_.CR2 |= USART_CR2_CPOL;
   }

   inline void setPolarityLow() const {
      usart_.CR2 &= ~USART_CR2_CPOL;
   }

   inline void enableTxDma() const {
      usart_.CR3 |= USART_CR3_DMAT;
   }

   inline void enableRxDma() const {
      usart_.CR3 |= USART_CR3_DMAR;
   }

   inline uint8_t getData() const {
      return usart_.RDR;
   }

   inline void setData(uint8_t data) const {
      usart_.TDR = data;
   }

   inline bool isRxRegisterNotEmpty() const {
#if defined(USART_ISR_RXNE_RXFNE)
      return usart_.ISR & USART_ISR_RXNE_RXFNE;
#else
      return usart_.ISR & USART_ISR_RXNE;
#endif
   }

   inline bool isTxRegisterEmpty() const {
#if defined(USART_ISR_RXNE_RXFNE)
      return usart_.ISR & USART_ISR_TXE_TXFNF;
#else
      return usart_.ISR & USART_ISR_TXE;
#endif
   }

   inline bool isTransferComplete() const {
      return usart_.ISR & USART_ISR_TC;
   }

   inline void disable() const {
      usart_.CR1 &= ~USART_CR1_UE;
   }
   inline void enable() const {
      usart_.CR1 |= USART_CR1_UE;
   }

   inline bool isEnabled() const {
      return usart_.CR1 & USART_CR1_UE;
   }

   void setParity(Parity parity) const;

   inline void setStopBits(StopBits bits) const {
      usart_.CR2 = (usart_.CR2 & ~USART_CR2_STOP) | static_cast<uint32_t>(bits);
   }

   // =============================================================
   // Interrupts
   // =============================================================
   // RXNE: Receive Data Register Not Empty (Byte received)
   void enableRxNotEmptyInterrupt() const {
#if defined(USART_ISR_RXNE_RXFNE)
      usart_.CR1 |= USART_CR1_RXNEIE_RXFNEIE;
#else
      usart_.CR1 |= USART_CR1_RXNEIE;
#endif
   }
   void disableRxNotEmptyInterrupt() const {
#if defined(USART_ISR_RXNE_RXFNE)
      usart_.CR1 &= ~USART_CR1_RXNEIE_RXFNEIE;
#else
      usart_.CR1 &= ~USART_CR1_RXNEIE;
#endif
   }
   bool isRxNotEmptyInterruptEnabled() const {
#if defined(USART_ISR_RXNE_RXFNE)
      return (usart_.CR1 & USART_CR1_RXNEIE_RXFNEIE) != 0;
#else
      return (usart_.CR1 & USART_CR1_RXNEIE) != 0;
#endif
   }

   // TXE: Transmit Data Register Empty (Ready for next byte)
   void enableTxEmptyInterrupt() const {
#if defined(USART_ISR_RXNE_RXFNE)
      usart_.CR1 |= USART_CR1_TXEIE_TXFNFIE;
#else
      usart_.CR1 |= USART_CR1_TXEIE;
#endif
   }
   void disableTxEmptyInterrupt() const {
#if defined(USART_ISR_RXNE_RXFNE)
      usart_.CR1 &= ~USART_CR1_TXEIE_TXFNFIE;
#else
      usart_.CR1 &= ~USART_CR1_TXEIE;
#endif
   }
   bool isTxEmptyInterruptEnabled() const {
#if defined(USART_ISR_RXNE_RXFNE)
      return (usart_.CR1 & USART_CR1_TXEIE_TXFNFIE) != 0;
#else
      return (usart_.CR1 & USART_CR1_TXEIE) != 0;
#endif
   }

   // TC: Transmission Complete (All data shifted out)
   void enableTxCompleteInterrupt() const {
      usart_.CR1 |= USART_CR1_TCIE;
   }
   void disableTxCompleteInterrupt() const {
      usart_.CR1 &= ~USART_CR1_TCIE;
   }
   bool isTxCompleteInterruptEnabled() const {
      return (usart_.CR1 & USART_CR1_TCIE) != 0;
   }

   // IDLE: Idle Line Detected (Useful for packet end detection)
   void enableIdleLineInterrupt() const {
      usart_.CR1 |= USART_CR1_IDLEIE;
   }
   void disableIdleLineInterrupt() const {
      usart_.CR1 &= ~USART_CR1_IDLEIE;
   }
   bool isIdleLineInterruptEnabled() const {
      return (usart_.CR1 & USART_CR1_IDLEIE) != 0;
   }

   // PE: Parity Error
   void enableParityErrorInterrupt() const {
      usart_.CR1 |= USART_CR1_PEIE;
   }
   void disableParityErrorInterrupt() const {
      usart_.CR1 &= ~USART_CR1_PEIE;
   }
   bool isParityErrorInterruptEnabled() const {
      return (usart_.CR1 & USART_CR1_PEIE) != 0;
   }

   // EIE: Error Interrupt (Frame Error, Noise Error, Overrun Error)
   void enableErrorInterrupt() const {
      usart_.CR3 |= USART_CR3_EIE;
   }
   void disableErrorInterrupt() const {
      usart_.CR3 &= ~USART_CR3_EIE;
   }
   bool isErrorInterruptEnabled() const {
      return (usart_.CR3 & USART_CR3_EIE) != 0;
   }

   // IDLE line interrupt and flag helpers
   inline void enableIdleInterrupt() const {
      usart_.CR1 |= USART_CR1_IDLEIE;
   }

   inline void disableIdleInterrupt() const {
      usart_.CR1 &= ~USART_CR1_IDLEIE;
   }

   inline bool isIdleInterruptEnabled() const {
      return usart_.CR1 & USART_CR1_IDLEIE;
   }

   inline bool isIdleFlagSet() const {
      return usart_.ISR & USART_ISR_IDLE;
   }

   inline void clearIdleFlag() const {
      usart_.ICR |= USART_ICR_IDLECF;
   }

   void setBaudRate(uint32_t baudRate) const;

   uint32_t getBaudRate() const;

   inline void setBRR(uint32_t BRR) const {
      usart_.BRR = BRR;
   }

   inline volatile USART_TypeDef *getUsart() const {
      return &usart_;
   }

   inline bool getStatusFlag(uint32_t flag) const {
      return (usart_.ISR & flag) == flag;
   }

   inline uint32_t returnStatusFlag() const {
      return (usart_.ISR);
   }

   inline void clearStatusFlag(uint32_t flag) const {
      usart_.ISR |= flag;
   }


   inline void sendBreak() const {
      usart_.RQR |= USART_RQR_SBKRQ;
   }

   inline bool isFramingError() const {
      return usart_.ISR & USART_ISR_FE;
   }

   inline void clearFramingError() const {
#ifdef USART_ICR_FECF
      usart_.ICR = USART_ICR_FECF;
#endif
   }

#if defined(USART_PRESC_PRESCALER)
   inline void setPrescaler(Prescaler prescaler) const {
      usart_.PRESC = (usart_.PRESC & ~(USART_PRESC_PRESCALER)) | static_cast<uint32_t>(prescaler);
   }
#endif

   static inline void setUSART1ClockSource(ClockSource clockSource) {
      RCC->CCIPR = (RCC->CCIPR & ~RCC_CCIPR_USART1SEL) | static_cast<uint32_t>(clockSource);
   }

   static inline ClockSource getUSART1ClockSource() {
      return static_cast<ClockSource>(RCC->CCIPR & RCC_CCIPR_USART1SEL);
   }

   inline void enablePinSwap() const {
      usart_.CR2 |= USART_CR2_SWAP;
   }

   inline void disablePinSwap() const {
      usart_.CR2 &= ~USART_CR2_SWAP;
   }

   inline bool isPinSwapEnabled() const {
      return usart_.CR2 & USART_CR2_SWAP;
   }

   inline void enableHalfDuplexMode() const {
      usart_.CR3 |= USART_CR3_HDSEL;
   }

   inline void disableHalfDuplexMode() const {
      usart_.CR3 &= ~USART_CR3_HDSEL;
   }

   inline bool isHalfDuplexModeEnabled() const {
      return usart_.CR3 & USART_CR3_HDSEL;
   }

   uint32_t getClockFrequency() const;


   inline void enableAutoBaudRateDetection() const {
      usart_.CR2 |= USART_CR2_ABREN;
   }

   inline void disableAutoBaudRateDetection() const {
      usart_.CR2 &= ~USART_CR2_ABREN;
   }

   inline bool isAutoBaudRateDetectionComplete() const {
      return usart_.ISR & USART_ISR_ABRF;
   }

   inline void clearAutoBaudRateDetectionFlag() const {
      usart_.ICR |= USART_ISR_ABRF;
   }

   inline void setAutoBaudRateMode(AutoBaudRateMode mode) const {
      usart_.CR2 = (usart_.CR2 & ~USART_CR2_ABRMODE) | (static_cast<uint32_t>(mode) << USART_CR2_ABRMODE_Pos);
   }

   // =============================================================
   // DMA Support
   // =============================================================
   void enableDmaTx() const {
      usart_.CR3 |= USART_CR3_DMAT;
   }
   void disableDmaTx() const {
      usart_.CR3 &= ~USART_CR3_DMAT;
   }

   void enableDmaRx() const {
      usart_.CR3 |= USART_CR3_DMAR;
   }
   void disableDmaRx() const {
      usart_.CR3 &= ~USART_CR3_DMAR;
   }

   // =============================================================
   // Hardware Flow Control & Modes
   // =============================================================
   void enableRts() const {
      usart_.CR3 |= USART_CR3_RTSE;
   }
   void disableRts() const {
      usart_.CR3 &= ~USART_CR3_RTSE;
   }

   void enableCts() const {
      usart_.CR3 |= USART_CR3_CTSE;
   }
   void disableCts() const {
      usart_.CR3 &= ~USART_CR3_CTSE;
   }

   void enableHalfDuplex() const {
      usart_.CR3 |= USART_CR3_HDSEL;
   }
   void disableHalfDuplex() const {
      usart_.CR3 &= ~USART_CR3_HDSEL;
   }

   void enable8xOversampling() const {
      usart_.CR1 |= USART_CR1_OVER8;
   }

   void disable8xOversampling() const {
      usart_.CR1 &= ~USART_CR1_OVER8;
   }

   // Clears all known status flags that are cleared by writing 1 to USART_ICR
   void clearAllStatusFlags() const {
      uint32_t mask = 0;
#ifdef USART_ICR_PECF
      mask |= USART_ICR_PECF; // Parity error clear
#endif
#ifdef USART_ICR_FECF
      mask |= USART_ICR_FECF; // Framing error clear
#endif
#ifdef USART_ICR_NECF
      mask |= USART_ICR_NECF; // Noise error clear
#endif
#ifdef USART_ICR_ORECF
      mask |= USART_ICR_ORECF; // Overrun error clear
#endif
#ifdef USART_ICR_IDLECF
      mask |= USART_ICR_IDLECF; // IDLE line detected clear
#endif
#ifdef USART_ICR_TCCF
      mask |= USART_ICR_TCCF; // Transmission complete clear
#endif
#ifdef USART_ICR_EOBCF
      mask |= USART_ICR_EOBCF; // End of block clear (Smartcard/Receiver timeout end)
#endif
#ifdef USART_ICR_RTOCF
      mask |= USART_ICR_RTOCF; // Receiver timeout clear
#endif
#ifdef USART_ICR_CMCF
      mask |= USART_ICR_CMCF; // Character match clear
#endif
#ifdef USART_ICR_WUCF
      mask |= USART_ICR_WUCF; // Wake-up from Stop mode clear
#endif
#ifdef USART_ICR_CTSCF
      mask |= USART_ICR_CTSCF; // CTS flag clear (if present)
#endif
      if(mask != 0) {
         usart_.ICR = mask;
      }
   }
};

} // namespace hw

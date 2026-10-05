#pragma once

// USART v1 (STM32F1, RM0008 section 27): one status register (SR) and one
// data register (DR) instead of ISR/ICR/RDR/TDR, a single word length bit
// (M: 8 or 9 bits), no clock source mux, no receiver timeout, no RS485 driver
// enable, no auto baud rate and no 8x oversampling.
//
// Do not include this file directly; include "peripherals/Usart.h". The class
// has the same name and the same interface as the USART v2 driver, minus the
// functions the v1 hardware does not have.
//
// Status flags: PE, FE, NE, ORE and IDLE are cleared by a read of SR followed
// by a read of DR. The usual receive path (check a flag, then getData())
// clears them as a side effect. TC, LBD and CTS are cleared by writing 0;
// write 1 to the other bits, because writing 0 to RXNE would drop a byte.

#include <cmsis_device.h>
#include <cstdint>

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

   // Data bits without the parity bit
   enum class DataBits {
      Seven = 7,
      Eight = 8,
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
    * @brief Sets data bits, parity and stop bits (e.g. from a USB CDC line
    * coding). The frame length is data bits + parity bit: 7E/7O use M = 0,
    * 8N uses M = 0, 8E/8O use M = 1.
    * Disables the USART for the change and restores UE afterwards. Wait for
    * isTransferComplete() first, or a byte in transmission is cut off.
    * @return false (nothing changed) for 7 data bits without parity: v1 has
    * no 7-bit frame.
    */
   [[nodiscard]] bool setFrameFormat(DataBits dataBits, Parity parity, StopBits stopBits) const;

   // USART v1 has no receiver timeout. These two exist so that receive code
   // shared with the v2 families compiles; the flag is never set.
   inline bool isReceiverTimeoutFlag() const {
      return false;
   }
   inline void clearReceiverTimeoutFlag() const {}

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

   // With parity in an 8-bit frame (7 data bits), DR bit 7 is the received
   // parity bit, so it is masked off. In a 9-bit frame the parity bit is bit
   // 8 and the cast drops it.
   inline uint8_t getData() const {
      const uint32_t dr = usart_.DR;
      if((usart_.CR1 & (USART_CR1_PCE | USART_CR1_M)) == USART_CR1_PCE) {
         return static_cast<uint8_t>(dr & 0x7FU);
      }
      return static_cast<uint8_t>(dr);
   }

   inline void setData(uint8_t data) const {
      usart_.DR = data;
   }

   inline bool isRxRegisterNotEmpty() const {
      return usart_.SR & USART_SR_RXNE;
   }

   inline bool isTxRegisterEmpty() const {
      return usart_.SR & USART_SR_TXE;
   }

   inline bool isTransferComplete() const {
      return usart_.SR & USART_SR_TC;
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
   // RXNE: Receive Data Register Not Empty (Byte received). On v1 this
   // interrupt also fires for an overrun (ORE).
   void enableRxNotEmptyInterrupt() const {
      usart_.CR1 |= USART_CR1_RXNEIE;
   }
   void disableRxNotEmptyInterrupt() const {
      usart_.CR1 &= ~USART_CR1_RXNEIE;
   }
   bool isRxNotEmptyInterruptEnabled() const {
      return (usart_.CR1 & USART_CR1_RXNEIE) != 0;
   }

   // TXE: Transmit Data Register Empty (Ready for next byte)
   void enableTxEmptyInterrupt() const {
      usart_.CR1 |= USART_CR1_TXEIE;
   }
   void disableTxEmptyInterrupt() const {
      usart_.CR1 &= ~USART_CR1_TXEIE;
   }
   bool isTxEmptyInterruptEnabled() const {
      return (usart_.CR1 & USART_CR1_TXEIE) != 0;
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
   void clearTxCompleteFlag() const {
      usart_.SR = ~USART_SR_TC;
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
      return usart_.SR & USART_SR_IDLE;
   }

   // SR then DR read sequence; this discards a received byte in DR.
   inline void clearIdleFlag() const {
      clearReadSequenceFlags();
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
      return (usart_.SR & flag) == flag;
   }

   inline uint32_t returnStatusFlag() const {
      return (usart_.SR);
   }

   // Clears the write-0 flags (TC, LBD, CTS, RXNE) in flag. Use the SR/DR
   // read sequence (getData()) for PE, FE, NE, ORE and IDLE.
   inline void clearStatusFlag(uint32_t flag) const {
      usart_.SR = ~flag;
   }

   inline void sendBreak() const {
      usart_.CR1 |= USART_CR1_SBK;
   }

   inline bool isFramingError() const {
      return usart_.SR & USART_SR_FE;
   }

   // SR then DR read sequence; this discards a received byte in DR.
   inline void clearFramingError() const {
      clearReadSequenceFlags();
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

   // USART1 runs from PCLK2 (APB2), USART2/3 and UART4/5 from PCLK1 (APB1).
   uint32_t getClockFrequency() const;

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

   // Clears PE, FE, NE, ORE, IDLE (SR/DR read sequence, discards DR) and
   // TC, LBD, CTS (write 0).
   void clearAllStatusFlags() const {
      clearReadSequenceFlags();
      usart_.SR = ~(USART_SR_TC | USART_SR_LBD | USART_SR_CTS);
   }

private:
   inline void clearReadSequenceFlags() const {
      static_cast<void>(usart_.SR);
      static_cast<void>(usart_.DR);
   }
};

} // namespace hw

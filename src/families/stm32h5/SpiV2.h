#pragma once

// SPI v2 (STM32H5, RM0481 section 58): CFG1/CFG2 configuration registers,
// a FIFO with a threshold (FTHLV), a transfer size counter (TSIZE), a master
// start bit (CSTART), separate TXDR/RXDR, and flags that are cleared in IFCR.
// The v1 block of the other families has none of these, and it has a BSY
// flag that v2 does not have. Thus this is a separate driver with an
// interface that follows the v2 hardware.
//
// Do not include this file directly; include "peripherals/Spi.h".
//
// SPI1..SPI3 of the H533 take 4 to 32 data bits and have a 16-byte FIFO.
// SPI4 takes 4 to 16 data bits and has an 8-byte FIFO.
//
// CFG1, CFG2 and CR2 (TSIZE) are write-protected while SPE = 1. All setters
// of these registers in this class require a disabled SPI; configure()
// disables it and restores SPE afterwards.
//
// Master transfers: set SPE, then CSTART (startTransfer()). With TSIZE = 0
// (the reset value) the transfer is endless: in full-duplex and transmit-only
// mode, the master clocks a frame each time the TX FIFO has data, and it
// stops the clock when the TX FIFO is empty. With TSIZE > 0 the hardware
// clears CSTART and sets EOT after TSIZE frames.

#include <cmsis_device.h>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace hw {

struct Spi {
   enum class Mode : uint32_t { Slave = 0, Master = SPI_CFG2_MASTER };

   // CFG2.COMM. In half-duplex mode, setHalfDuplexDirection() selects the
   // direction of the single data line (CR1.HDDIR).
   enum class Direction : uint32_t {
      FullDuplex = 0,
      TxOnly = SPI_CFG2_COMM_0,
      RxOnly = SPI_CFG2_COMM_1,
      HalfDuplex = SPI_CFG2_COMM,
   };

   // CFG1.DSIZE = bits - 1. Bits17 to Bits32: SPI1..SPI3 only.
   enum class DataSize : uint32_t {
      Bits4 = 3, Bits5, Bits6, Bits7, Bits8, Bits9, Bits10, Bits11, Bits12, Bits13, Bits14, Bits15, Bits16,
      Bits17, Bits18, Bits19, Bits20, Bits21, Bits22, Bits23, Bits24, Bits25, Bits26, Bits27, Bits28,
      Bits29, Bits30, Bits31, Bits32,
   };

   enum class ClockPolarity : uint32_t { Low = 0, High = SPI_CFG2_CPOL };

   enum class ClockPhase : uint32_t { OneEdge = 0, TwoEdge = SPI_CFG2_CPHA };

   // CFG1.MBR, master only. Div1 sets BPASS: SCK is the kernel clock.
   enum class BaudRatePrescaler : uint32_t {
      Div1 = SPI_CFG1_BPASS,
      Div2 = 0,
      Div4 = (1UL << SPI_CFG1_MBR_Pos),
      Div8 = (2UL << SPI_CFG1_MBR_Pos),
      Div16 = (3UL << SPI_CFG1_MBR_Pos),
      Div32 = (4UL << SPI_CFG1_MBR_Pos),
      Div64 = (5UL << SPI_CFG1_MBR_Pos),
      Div128 = (6UL << SPI_CFG1_MBR_Pos),
      Div256 = (7UL << SPI_CFG1_MBR_Pos),
   };

   enum class FirstBit : uint32_t { Msb = 0, Lsb = SPI_CFG2_LSBFRST };

   // Soft: SSM = 1. The internal slave select is SSI: inactive for a master
   // (no mode fault), active for a slave (always selected).
   // HardOutput: master drives NSS (SSOE = 1). HardInput: the NSS pin is the
   // slave select input (or the multi-master mode fault input).
   enum class NssMode : uint32_t { HardInput, HardOutput, Soft };

   SPI_TypeDef &spi_;

   explicit Spi(SPI_TypeDef *spiBase)
      : spi_(*spiBase) {}

   Spi(const Spi &) = default;
   Spi(Spi &&) = default;
   Spi &operator=(const Spi &) = delete;
   Spi &operator=(Spi &&) = delete;

   /**
    * @brief Writes CFG1 and CFG2 (FIFO threshold 1 frame, no CRC, no DMA,
    * TSIZE = 0). Disables the SPI for the change and restores SPE.
    * The NSS bits (SSM, SSI, SSOE) are written before MASTER, so a master
    * never sees its NSS input low: that would set MODF and clear MASTER.
    */
   void configure(BaudRatePrescaler prescaler,
                  Mode mode = Mode::Master,
                  ClockPolarity cpol = ClockPolarity::Low,
                  ClockPhase cpha = ClockPhase::OneEdge,
                  DataSize dataSize = DataSize::Bits8,
                  FirstBit firstBit = FirstBit::Msb,
                  NssMode nss = NssMode::Soft,
                  Direction direction = Direction::FullDuplex) const;

   inline void enable() const {
      spi_.CR1 |= SPI_CR1_SPE;
   }

   /**
    * @brief Clears SPE. This flushes both FIFOs. In master mode, call
    * waitUntilIdle() first (RM0481 section 58.4.13).
    * STM32H5 erratum (ES0621 2.17.2): a disable directly after EOT can cut
    * the last SCK pulse at a low SCK/PCLK ratio. Wait half an SCK period.
    */
   inline void disable() const {
      spi_.CR1 &= ~SPI_CR1_SPE;
   }

   inline bool isEnabled() const {
      return (spi_.CR1 & SPI_CR1_SPE) != 0;
   }

   /// Master: starts (or continues) the transfer. Needs SPE = 1.
   inline void startTransfer() const {
      spi_.CR1 |= SPI_CR1_CSTART;
   }

   inline bool isTransferStarted() const {
      return (spi_.CR1 & SPI_CR1_CSTART) != 0;
   }

   /// Master: suspends at the end of the current frame. Wait for isSuspended().
   inline void suspend() const {
      spi_.CR1 |= SPI_CR1_CSUSP;
   }

   /// Half-duplex mode only: true = transmit, false = receive.
   inline void setHalfDuplexDirection(bool transmit) const {
      spi_.CR1 = (spi_.CR1 & ~SPI_CR1_HDDIR) | (transmit ? SPI_CR1_HDDIR : 0U);
   }

   // ---- Setters of CFG1, CFG2 and CR2: only with SPE = 0 ----
   inline void setMode(Mode mode) const {
      spi_.CFG2 = (spi_.CFG2 & ~SPI_CFG2_MASTER) | static_cast<uint32_t>(mode);
   }

   inline void setDirection(Direction direction) const {
      spi_.CFG2 = (spi_.CFG2 & ~SPI_CFG2_COMM) | static_cast<uint32_t>(direction);
   }

   inline void setBaudRatePrescaler(BaudRatePrescaler prescaler) const {
      spi_.CFG1 = (spi_.CFG1 & ~(SPI_CFG1_MBR | SPI_CFG1_BPASS)) | static_cast<uint32_t>(prescaler);
   }

   inline void setClockPolarity(ClockPolarity cpol) const {
      spi_.CFG2 = (spi_.CFG2 & ~SPI_CFG2_CPOL) | static_cast<uint32_t>(cpol);
   }

   inline void setClockPhase(ClockPhase cpha) const {
      spi_.CFG2 = (spi_.CFG2 & ~SPI_CFG2_CPHA) | static_cast<uint32_t>(cpha);
   }

   inline void setDataSize(DataSize dataSize) const {
      spi_.CFG1 = (spi_.CFG1 & ~SPI_CFG1_DSIZE) | (static_cast<uint32_t>(dataSize) << SPI_CFG1_DSIZE_Pos);
   }

   inline DataSize getDataSize() const {
      return static_cast<DataSize>((spi_.CFG1 & SPI_CFG1_DSIZE) >> SPI_CFG1_DSIZE_Pos);
   }

   inline void setFirstBit(FirstBit firstBit) const {
      spi_.CFG2 = (spi_.CFG2 & ~SPI_CFG2_LSBFRST) | static_cast<uint32_t>(firstBit);
   }

   /// Uses CFG2.MASTER for the SSI level of NssMode::Soft: call setMode() first.
   void setNssMode(NssMode nss) const;

   /// Internal: CFG2 and CR1 for `nss`. `master` selects the SSI level of NssMode::Soft.
   void applyNssMode(NssMode nss, bool master) const;

   /// RXP/TXP and the DMA requests come per `frames` data frames (1 to 16).
   inline void setFifoThreshold(uint8_t frames) const {
      const uint32_t fthlv = (frames == 0 ? 0U : (frames > 16 ? 15U : frames - 1U));
      spi_.CFG1 = (spi_.CFG1 & ~SPI_CFG1_FTHLV) | (fthlv << SPI_CFG1_FTHLV_Pos);
   }

   /// Number of frames of the next transfer; 0 = endless. SPI4: up to 1023 with CRC.
   inline void setTransferSize(uint16_t frames) const {
      spi_.CR2 = frames;
   }

   /// Master only (RM0481: must not be set in slave mode). Keeps SCK, MOSI and
   /// NSS at their idle levels while SPE = 0, so a reconfiguration gives no glitch.
   inline void enableAlternateFunctionControl() const {
      spi_.CFG2 |= SPI_CFG2_AFCNTR;
   }

   inline void disableAlternateFunctionControl() const {
      spi_.CFG2 &= ~SPI_CFG2_AFCNTR;
   }

   inline void enableTxDma() const {
      spi_.CFG1 |= SPI_CFG1_TXDMAEN;
   }
   inline void disableTxDma() const {
      spi_.CFG1 &= ~SPI_CFG1_TXDMAEN;
   }
   inline void enableRxDma() const {
      spi_.CFG1 |= SPI_CFG1_RXDMAEN;
   }
   inline void disableRxDma() const {
      spi_.CFG1 &= ~SPI_CFG1_RXDMAEN;
   }

   // ---- Data ----
   /**
    * @brief Writes one frame to TXDR. The access width follows DSIZE (8, 16
    * or 32 bits): a wider access would put more than one frame into the FIFO.
    * The cast through uintptr_t is an MMIO access, not a type pun.
    */
   inline void write(uint32_t data) const {
      const uint32_t dsize = static_cast<uint32_t>(getDataSize());
      if(dsize < 8) {
         *reinterpret_cast<__IO uint8_t *>(&spi_.TXDR) = static_cast<uint8_t>(data);
      } else if(dsize < 16) {
         *reinterpret_cast<__IO uint16_t *>(reinterpret_cast<uintptr_t>(&spi_) + offsetof(SPI_TypeDef, TXDR)) =
            static_cast<uint16_t>(data);
      } else {
         spi_.TXDR = data;
      }
   }

   /// Reads one frame from RXDR, with the access width of write().
   inline uint32_t read() const {
      const uint32_t dsize = static_cast<uint32_t>(getDataSize());
      if(dsize < 8) {
         return *reinterpret_cast<__IO uint8_t *>(&spi_.RXDR);
      }
      if(dsize < 16) {
         return *reinterpret_cast<__IO uint16_t *>(reinterpret_cast<uintptr_t>(&spi_) + offsetof(SPI_TypeDef, RXDR));
      }
      return spi_.RXDR;
   }

   /// Bound of the poll loops; see the SPI v1 driver for the reasoning.
   static constexpr uint32_t defaultTimeoutLoops = 100'000u;

   /**
    * @brief Blocking full-duplex transfer of one frame, master mode, FIFO
    * threshold 1. Sets CSTART if it is not set.
    * @return The received frame, or std::nullopt if TXP or RXP did not come
    * in `timeoutLoops` polls. After a timeout the RX FIFO is drained and OVR
    * is cleared, so the next call starts from a known state.
    */
   [[nodiscard]] std::optional<uint32_t> transfer(uint32_t data, uint32_t timeoutLoops = defaultTimeoutLoops) const;

   /**
    * @brief Waits for TXC: the TX FIFO is empty and the last frame is on the
    * bus. With TSIZE > 0, TXC is a copy of EOT.
    * @return false if TXC did not come in `timeoutLoops` polls.
    */
   [[nodiscard]] bool waitUntilIdle(uint32_t timeoutLoops = defaultTimeoutLoops) const;

   // ---- Status (SR) ----
   inline bool isTxPacketSpaceAvailable() const {
      return (spi_.SR & SPI_SR_TXP) != 0;
   }
   inline bool isRxPacketAvailable() const {
      return (spi_.SR & SPI_SR_RXP) != 0;
   }
   inline bool isEndOfTransfer() const {
      return (spi_.SR & SPI_SR_EOT) != 0;
   }
   inline bool isTxTransferFilled() const {
      return (spi_.SR & SPI_SR_TXTF) != 0;
   }
   inline bool isTxComplete() const {
      return (spi_.SR & SPI_SR_TXC) != 0;
   }
   inline bool isSuspended() const {
      return (spi_.SR & SPI_SR_SUSP) != 0;
   }
   inline bool isOverrun() const {
      return (spi_.SR & SPI_SR_OVR) != 0;
   }
   inline bool isUnderrun() const {
      return (spi_.SR & SPI_SR_UDR) != 0;
   }
   inline bool isModeFault() const {
      return (spi_.SR & SPI_SR_MODF) != 0;
   }
   /// Frames left in the current transfer (TSIZE > 0).
   inline uint16_t remainingFrames() const {
      return static_cast<uint16_t>((spi_.SR & SPI_SR_CTSIZE) >> SPI_SR_CTSIZE_Pos);
   }

   // ---- Flag clear (IFCR) ----
   inline void clearEndOfTransfer() const {
      spi_.IFCR = SPI_IFCR_EOTC;
   }
   inline void clearTxTransferFilled() const {
      spi_.IFCR = SPI_IFCR_TXTFC;
   }
   inline void clearSuspend() const {
      spi_.IFCR = SPI_IFCR_SUSPC;
   }
   inline void clearOverrun() const {
      spi_.IFCR = SPI_IFCR_OVRC;
   }
   inline void clearUnderrun() const {
      spi_.IFCR = SPI_IFCR_UDRC;
   }
   /// A mode fault clears SPE and blocks it until MODF is cleared.
   inline void clearModeFault() const {
      spi_.IFCR = SPI_IFCR_MODFC;
   }
   inline void clearAllFlags() const {
      spi_.IFCR = SPI_IFCR_EOTC | SPI_IFCR_TXTFC | SPI_IFCR_UDRC | SPI_IFCR_OVRC | SPI_IFCR_CRCEC | SPI_IFCR_TIFREC |
                  SPI_IFCR_MODFC | SPI_IFCR_SUSPC;
   }

   // ---- Interrupts (IER) ----
   inline void enableRxPacketInterrupt() const {
      spi_.IER |= SPI_IER_RXPIE;
   }
   inline void disableRxPacketInterrupt() const {
      spi_.IER &= ~SPI_IER_RXPIE;
   }
   inline void enableTxPacketInterrupt() const {
      spi_.IER |= SPI_IER_TXPIE;
   }
   inline void disableTxPacketInterrupt() const {
      spi_.IER &= ~SPI_IER_TXPIE;
   }
   /// EOT, SUSP and TXC.
   inline void enableEndOfTransferInterrupt() const {
      spi_.IER |= SPI_IER_EOTIE;
   }
   inline void disableEndOfTransferInterrupt() const {
      spi_.IER &= ~SPI_IER_EOTIE;
   }
   /// OVR, UDR and MODF.
   inline void enableErrorInterrupt() const {
      spi_.IER |= SPI_IER_OVRIE | SPI_IER_UDRIE | SPI_IER_MODFIE;
   }
   inline void disableErrorInterrupt() const {
      spi_.IER &= ~(SPI_IER_OVRIE | SPI_IER_UDRIE | SPI_IER_MODFIE);
   }

   /**
    * @brief The kernel clock (spi_ker_ck) in Hz, before the baud prescaler.
    * Decodes RCC_CCIPR3.SPIxSEL for SPI1..SPI4. Returns 0 for a source that
    * system-stm32 does not start (PLL2, PLL3, AUDIOCLK, per_ck).
    */
   uint32_t getClockFrequency() const;
};

} // namespace hw

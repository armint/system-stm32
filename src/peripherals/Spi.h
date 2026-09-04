#pragma once

#include <cmsis_device.h>
#include <cstdint>

/**
 * @brief SPI Peripheral access class.
 *
 * This class provides a thin wrapper around the STM32 SPI peripheral registers.
 * It follows the pattern used for other peripheral classes in this project.
 */
namespace hw {

struct Spi {
   enum class Mode : uint32_t { Slave = 0, Master = SPI_CR1_MSTR };

   enum class Direction : uint32_t {
      TwoLines = 0,
      TwoLines_RxOnly = SPI_CR1_RXONLY,
      OneLine_Rx = SPI_CR1_BIDIMODE,
      OneLine_Tx = SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE
   };

   enum class DataSize : uint32_t {
#if defined(SPI_CR1_DFF)
      Bits8 = 0,
      Bits16 = SPI_CR1_DFF
#elif defined(SPI_CR2_DS)
      Bits4 = (0x3UL << SPI_CR2_DS_Pos),
      Bits5 = (0x4UL << SPI_CR2_DS_Pos),
      Bits6 = (0x5UL << SPI_CR2_DS_Pos),
      Bits7 = (0x6UL << SPI_CR2_DS_Pos),
      Bits8 = (0x7UL << SPI_CR2_DS_Pos),
      Bits9 = (0x8UL << SPI_CR2_DS_Pos),
      Bits10 = (0x9UL << SPI_CR2_DS_Pos),
      Bits11 = (0xAUL << SPI_CR2_DS_Pos),
      Bits12 = (0xBUL << SPI_CR2_DS_Pos),
      Bits13 = (0xCUL << SPI_CR2_DS_Pos),
      Bits14 = (0xDUL << SPI_CR2_DS_Pos),
      Bits15 = (0xEUL << SPI_CR2_DS_Pos),
      Bits16 = (0xFUL << SPI_CR2_DS_Pos)
#else
      Bits8 = 0,
      Bits16 = (1 << 11) // Fallback for standard DFF position
#endif
   };

   enum class ClockPolarity : uint32_t { Low = 0, High = SPI_CR1_CPOL };

   enum class ClockPhase : uint32_t { OneEdge = 0, TwoEdge = SPI_CR1_CPHA };

   enum class BaudRatePrescaler : uint32_t {
      Div2 = 0,
      Div4 = SPI_CR1_BR_0,
      Div8 = SPI_CR1_BR_1,
      Div16 = (SPI_CR1_BR_1 | SPI_CR1_BR_0),
      Div32 = SPI_CR1_BR_2,
      Div64 = (SPI_CR1_BR_2 | SPI_CR1_BR_0),
      Div128 = (SPI_CR1_BR_2 | SPI_CR1_BR_1),
      Div256 = (SPI_CR1_BR_2 | SPI_CR1_BR_1 | SPI_CR1_BR_0)
   };

   enum class FirstBit : uint32_t { Msb = 0, Lsb = SPI_CR1_LSBFIRST };

   enum class NssMode : uint32_t {
      HardInput = 0,
      HardOutput = 0x10000, // Custom internal flag
      Soft = SPI_CR1_SSM
   };

   SPI_TypeDef &spi_;

   explicit Spi(SPI_TypeDef *spiBase)
      : spi_(*spiBase) {}

   Spi(const Spi &) = default;
   Spi(Spi &&) = default;
   Spi &operator=(const Spi &) = delete;
   Spi &operator=(Spi &&) = delete;

   /**
    * @brief Configures the SPI peripheral.
    * @note SPI should be disabled before calling this, or it will be disabled automatically during configuration.
    */
   void configure(BaudRatePrescaler prescaler,
                  Mode mode = Mode::Master,
                  ClockPolarity cpol = ClockPolarity::Low,
                  ClockPhase cpha = ClockPhase::OneEdge,
                  DataSize dataSize = DataSize::Bits8,
                  FirstBit firstBit = FirstBit::Msb,
                  NssMode nss = NssMode::Soft,
                  Direction direction = Direction::TwoLines) const;

   inline void enable() const {
      spi_.CR1 |= SPI_CR1_SPE;
   }

   inline void disable() const {
      spi_.CR1 &= ~SPI_CR1_SPE;
   }

   inline bool isEnabled() const {
      return (spi_.CR1 & SPI_CR1_SPE) != 0;
   }

   inline void setMode(Mode mode) const {
      spi_.CR1 = (spi_.CR1 & ~SPI_CR1_MSTR) | static_cast<uint32_t>(mode);
   }

   inline void setDirection(Direction direction) const {
      spi_.CR1 = (spi_.CR1 & ~(SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE | SPI_CR1_RXONLY)) | static_cast<uint32_t>(direction);
   }

   inline void setBaudRatePrescaler(BaudRatePrescaler prescaler) const {
      spi_.CR1 = (spi_.CR1 & ~SPI_CR1_BR) | static_cast<uint32_t>(prescaler);
   }

   inline void setClockPolarity(ClockPolarity cpol) const {
      spi_.CR1 = (spi_.CR1 & ~SPI_CR1_CPOL) | static_cast<uint32_t>(cpol);
   }

   inline void setClockPhase(ClockPhase cpha) const {
      spi_.CR1 = (spi_.CR1 & ~SPI_CR1_CPHA) | static_cast<uint32_t>(cpha);
   }

   void setDataSize(DataSize dataSize) const;

   inline void setFirstBit(FirstBit firstBit) const {
      spi_.CR1 = (spi_.CR1 & ~SPI_CR1_LSBFIRST) | static_cast<uint32_t>(firstBit);
   }

   void setNssMode(NssMode nss) const;

   /**
    * @brief Checks if the SPI is configured for 16-bit data size.
    * @return true if 16-bit, false if 8-bit (or less).
    */
   inline bool hasWordAccess() const {
#if defined(SPI_CR1_DFF)
      return (spi_.CR1 & SPI_CR1_DFF) != 0;
#elif defined(SPI_CR2_DS)
      return (spi_.CR2 & SPI_CR2_DS) > (0x7UL << SPI_CR2_DS_Pos);
#else
      return false;
#endif
   }

   /**
    * @brief Transmits a single data word.
    * @param data Data to transmit (8 or 16 bits).
    */
   inline void write(uint16_t data) const {
#if defined(SPI_CR2_DS)
      // On some families, we must use 8-bit access to DR for 8-bit data to work with FIFO
      if((spi_.CR2 & SPI_CR2_DS) <= (0x7UL << SPI_CR2_DS_Pos)) {
         *(__IO uint8_t *)&spi_.DR = static_cast<uint8_t>(data);
      } else {
         spi_.DR = data;
      }
#elif defined(SPI_CR1_DFF)
      if(!(spi_.CR1 & SPI_CR1_DFF)) {
         *(__IO uint8_t *)&spi_.DR = static_cast<uint8_t>(data);
      } else {
         spi_.DR = data;
      }
#else
      spi_.DR = data;
#endif
   }

   /**
    * @brief Receives a single data word.
    * @return Received data (8 or 16 bits).
    */
   inline uint16_t read() const {
#if defined(SPI_CR2_DS)
      if((spi_.CR2 & SPI_CR2_DS) <= (0x7UL << SPI_CR2_DS_Pos)) {
         return *(__IO uint8_t *)&spi_.DR;
      } else {
         return static_cast<uint16_t>(spi_.DR);
      }
#elif defined(SPI_CR1_DFF)
      if(!(spi_.CR1 & SPI_CR1_DFF)) {
         return *(__IO uint8_t *)&spi_.DR;
      } else {
         return static_cast<uint16_t>(spi_.DR);
      }
#else
      return static_cast<uint16_t>(spi_.DR);
#endif
   }

   /**
    * @brief Blocking full duplex transfer of a single data word.
    * @param data Data to transmit (8 or 16 bits, according to the configured data size).
    * @return The data word received while transmitting.
    * @note Busy waits; only meaningful while the peripheral is enabled and in master mode.
    */
   inline uint16_t transfer(uint16_t data) const {
      while(!isTxEmpty()) {
      }
      write(data);
      while(!isRxNotEmpty()) {
      }
      return read();
   }

   /**
    * @brief Busy waits until the peripheral has finished shifting out all data.
    */
   inline void waitUntilIdle() const {
      while(isBusy()) {
      }
   }

   inline bool isTxEmpty() const {
      return (spi_.SR & SPI_SR_TXE) != 0;
   }

   inline bool isRxNotEmpty() const {
      return (spi_.SR & SPI_SR_RXNE) != 0;
   }

   inline bool isBusy() const {
      return (spi_.SR & SPI_SR_BSY) != 0;
   }

   inline bool isOverrun() const {
      return (spi_.SR & SPI_SR_OVR) != 0;
   }

   inline bool isModeFault() const {
      return (spi_.SR & SPI_SR_MODF) != 0;
   }

   /**
    * @brief Clears the overrun flag.
    */
   inline void clearOverrun() const {
      uint32_t tmp = spi_.DR;
      tmp = spi_.SR;
      (void)tmp;
   }

   /**
    * @brief Clears the mode fault flag.
    */
   inline void clearModeFault() const {
      uint32_t tmp = spi_.SR;
      spi_.CR1 &= ~SPI_CR1_SPE; // Must disable and re-enable to clear MODF on some chips
      (void)tmp;
      spi_.CR1 |= SPI_CR1_SPE;
   }

   // DMA Control
   inline void enableTxDma() const {
      spi_.CR2 |= SPI_CR2_TXDMAEN;
   }
   inline void disableTxDma() const {
      spi_.CR2 &= ~SPI_CR2_TXDMAEN;
   }
   inline void enableRxDma() const {
      spi_.CR2 |= SPI_CR2_RXDMAEN;
   }
   inline void disableRxDma() const {
      spi_.CR2 &= ~SPI_CR2_RXDMAEN;
   }

   // Interrupt Control
   inline void enableRxNotEmptyInterrupt() const {
      spi_.CR2 |= SPI_CR2_RXNEIE;
   }
   inline void disableRxNotEmptyInterrupt() const {
      spi_.CR2 &= ~SPI_CR2_RXNEIE;
   }
   inline void enableTxEmptyInterrupt() const {
      spi_.CR2 |= SPI_CR2_TXEIE;
   }
   inline void disableTxEmptyInterrupt() const {
      spi_.CR2 &= ~SPI_CR2_TXEIE;
   }
   inline void enableErrorInterrupt() const {
      spi_.CR2 |= SPI_CR2_ERRIE;
   }
   inline void disableErrorInterrupt() const {
      spi_.CR2 &= ~SPI_CR2_ERRIE;
   }

   /**
    * @brief Returns the clock frequency of the APB bus the SPI is connected to.
    */
   uint32_t getClockFrequency() const;
};

} // namespace hw

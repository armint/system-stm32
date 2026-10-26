#include "Spi.h"
#include "Rcc.h"

namespace hw {

void Spi::configure(BaudRatePrescaler prescaler,
                    Mode mode,
                    ClockPolarity cpol,
                    ClockPhase cpha,
                    DataSize dataSize,
                    FirstBit firstBit,
                    NssMode nss,
                    Direction direction) const {
   bool wasEnabled = isEnabled();
   if(wasEnabled)
      disable();

   setBaudRatePrescaler(prescaler);
   setMode(mode);
   setDirection(direction);
   setClockPolarity(cpol);
   setClockPhase(cpha);
   setDataSize(dataSize);
   setFirstBit(firstBit);
   setNssMode(nss);

   if(wasEnabled)
      enable();
}

void Spi::setDataSize(DataSize dataSize) const {
#if defined(SPI_CR1_DFF)
   spi_.CR1 = (spi_.CR1 & ~SPI_CR1_DFF) | static_cast<uint32_t>(dataSize);
#elif defined(SPI_CR2_DS)
   uint32_t cr2 = (spi_.CR2 & ~SPI_CR2_DS) | static_cast<uint32_t>(dataSize);
#if defined(SPI_CR2_FRXTH)
   // On peripherals with a receive FIFO (DS-based data size, e.g. STM32WL) the
   // RXNE event is raised at the FIFO threshold, not per data frame. For data
   // sizes up to 8 bits the threshold must be a quarter of the FIFO (one byte),
   // otherwise RXNE would only be set after two frames and byte-wise transfers
   // would stall. Above 8 bits the half-FIFO (16-bit) threshold applies.
   if((cr2 & SPI_CR2_DS) <= (0x7UL << SPI_CR2_DS_Pos)) {
      cr2 |= SPI_CR2_FRXTH;
   } else {
      cr2 &= ~SPI_CR2_FRXTH;
   }
#endif
   spi_.CR2 = cr2;
#endif
}

void Spi::setNssMode(NssMode nss) const {
   if(nss == NssMode::Soft) {
      spi_.CR1 |= SPI_CR1_SSM | SPI_CR1_SSI;
      spi_.CR2 &= ~SPI_CR2_SSOE;
   } else if(nss == NssMode::HardOutput) {
      spi_.CR1 &= ~SPI_CR1_SSM;
      spi_.CR2 |= SPI_CR2_SSOE;
   } else {
      spi_.CR1 &= ~SPI_CR1_SSM;
      spi_.CR2 &= ~SPI_CR2_SSOE;
   }
}

uint32_t Spi::getClockFrequency() const {
// SPI1 sits on APB2 wherever a separate APB2 prescaler exists (this includes
// STM32WL, where SPI1 is on APB2 and SPI2/S2 on APB1).
#if defined(RCC_CFGR_PPRE2) && defined(SPI1_BASE)
   if(reinterpret_cast<uintptr_t>(&spi_) == SPI1_BASE) {
      return Rcc::getAPB2Clock();
   }
#endif
   return Rcc::getAPBClock();
}

} // namespace hw

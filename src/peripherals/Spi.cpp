#include "Spi.h"
#include "Rcc.h"

namespace hw {

void Spi::configure(BaudRatePrescaler prescaler, Mode mode, ClockPolarity cpol, ClockPhase cpha, DataSize dataSize,
                    FirstBit firstBit, NssMode nss, Direction direction) const {
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
   spi_.CR2 = (spi_.CR2 & ~SPI_CR2_DS) | static_cast<uint32_t>(dataSize);
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
#if defined(RCC_CFGR_PPRE2)
   if(reinterpret_cast<uintptr_t>(&spi_) == SPI1_BASE) {
      return Rcc::getAPB2Clock();
   }
#endif
   return Rcc::getAPBClock();
}

} // namespace hw

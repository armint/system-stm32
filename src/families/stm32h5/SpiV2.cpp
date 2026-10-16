#include "peripherals/Spi.h"
#include "peripherals/Rcc.h"

namespace hw {

void Spi::configure(BaudRatePrescaler prescaler, Mode mode, ClockPolarity cpol, ClockPhase cpha, DataSize dataSize,
                    FirstBit firstBit, NssMode nss, Direction direction) const {
   const bool wasEnabled = isEnabled();
   disable();

   // FTHLV = 0 (1 frame), no CRC, no DMA. The reset value of CFG1 has
   // CRCSIZE = 7: keep it, CRCEN stays 0.
   spi_.CFG1 = (spi_.CFG1 & SPI_CFG1_CRCSIZE) | static_cast<uint32_t>(prescaler) |
               (static_cast<uint32_t>(dataSize) << SPI_CFG1_DSIZE_Pos);
   spi_.CFG2 = static_cast<uint32_t>(mode) | static_cast<uint32_t>(cpol) | static_cast<uint32_t>(cpha) |
               static_cast<uint32_t>(firstBit) | static_cast<uint32_t>(direction);
   spi_.CR2 = 0; // TSIZE = 0: endless transfer
   setNssMode(nss);

   if(wasEnabled) {
      enable();
   }
}

void Spi::setNssMode(NssMode nss) const {
   uint32_t cfg2 = spi_.CFG2 & ~(SPI_CFG2_SSM | SPI_CFG2_SSOE);
   const bool master = (cfg2 & SPI_CFG2_MASTER) != 0;
   if(nss == NssMode::Soft) {
      cfg2 |= SPI_CFG2_SSM;
      // SSI is the internal slave select (active low, SSIOP = 0): high for a
      // master (no mode fault), low for a slave (selected).
      if(master) {
         spi_.CR1 |= SPI_CR1_SSI;
      } else {
         spi_.CR1 &= ~SPI_CR1_SSI;
      }
   } else if(nss == NssMode::HardOutput) {
      cfg2 |= SPI_CFG2_SSOE;
   }
   spi_.CFG2 = cfg2;
}

std::optional<uint32_t> Spi::transfer(uint32_t data, uint32_t timeoutLoops) const {
   const auto fail = [this]() {
      while(isRxPacketAvailable()) {
         (void)read();
      }
      clearOverrun();
      return std::nullopt;
   };
   if(!isTransferStarted()) {
      startTransfer();
   }
   uint32_t remaining = timeoutLoops;
   while(!isTxPacketSpaceAvailable()) {
      if(remaining == 0u) {
         return fail();
      }
      --remaining;
   }
   write(data);
   remaining = timeoutLoops;
   while(!isRxPacketAvailable()) {
      if(remaining == 0u) {
         return fail();
      }
      --remaining;
   }
   return read();
}

bool Spi::waitUntilIdle(uint32_t timeoutLoops) const {
   while(!isTxComplete()) {
      if(timeoutLoops == 0u) {
         return false;
      }
      --timeoutLoops;
   }
   return true;
}

uint32_t Spi::getClockFrequency() const {
   const auto base = reinterpret_cast<uintptr_t>(&spi_);
   uint32_t sel;
   if(base == SPI1_BASE) {
      sel = (RCC->CCIPR3 & RCC_CCIPR3_SPI1SEL) >> RCC_CCIPR3_SPI1SEL_Pos;
   } else if(base == SPI2_BASE) {
      sel = (RCC->CCIPR3 & RCC_CCIPR3_SPI2SEL) >> RCC_CCIPR3_SPI2SEL_Pos;
   } else if(base == SPI3_BASE) {
      sel = (RCC->CCIPR3 & RCC_CCIPR3_SPI3SEL) >> RCC_CCIPR3_SPI3SEL_Pos;
   } else if(base == SPI4_BASE) {
      // SPI4SEL: 000 pclk2, 001 pll2_q, 010 pll3_q, 011 hsi_ker, 100 csi_ker, 101 hse.
      switch((RCC->CCIPR3 & RCC_CCIPR3_SPI4SEL) >> RCC_CCIPR3_SPI4SEL_Pos) {
      case 0:
         return Rcc::getAPB2Clock();
      case 3:
         return Rcc::getHSIClock();
      case 4:
         return Rcc::CSIHz;
      case 5:
         return HSE_VALUE;
      default:
         return 0; // PLL2, PLL3: not started by system-stm32
      }
   } else {
      return 0;
   }
   // 000: pll1_q_ck. 001 pll2_p, 010 pll3_p, 011 AUDIOCLK, 100 per_ck: not started here.
   return sel == 0 ? Rcc::getPLL1QClock() : 0;
}

} // namespace hw

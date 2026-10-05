#if defined(STM32F1)
#include "Adc.h"

#include <delaybusywait.h>

extern uint32_t SystemCoreClock;

static_assert(Adc::supplyFromVrefint(1489) == 3300, "VDDA from Vrefint");
static_assert(Adc::supplyFromVrefint(0) == 0, "VDDA from Vrefint, zero reading");
static_assert(Adc::toMillivolts(4095, 3300) == 3300, "full scale");
static_assert(Adc::toMillivolts(2048, 3300) == 1650, "mid scale");

namespace {
// Bound for the poll loops. A conversion at 239.5 + 12.5 cycles of 14 MHz takes
// 18 us, so this is far more than needed, also at a low ADCCLK.
constexpr uint32_t kPollLimit = 1000000;

// AHB prescaler shift for HPRE field values 8..15, and APB prescaler shift for PPRE 4..7.
constexpr uint8_t kAhbShift[8] = {1, 2, 3, 4, 6, 7, 8, 9};
constexpr uint8_t kApbShift[4] = {1, 2, 3, 4};

uint32_t pclk2Hz() {
   const uint32_t cfgr = RCC->CFGR;
   uint32_t hclk = SystemCoreClock;
   const uint32_t hpre = (cfgr & RCC_CFGR_HPRE) >> RCC_CFGR_HPRE_Pos;
   if(hpre >= 8)
      hclk >>= kAhbShift[hpre - 8];
   const uint32_t ppre2 = (cfgr & RCC_CFGR_PPRE2) >> RCC_CFGR_PPRE2_Pos;
   if(ppre2 >= 4)
      hclk >>= kApbShift[ppre2 - 4];
   return hclk;
}
} // namespace

uint32_t Adc::clockHz() {
   const uint32_t adcpre = (RCC->CFGR & RCC_CFGR_ADCPRE) >> RCC_CFGR_ADCPRE_Pos;
   return pclk2Hz() / (2 * (adcpre + 1));
}

uint32_t Adc::configureClock() {
   const uint32_t pclk2 = pclk2Hz();
   uint32_t adcpre = 0; // ADCPRE field: 0 -> /2, 1 -> /4, 2 -> /6, 3 -> /8
   while(adcpre < 3 && pclk2 / (2 * (adcpre + 1)) > maxClockHz)
      adcpre++;
   RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_ADCPRE) | (adcpre << RCC_CFGR_ADCPRE_Pos);
   return pclk2 / (2 * (adcpre + 1));
}

void Adc::enable() const {
   // Writing ADON while it is already 1 starts a conversion, so check first.
   if(_adc.CR2 & ADC_CR2_ADON)
      return;
   _adc.CR2 |= ADC_CR2_ADON;
   delayuS(2); // tSTAB is 1 us, and the calibration needs 2 ADCCLK cycles after power-up
}

void Adc::disable() const {
   _adc.CR2 &= ~ADC_CR2_ADON;
}

void Adc::calibrate() const {
   _adc.CR2 |= ADC_CR2_RSTCAL;
   for(uint32_t i = 0; (_adc.CR2 & ADC_CR2_RSTCAL) && i < kPollLimit; i++) {
   }
   _adc.CR2 |= ADC_CR2_CAL;
   for(uint32_t i = 0; (_adc.CR2 & ADC_CR2_CAL) && i < kPollLimit; i++) {
   }
}

void Adc::setSamplingTime(Channel channel, SamplingCycles cycles) const {
   const uint32_t value = static_cast<uint32_t>(cycles);
   if(channel < 10) {
      const uint32_t shift = channel * 3;
      _adc.SMPR2 = (_adc.SMPR2 & ~(0x7u << shift)) | (value << shift);
   } else {
      const uint32_t shift = (channel - 10) * 3;
      _adc.SMPR1 = (_adc.SMPR1 & ~(0x7u << shift)) | (value << shift);
   }
}

bool Adc::convert(Channel channel, uint16_t &value) const {
   // Regular sequence of length 1 (L = 0) with the channel as the first conversion.
   _adc.SQR1 = 0;
   _adc.SQR3 = channel;
   // Software trigger. Single conversion, right aligned.
   _adc.CR2 = (_adc.CR2 & ~(ADC_CR2_EXTSEL | ADC_CR2_CONT | ADC_CR2_ALIGN | ADC_CR2_DMA)) | ADC_CR2_EXTSEL | ADC_CR2_EXTTRIG;
   _adc.SR = 0;
   _adc.CR2 |= ADC_CR2_SWSTART;
   uint32_t i = 0;
   while(!(_adc.SR & ADC_SR_EOC)) {
      if(++i > kPollLimit)
         return false;
   }
   value = static_cast<uint16_t>(_adc.DR); // reading DR clears EOC
   return true;
}

bool Adc::readSupplyMillivolts(uint32_t &vddaMv) const {
   enableInternalChannels();
   delayuS(10); // tSTART of the internal reference
   setSamplingTime(Vrefint, SamplingCycles::Cycles239_5);
   uint16_t raw;
   if(!convert(Vrefint, raw) || raw == 0)
      return false;
   vddaMv = supplyFromVrefint(raw);
   return true;
}
#endif

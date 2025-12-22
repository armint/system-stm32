#include "Adc.h"
#if defined(STM32L0)

void Adc::calibrate() const {
   if(adc.CR & ADC_CR_ADEN) {
      disable();
   }
   adc.CR |= ADC_CR_ADCAL;
   while(!(adc.ISR & ADC_ISR_EOCAL))
      ;
   adc.ISR |= ADC_ISR_EOCAL;
}

void Adc::enable() const {
   if(adc.CR & ADC_CR_ADEN)
      return;

   adc.CR |= ADC_CR_ADEN;
   while(!(adc.ISR & ADC_ISR_ADRDY))
      ;
}

void Adc::disable() const {
   if(!(adc.CR & ADC_CR_ADEN))
      return;

   if(adc.CR & ADC_CR_ADSTART) {
      adc.CR |= ADC_CR_ADSTP;
      while(adc.CR & ADC_CR_ADSTP)
         ;
   }

   adc.CR |= ADC_CR_ADDIS;
   while(adc.CR & ADC_CR_ADEN)
      ;
}

void Adc::setClockMode(ClockMode mode) const {
   bool wasEnabled = (adc.CR & ADC_CR_ADEN);
   if(wasEnabled)
      disable();

   uint32_t cfgr2 = adc.CFGR2;
   cfgr2 &= ~ADC_CFGR2_CKMODE;
   cfgr2 |= static_cast<uint32_t>(mode);
   adc.CFGR2 = cfgr2;

   if(wasEnabled)
      enable();
}

void Adc::setResolution(Resolution resolution) const {
   bool wasEnabled = (adc.CR & ADC_CR_ADEN);
   if(wasEnabled)
      disable();

   uint32_t cfgr1 = adc.CFGR1;
   cfgr1 &= ~ADC_CFGR1_RES;
   cfgr1 |= static_cast<uint32_t>(resolution);
   adc.CFGR1 = cfgr1;

   if(wasEnabled)
      enable();
}

void Adc::setSamplingTime(SamplingTime cycles) const {
   bool wasEnabled = (adc.CR & ADC_CR_ADEN);
   if(wasEnabled)
      disable();

   uint32_t smpr = adc.SMPR;
   smpr &= ~ADC_SMPR_SMP;
   smpr |= static_cast<uint32_t>(cycles);
   adc.SMPR = smpr;

   if(wasEnabled)
      enable();
}

void Adc::setSequence(std::initializer_list<Channel> channels) const {
   uint32_t mask = 0;
   for(auto ch : channels) {
      mask |= (1UL << static_cast<uint32_t>(ch));
   }
   setSequence(mask);
}

void Adc::setSequence(uint32_t channelMask) const {
   // Note: CHSELR can be written even if ADEN=1,
   // BUT only if ADSTART=0 (no conversion running).
   // It's good practice to stop conversion if running.
   if(adc.CR & ADC_CR_ADSTART) {
      stopConversion();
      while(adc.CR & ADC_CR_ADSTP)
         ; // Wait for stop
   }

   adc.CHSELR = channelMask;
}
void Adc::enableOversampling(OversamplingRatio ratio, OversamplingShift shift) const {
   // Configuration bits can only be modified when ADEN=0
   bool wasEnabled = (adc.CR & ADC_CR_ADEN);
   if(wasEnabled) {
      disable();
   }

   uint32_t cfgr2 = adc.CFGR2;

   // 1. Clear existing OVSR and OVSS bits
   cfgr2 &= ~(ADC_CFGR2_OVSR | ADC_CFGR2_OVSS);

   // 2. Set new Ratio and Shift
   cfgr2 |= static_cast<uint32_t>(ratio);
   cfgr2 |= static_cast<uint32_t>(shift);

   // 3. Enable Oversampler (OVSE)
   cfgr2 |= ADC_CFGR2_OVSE;

   adc.CFGR2 = cfgr2;

   if(wasEnabled) {
      enable();
   }
}

void Adc::disableOversampling() const {
   bool wasEnabled = (adc.CR & ADC_CR_ADEN);
   if(wasEnabled)
      disable();

   // Clear the OVSE bit
   adc.CFGR2 &= ~ADC_CFGR2_OVSE;

   if(wasEnabled)
      enable();
}


// Static helpers
void Adc::enableVRefInt() {
   ADC1_COMMON->CCR |= ADC_CCR_VREFEN;
}

void Adc::disableVRefInt() {
   ADC1_COMMON->CCR &= ~ADC_CCR_VREFEN;
}

void Adc::enableTempSensor() {
   ADC1_COMMON->CCR |= ADC_CCR_TSEN;
}

void Adc::disableTempSensor() {
   ADC1_COMMON->CCR &= ~ADC_CCR_TSEN;
}
#endif
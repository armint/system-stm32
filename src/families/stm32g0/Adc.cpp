#if defined(STM32G0)
#include <cassert>

#include "Adc.h"

void Adc::enable() const {
   // Enable the ADC
   _adc.CR |= ADC_CR_ADEN;
   _adc.ISR |= ADC_ISR_ADRDY;
   if(!(_adc.CFGR1 & ADC_CFGR1_AUTOFF)) {
      // If auto off is disabled, wait until ADC ready
      while((_adc.ISR & ADC_ISR_ADRDY) == 0) {
      }
   }
}

void Adc::disable() const {
   // Ensure that no conversion is ongoing
   if((_adc.CR & ADC_CR_ADSTART) != 0) {
      // Stop any ongoing conversion
      _adc.CR |= ADC_CR_ADSTP;
   }
   // Wait until the conversion is stopped
   while((_adc.CR & ADC_CR_ADSTP) != 0) {
   }
   // Disable the ADC
   _adc.CR |= ADC_CR_ADDIS;
   // Wait until the enabled flag is cleared
   while((_adc.CR & ADC_CR_ADEN) != 0) {
   }
}

void Adc::calibrate() const {
   // Ensure that the ADC is disabled
   if(_adc.CR & ADC_CR_ADEN) {
      _adc.CR &= ~ADC_CR_ADEN;
   }
   // Start calibration
   _adc.CR |= ADC_CR_ADCAL;
   // Wait until calibration finished
   while(_adc.CR & ADC_CR_ADCAL) {
   }
   // Clear EOCAL flag
   _adc.ISR |= ADC_ISR_EOCAL;
}

void Adc::setSamplingTime(SamplingGroup group, SamplingCycles cycles) const {
   int shift = group == SamplingGroup::Group1 ? ADC_SMPR_SMP1_Pos : ADC_SMPR_SMP2_Pos;
   _adc.SMPR = (_adc.SMPR & ~(0b111 << shift)) | (static_cast<uint32_t>(cycles) << shift);
}

void Adc::setChannelSamplingGroup(Channel channel, SamplingGroup group) const {
   _adc.SMPR = (_adc.SMPR & ~(1 << (static_cast<uint32_t>(channel) + ADC_SMPR_SMPSEL_Pos))) |
               static_cast<uint32_t>(group) << (static_cast<uint32_t>(channel) + ADC_SMPR_SMPSEL_Pos);
}

void Adc::setExternalTrigger(ExternalTrigger trigger, TriggerEdge edge) const {
   _adc.CFGR1 = (_adc.CFGR1 & ~(ADC_CFGR1_EXTSEL | ADC_CFGR1_EXTEN)) |
                ((static_cast<uint32_t>(edge) << ADC_CFGR1_EXTEN_Pos) | (static_cast<uint32_t>(trigger) << ADC_CFGR1_EXTSEL_Pos));
}

void Adc::enableChannel(Adc::Channel channel) const {
   assert(getChannelSequenceMode() == ChannelSequenceMode::Fixed);
   _adc.ISR |= ADC_ISR_CCRDY;
   _adc.CHSELR |= (1UL << static_cast<uint32_t>(channel));
   while(!(_adc.ISR & ADC_ISR_CCRDY)) {
   }
}

void Adc::disableChannel(Adc::Channel channel) const {
   assert(getChannelSequenceMode() == ChannelSequenceMode::Fixed);
   _adc.ISR |= ADC_ISR_CCRDY;
   _adc.CHSELR &= ~(1UL << static_cast<uint32_t>(channel));
   while(!(_adc.ISR & ADC_ISR_CCRDY)) {
   }
}

void Adc::setChannelSequence(int sequenceNo, Adc::Channel channel) const {
   assert(getChannelSequenceMode() == ChannelSequenceMode::Configurable);
   assert(sequenceNo < 8);
   assert(channel <= Channel::Vbat);
   _adc.ISR |= ADC_ISR_CCRDY;
   _adc.CHSELR = (_adc.CHSELR & ~(0xF << sequenceNo * 4)) | (static_cast<uint32_t>(channel) << sequenceNo * 4);
   while(!(_adc.ISR & ADC_ISR_CCRDY)) {
   }
}

void Adc::setChannelSequenceCount(int count) const {
   assert(getChannelSequenceMode() == ChannelSequenceMode::Configurable);
   assert(count > 0 && count <= 8);
   _adc.ISR |= ADC_ISR_CCRDY;
   if(count < 8) {
      _adc.CHSELR |= (0xF << count * 4);
   }
   while(!(_adc.ISR & ADC_ISR_CCRDY)) {
   }
}

#endif
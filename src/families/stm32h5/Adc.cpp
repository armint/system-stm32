#include "Adc.h"

#include <delaybusywait.h>

#include "peripherals/Rcc.h"

static_assert(Adc::supplyFromVrefint(1500, 1500) == 3300, "VDDA from Vrefint");
static_assert(Adc::supplyFromVrefint(1650, 1500) == 3000, "VDDA from Vrefint, lower VDDA");
static_assert(Adc::supplyFromVrefint(0, 1500) == 0, "VDDA from Vrefint, zero reading");
static_assert(Adc::toMillivolts(4095, 3300) == 3300, "full scale");

namespace {
// Divider of ADC_CCR.PRESC.
constexpr uint16_t presc[12] = { 1, 2, 4, 6, 8, 10, 12, 16, 32, 64, 128, 256 };

bool waitClear(const volatile uint32_t &reg, uint32_t mask) {
   for(uint32_t i = 0; i < Adc::pollLimit; ++i) {
      if((reg & mask) == 0) {
         return true;
      }
   }
   return false;
}

bool waitSet(const volatile uint32_t &reg, uint32_t mask) {
   for(uint32_t i = 0; i < Adc::pollLimit; ++i) {
      if((reg & mask) != 0) {
         return true;
      }
   }
   return false;
}

// Position of a 5-bit channel number in SQR1..SQR4: SQR1 has L in bits 3:0,
// then ranks 1..4; SQR2 ranks 5..9; SQR3 10..14; SQR4 15..16. 6 bits per rank.
struct RankSlot {
   uint8_t sqr; // 0: SQR1 .. 3: SQR4
   uint8_t shift;
};
constexpr RankSlot rankSlot(uint8_t rank) {
   if(rank <= 4) {
      return { 0, static_cast<uint8_t>(rank * 6U) };
   }
   if(rank <= 9) {
      return { 1, static_cast<uint8_t>((rank - 5U) * 6U) };
   }
   if(rank <= 14) {
      return { 2, static_cast<uint8_t>((rank - 10U) * 6U) };
   }
   return { 3, static_cast<uint8_t>((rank - 15U) * 6U) };
}
static_assert(rankSlot(1).sqr == 0 && rankSlot(1).shift == ADC_SQR1_SQ1_Pos);
static_assert(rankSlot(4).sqr == 0 && rankSlot(4).shift == ADC_SQR1_SQ4_Pos);
static_assert(rankSlot(5).sqr == 1 && rankSlot(5).shift == ADC_SQR2_SQ5_Pos);
static_assert(rankSlot(9).sqr == 1 && rankSlot(9).shift == ADC_SQR2_SQ9_Pos);
static_assert(rankSlot(10).sqr == 2 && rankSlot(10).shift == ADC_SQR3_SQ10_Pos);
static_assert(rankSlot(14).sqr == 2 && rankSlot(14).shift == ADC_SQR3_SQ14_Pos);
static_assert(rankSlot(15).sqr == 3 && rankSlot(15).shift == ADC_SQR4_SQ15_Pos);
static_assert(rankSlot(16).sqr == 3 && rankSlot(16).shift == ADC_SQR4_SQ16_Pos);

// 3 bits of sample time per channel: SMPR1 channels 0..9, SMPR2 10..19.
constexpr uint32_t smpShift(uint8_t channel) {
   return (channel % 10U) * 3U;
}
static_assert(smpShift(9) == ADC_SMPR1_SMP9_Pos && smpShift(10) == ADC_SMPR2_SMP10_Pos);
// The CMSIS header has no SMP19 (RM0481 section 28.6.7 has it: bits 29:27).
static_assert(smpShift(18) == ADC_SMPR2_SMP18_Pos && smpShift(19) == ADC_SMPR2_SMP18_Pos + 3U);

void setSmp(ADC_TypeDef &adc, uint8_t channel, uint32_t cycles) {
   volatile uint32_t &smpr = channel < 10 ? adc.SMPR1 : adc.SMPR2;
   const uint32_t shift = smpShift(channel);
   smpr = (smpr & ~(0x7UL << shift)) | ((cycles & 0x7UL) << shift);
}
} // namespace

uint32_t Adc::clockHz() {
   const uint32_t ccr = ADC12_COMMON->CCR;
   const uint32_t ckmode = (ccr & ADC_CCR_CKMODE) >> ADC_CCR_CKMODE_Pos;
   if(ckmode != 0) {
      return SystemCoreClock >> (ckmode - 1); // 1: /1, 2: /2, 3: /4
   }
   uint32_t kernelHz;
   switch(getKernelClock()) {
   case KernelClock::Hclk:
      kernelHz = SystemCoreClock;
      break;
   case KernelClock::Sysclk: {
      // sys_ck = hclk * HPRE. HPRE: 0xxx /1, 1000../1111: /2, 4, 8, 16, 64, 128, 256, 512.
      constexpr uint8_t hpreShift[8] = { 1, 2, 3, 4, 6, 7, 8, 9 };
      const uint32_t hpre = (RCC->CFGR2 & RCC_CFGR2_HPRE) >> RCC_CFGR2_HPRE_Pos;
      kernelHz = hpre < 8 ? SystemCoreClock : SystemCoreClock << hpreShift[hpre - 8];
      break;
   }
   case KernelClock::Hse:
      kernelHz = HSE_VALUE;
      break;
   case KernelClock::HsiKer:
      kernelHz = Rcc::getHSIClock();
      break;
   case KernelClock::CsiKer:
      kernelHz = Rcc::CSIHz;
      break;
   default:
      return 0; // PLL2R or reserved
   }
   const uint32_t prescIndex = (ccr & ADC_CCR_PRESC) >> ADC_CCR_PRESC_Pos;
   return prescIndex < 12 ? kernelHz / presc[prescIndex] : 0;
}

uint32_t Adc::configureClock() {
   const uint32_t hclk = SystemCoreClock;
   // CKMODE = 01 (hclk / 1) needs HPRE = /1: RCC_CFGR2.HPRE[3] = 0.
   const bool hpreDiv1 = (RCC->CFGR2 & RCC_CFGR2_HPRE_3) == 0;
   if(hpreDiv1 && hclk <= maxClockHz) {
      setClockMode(ClockMode::HclkDiv1);
      return hclk;
   }
   if(hclk / 2 <= maxClockHz) {
      setClockMode(ClockMode::HclkDiv2);
      return hclk / 2;
   }
   if(hclk / 4 <= maxClockHz) {
      setClockMode(ClockMode::HclkDiv4);
      return hclk / 4;
   }
   return 0;
}

bool Adc::powerUp() const {
   if(isEnabled()) {
      return false;
   }
   if((_adc.CR & (ADC_CR_DEEPPWD | ADC_CR_ADVREGEN)) == ADC_CR_ADVREGEN) {
      return true;
   }
   writeCrConfig(ADC_CR_DEEPPWD | ADC_CR_ADVREGEN, 0);
   writeCrConfig(ADC_CR_ADVREGEN, ADC_CR_ADVREGEN);
   delayuS(10); // tADCVREG_STUP, DS14539 table 179: 10 us maximum
   return true;
}

bool Adc::powerDown() const {
   if(!disable()) {
      return false;
   }
   writeCrConfig(ADC_CR_ADVREGEN, 0);
   writeCrConfig(ADC_CR_DEEPPWD, ADC_CR_DEEPPWD);
   return true;
}

bool Adc::calibrate(InputMode mode) const {
   if((_adc.CR & (ADC_CR_DEEPPWD | ADC_CR_ADVREGEN)) != ADC_CR_ADVREGEN) {
      return false;
   }
   const bool wasEnabled = isEnabled();
   if(wasEnabled && !disable()) {
      return false;
   }
   writeCrConfig(ADC_CR_ADCALDIF, mode == InputMode::Differential ? ADC_CR_ADCALDIF : 0U);
   command(ADC_CR_ADCAL);
   if(!waitClear(_adc.CR, ADC_CR_ADCAL)) {
      return false;
   }
   // ADEN is ignored during four ADC clock cycles after ADCAL clears
   // (RM0481 section 28.4.9). 4 cycles of the slowest ADC clock (1.5 MHz)
   // are less than 3 us. enable() also sets ADEN again while it reads 0.
   delayuS(3);
   return wasEnabled ? enable() : true;
}

bool Adc::enable() const {
   if(isEnabled()) {
      return true;
   }
   _adc.ISR = ADC_ISR_ADRDY;
   command(ADC_CR_ADEN);
   for(uint32_t i = 0; i < pollLimit; ++i) {
      if(_adc.ISR & ADC_ISR_ADRDY) {
         _adc.ISR = ADC_ISR_ADRDY;
         return true;
      }
      // The hardware ignores ADEN shortly after a calibration: set it again.
      if(!isEnabled()) {
         command(ADC_CR_ADEN);
      }
   }
   return false;
}

bool Adc::disable() const {
   if(!isEnabled()) {
      return true;
   }
   if(!stop() || !stopInjected()) {
      return false;
   }
   command(ADC_CR_ADDIS);
   return waitClear(_adc.CR, ADC_CR_ADEN);
}

bool Adc::stop() const {
   if(_adc.CR & ADC_CR_ADSTART) {
      command(ADC_CR_ADSTP);
   }
   return waitClear(_adc.CR, ADC_CR_ADSTP);
}

bool Adc::stopInjected() const {
   if(_adc.CR & ADC_CR_JADSTART) {
      command(ADC_CR_JADSTP);
   }
   return waitClear(_adc.CR, ADC_CR_JADSTP);
}

void Adc::setSamplingTime(Channel channel, SamplingCycles cycles) const {
   if(channel < channelCount) {
      setSmp(_adc, channel, static_cast<uint32_t>(cycles));
   }
}

void Adc::setRegularRank(uint8_t rank, Channel channel) const {
   if(rank < 1 || rank > 16) {
      return;
   }
   const RankSlot slot = rankSlot(rank);
   volatile uint32_t *const sqr[4] = { &_adc.SQR1, &_adc.SQR2, &_adc.SQR3, &_adc.SQR4 };
   volatile uint32_t &reg = *sqr[slot.sqr];
   reg = (reg & ~(0x1FUL << slot.shift)) | (static_cast<uint32_t>(channel & 0x1FU) << slot.shift);
}

bool Adc::convert(Channel channel, uint16_t &value) const {
   if(!isEnabled() || isConverting()) {
      return false;
   }
   _adc.SQR1 = static_cast<uint32_t>(channel & 0x1FU) << ADC_SQR1_SQ1_Pos; // L = 0: one rank
   // OVRMOD = 1 disables the 3-stage DR FIFO, so DR holds this result and
   // not older unread data (RM0481 section 28.4.27). Restored afterwards.
   const uint32_t ovrmod = _adc.CFGR & ADC_CFGR_OVRMOD;
   _adc.CFGR = (_adc.CFGR & ~(ADC_CFGR_EXTEN | ADC_CFGR_CONT | ADC_CFGR_DISCEN)) | ADC_CFGR_OVRMOD;
   _adc.ISR = ADC_ISR_EOC | ADC_ISR_EOS | ADC_ISR_OVR;
   start();
   const bool ok = waitSet(_adc.ISR, ADC_ISR_EOC);
   if(ok) {
      value = static_cast<uint16_t>(_adc.DR);
   } else {
      (void)stop();
   }
   _adc.CFGR = (_adc.CFGR & ~ADC_CFGR_OVRMOD) | ovrmod;
   return ok;
}

bool Adc::readSupplyMillivolts(uint32_t &vddaMv) const {
   if(&_adc != ADC1) {
      return false; // channel 17 is VDDCORE on ADC2
   }
   enableVrefint();
   // Smallest sample time of at least vrefintSampleNs (rounded up to 5 us).
   const uint32_t hz = clockHz();
   constexpr uint16_t halfCycles[8] = { 5, 13, 25, 49, 95, 185, 495, 1281 };
   uint32_t smp = 7;
   for(uint32_t i = 0; i < 8 && hz != 0; ++i) {
      // halfCycles / 2 / hz >= 5 us  <=>  halfCycles * 100000 >= hz
      if(static_cast<uint64_t>(halfCycles[i]) * 100000ULL >= hz) {
         smp = i;
         break;
      }
   }
   setSamplingTime(Vrefint, static_cast<SamplingCycles>(smp));
   delayuS(10); // Vrefint buffer start-up (DS14539: tstart_vrefint)
   uint16_t raw;
   if(!convert(Vrefint, raw) || raw == 0) {
      return false;
   }
   vddaMv = supplyFromVrefint(raw, Calibration::vrefint());
   return true;
}

bool Adc::setInjectedSequence(const Channel *channels, uint8_t count, InjectedTrigger trigger, TriggerEdge edge) const {
   // With the queue on, JEXTEN = 00 disables the software trigger too
   // (RM0481 section 28.6.16): the sequence would never convert.
   const bool queueOn = (_adc.CFGR & ADC_CFGR_JQDIS) == 0;
   if(count < 1 || count > 4 || (queueOn && edge == TriggerEdge::Software)) {
      return false;
   }
   uint32_t jsqr = (static_cast<uint32_t>(count - 1U) << ADC_JSQR_JL_Pos) |
                   (static_cast<uint32_t>(trigger) << ADC_JSQR_JEXTSEL_Pos) |
                   (static_cast<uint32_t>(edge) << ADC_JSQR_JEXTEN_Pos);
   constexpr uint32_t pos[4] = { ADC_JSQR_JSQ1_Pos, ADC_JSQR_JSQ2_Pos, ADC_JSQR_JSQ3_Pos, ADC_JSQR_JSQ4_Pos };
   for(uint8_t i = 0; i < count; ++i) {
      jsqr |= static_cast<uint32_t>(channels[i] & 0x1FU) << pos[i];
   }
   _adc.JSQR = jsqr;
   return true;
}

uint16_t Adc::injectedValue(uint8_t rank) const {
   switch(rank) {
   case 1:
      return static_cast<uint16_t>(_adc.JDR1);
   case 2:
      return static_cast<uint16_t>(_adc.JDR2);
   case 3:
      return static_cast<uint16_t>(_adc.JDR3);
   case 4:
      return static_cast<uint16_t>(_adc.JDR4);
   default:
      return 0;
   }
}

void Adc::setOffset(Offset unit, Channel channel, uint16_t value, bool add, bool saturate) const {
   volatile uint32_t *ofr = &_adc.OFR1 + static_cast<uint8_t>(unit);
   *ofr = ADC_OFR1_OFFSET1_EN | (static_cast<uint32_t>(channel & 0x1FU) << ADC_OFR1_OFFSET1_CH_Pos) |
          (add ? ADC_OFR1_OFFSETPOS : 0U) | (saturate ? ADC_OFR1_SATEN : 0U) | (static_cast<uint32_t>(value) & ADC_OFR1_OFFSET1);
}

void Adc::disableOffset(Offset unit) const {
   volatile uint32_t *ofr = &_adc.OFR1 + static_cast<uint8_t>(unit);
   *ofr &= ~ADC_OFR1_OFFSET1_EN;
}

void Adc::setOversampling(OversamplingRatio ratio, uint8_t shift, bool regular, bool injected, bool triggered, RegularOversamplingMode mode) const {
   constexpr uint32_t mask = ADC_CFGR2_ROVSE | ADC_CFGR2_JOVSE | ADC_CFGR2_OVSR | ADC_CFGR2_OVSS | ADC_CFGR2_TROVS | ADC_CFGR2_ROVSM;
   const uint32_t ovss = shift > 8 ? 8U : shift;
   _adc.CFGR2 = (_adc.CFGR2 & ~mask) | (regular ? ADC_CFGR2_ROVSE : 0U) | (injected ? ADC_CFGR2_JOVSE : 0U) |
                (static_cast<uint32_t>(ratio) << ADC_CFGR2_OVSR_Pos) | (ovss << ADC_CFGR2_OVSS_Pos) |
                (triggered ? ADC_CFGR2_TROVS : 0U) | static_cast<uint32_t>(mode);
}

void Adc::setWatchdog1(Channel channel, bool allChannels, bool regular, bool injected, uint16_t low, uint16_t high, uint8_t filter) const {
   _adc.TR1 = (static_cast<uint32_t>(low) & ADC_TR1_LT1) | ((static_cast<uint32_t>(filter) & 0x7U) << ADC_TR1_AWDFILT_Pos) |
              ((static_cast<uint32_t>(high) << ADC_TR1_HT1_Pos) & ADC_TR1_HT1);
   constexpr uint32_t mask = ADC_CFGR_AWD1CH | ADC_CFGR_AWD1SGL | ADC_CFGR_AWD1EN | ADC_CFGR_JAWD1EN;
   _adc.CFGR = (_adc.CFGR & ~mask) | (static_cast<uint32_t>(channel & 0x1FU) << ADC_CFGR_AWD1CH_Pos) |
               (allChannels ? 0U : ADC_CFGR_AWD1SGL) | (regular ? ADC_CFGR_AWD1EN : 0U) | (injected ? ADC_CFGR_JAWD1EN : 0U);
}

void Adc::setWatchdog23(Watchdog watchdog, uint32_t channelMask, uint8_t low, uint8_t high) const {
   const uint32_t tr = (static_cast<uint32_t>(low) << ADC_TR2_LT2_Pos) | (static_cast<uint32_t>(high) << ADC_TR2_HT2_Pos);
   if(watchdog == Watchdog::Awd2) {
      _adc.TR2 = tr;
      _adc.AWD2CR = channelMask & ADC_AWD2CR_AWD2CH;
   } else if(watchdog == Watchdog::Awd3) {
      _adc.TR3 = tr;
      _adc.AWD3CR = channelMask & ADC_AWD3CR_AWD3CH;
   }
}

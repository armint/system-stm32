#pragma once

#include <cmsis_device.h>

struct Adc {
   enum class ClockSource : uint32_t {
      SYSCLK = 0,
      HSIKER = RCC_CCIPR_ADCSEL_1,
   };

   enum class ClockMode : uint32_t {
      ADCCLK = 0,
      PCLK_DIV_2 = ADC_CFGR2_CKMODE_0,
      PCLK_DIV_4 = ADC_CFGR2_CKMODE_1,
      PCLK = ADC_CFGR2_CKMODE_1 | ADC_CFGR2_CKMODE_0,

   };

   enum class SamplingCycles {
      ADCSampling1_5 = 0,
      ADCSampling3_5,
      ADCSampling7_5,
      ADCSampling12_5,
      ADCSampling19_5,
      ADCSampling39_5,
      ADCSampling79_5,
      ADCSampling160_5
   };

   enum class TriggerEdge { Disabled = 0, Rising, Falling, Both };

   enum class ExternalTrigger {
      TIM1_TRGO = 0,
      TIM1_CC4 = 1,
      TIM3_TRGO = 3,
      EXTI11 = 7,
   };

   enum class DMAMode {
      OneShot,
      Circular = ADC_CFGR1_DMACFG,
   };

   enum Channel {
      PA0 = 0,
      PA1 = 1,
      PA2 = 2,
      PA3 = 3,
      PA4 = 4,
      PA5 = 5,
      PA6 = 6,
      PA7 = 7,
      PA8 = 8,
      Vsense = 9,
      Vrefint = 10,
      PA11 = 11,
      PA12 = 12,
      PA14 = 13,
      PA14_ALT = 14,
      VDDA = 15,
      VSSA = 16,
   };

   enum class OversamplingRatio : uint32_t {
      x2 = 0,
      x4 = ADC_CFGR2_OVSR_0,
      x8 = ADC_CFGR2_OVSR_1,
      x16 = ADC_CFGR2_OVSR_1 | ADC_CFGR2_OVSR_0,
      x32 = ADC_CFGR2_OVSR_2,
      x66 = ADC_CFGR2_OVSR_2 | ADC_CFGR2_OVSR_0,
      x128 = ADC_CFGR2_OVSR_2 | ADC_CFGR2_OVSR_1,
      x256 = ADC_CFGR2_OVSR_2 | ADC_CFGR2_OVSR_1 | ADC_CFGR2_OVSR_0

   };

   enum class SamplingResolution : uint32_t {
      res12bit = 0,
      res10bit = ADC_CFGR1_RES_0,
      res8bit = ADC_CFGR1_RES_1,
      res6bit = ADC_CFGR1_RES_1 | ADC_CFGR1_RES_0,
   };

   enum class SamplingGroup : bool { Group1, Group2 };
   enum class ConversionMode : uint32_t { Group = 0, Single = ADC_CFGR1_DISCEN, Continuous = ADC_CFGR1_CONT };
   enum class ChannelSequenceMode : uint32_t { Fixed = 0, Configurable = ADC_CFGR1_CHSELRMOD };
   enum class ScanDirection : uint32_t { Forward = 0, Backward = ADC_CFGR1_SCANDIR };
   enum class OverrunMode : uint32_t { Preserve = 0, Overwrite = ADC_CFGR1_OVRMOD };

   struct TemperatureSensorCalibration {
      // Factory calibration data addresses (Vref+ = 3.0V)
      static uint16_t cal1() { return *reinterpret_cast<const volatile uint16_t*>(0x1FFF75A8UL); } // 30 °C
      static uint16_t cal2() { return *reinterpret_cast<const volatile uint16_t*>(0x1FFF75CAUL); } // 130 °C
      static constexpr int32_t temp1 = 30;
      static constexpr int32_t temp2 = 130;
      static constexpr uint32_t vrefAnalog = 3000; // mV
   };

   static inline void enableTemperatureSensor() {
      ADC1_COMMON->CCR |= ADC_CCR_TSEN;
   }

   static inline void disableTemperatureSensor() {
      ADC1_COMMON->CCR &= ~ADC_CCR_TSEN;
   }

   static inline void enableVrefint() {
      ADC1_COMMON->CCR |= ADC_CCR_VREFEN;
   }

   static inline void disableVrefint() {
      ADC1_COMMON->CCR &= ~ADC_CCR_VREFEN;
   }

   static void reset() {
      RCC->APBRSTR2 |= RCC_APBRSTR2_ADCRST;
      RCC->APBRSTR2 &= ~RCC_APBRSTR2_ADCRST;
   }


   static inline void setClockSource(ClockSource clockSource) {
      RCC->CCIPR = (RCC->CCIPR & ~RCC_CCIPR_ADCSEL) | static_cast<uint32_t>(clockSource);
   }

   static inline ClockSource getClockSource() {
      return static_cast<ClockSource>(RCC->CCIPR & RCC_CCIPR_ADCSEL);
   }

   static inline void enableClock() {
      RCC->APBENR2 |= RCC_APBENR2_ADCEN;
   }

   ADC_TypeDef &_adc;

   explicit Adc(ADC_TypeDef *adc)
      : _adc(*adc) {}

   Adc(const Adc &) = default;
   Adc(Adc &&) = default;
   Adc &operator=(const Adc &) = delete;
   Adc &operator=(Adc &&) = delete;


   void enable() const;
   void disable() const;
   void calibrate() const;

   inline void start() const {
      _adc.CR |= ADC_CR_ADSTART;
   }

   inline uint16_t value() const {
      return _adc.DR;
   }

   void enableChannel(Channel channel) const;

   void disableChannel(Channel channel) const;

   void setChannelSequence(int sequenceNo, Adc::Channel channel) const;

   void setChannelSequenceCount(int count) const;

   void setSamplingTime(SamplingGroup group, SamplingCycles cycles) const;

   void setChannelSamplingGroup(Channel channel, SamplingGroup group) const;

   void setExternalTrigger(ExternalTrigger trigger, TriggerEdge edge) const;

   inline void disableExternalTrigger() const {
      _adc.CFGR1 &= ~ADC_CFGR1_EXTEN;
   }

   inline void enableAutoOff() const {
      _adc.CFGR1 |= ADC_CFGR1_AUTOFF;
   }

   inline void disableAutoOff() const {
      _adc.CFGR1 &= ~ADC_CFGR1_AUTOFF;
   }

   inline void enableOverrunInterrupt() const {
      _adc.IER |= ADC_IER_OVRIE;
   }

   inline void disableOverrunInterrupt() const {
      _adc.IER &= ~ADC_IER_OVRIE;
   }

   inline void clearOverrunFlag() const {
      _adc.ISR |= ADC_ISR_OVR;
   }

   inline void enableEndOfConversionInterrupt() const {
      _adc.IER |= ADC_IER_EOCIE;
   }

   inline void disableEndOfConversionInterrupt() const {
      _adc.IER &= ~ADC_IER_EOCIE;
   }

   inline void clearEndOfConversionFlag() const {
      _adc.ISR |= ADC_ISR_EOC;
   }

   inline void enableEndOfSequenceInterrupt() const {
      _adc.IER |= ADC_IER_EOSEQIE;
   }

   inline void disableEndOfSequenceInterrupt() const {
      _adc.IER &= ~ADC_IER_EOSEQIE;
   }

   inline void clearEndOfSequenceFlag() const {
      _adc.ISR |= ADC_ISR_EOSEQ;
   }

   inline void disableAdcVoltageRegulator() const {
      _adc.CR &= ~ADC_CR_ADVREGEN;
   }

   inline bool conversionComplete() const {
      return _adc.ISR & ADC_ISR_EOC;
   }

   inline void enableOverSampling() const {
      _adc.CFGR2 |= ADC_CFGR2_OVSE;
   }

   inline void disableOverSampling() const {
      _adc.CFGR2 &= ~ADC_CFGR2_OVSE;
   }

   inline void setOverSamplingRatio(OversamplingRatio ratio) const {
      _adc.CFGR2 = (_adc.CFGR2 & ~ADC_CFGR2_OVSR) | static_cast<uint32_t>(ratio);
   }

   inline void setOverSamplingShift(uint8_t bits) const {
      _adc.CFGR2 = (_adc.CFGR2 & ~ADC_CFGR2_OVSS) | (static_cast<uint32_t>(bits & 0b1111) << ADC_CFGR2_OVSS_Pos);
   }

   inline void setOverSamplingTriggerAll() const {
      _adc.CFGR2 &= ~ADC_CFGR2_TOVS;
   }

   inline void setOverSamplingTriggerEach() const {
      _adc.CFGR2 |= ADC_CFGR2_TOVS;
   }

   inline void setConversionMode(ConversionMode mode) const {
      _adc.CFGR1 = (_adc.CFGR1 & ~(ADC_CFGR1_CONT | ADC_CFGR1_DISCEN)) | static_cast<uint32_t>(mode);
   }

   inline void setDMAMode(DMAMode mode) const {
      _adc.CFGR1 = (_adc.CFGR1 & ~ADC_CFGR1_DMACFG) | static_cast<uint32_t>(mode);
   }

   inline void enableDMA() const {
      _adc.CFGR1 |= ADC_CFGR1_DMAEN;
   }

   inline void disableDMA() const {
      _adc.CFGR1 &= ~ADC_CFGR1_DMAEN;
   }

   inline void setScanDirectionForward() const {
      _adc.CFGR1 &= ~ADC_CFGR1_SCANDIR;
   }

   inline void setScanDirectionBackward() const {
      _adc.CFGR1 |= ADC_CFGR1_SCANDIR;
   }

   inline void setClockMode(ClockMode mode) const {
      _adc.CFGR2 = (_adc.CFGR2 & ~ADC_CFGR2_CKMODE) | static_cast<uint32_t>(mode);
   }


   inline void enableVoltageRegulator() const {
      _adc.CR |= ADC_CR_ADVREGEN;
   }

   inline void disableVoltageRegulator() const {
      _adc.CR &= ~ADC_CR_ADVREGEN;
   }

   inline void setChannelSequenceMode(ChannelSequenceMode mode) const {
      _adc.CFGR1 = (_adc.CFGR1 & ~ADC_CFGR1_CHSELRMOD) | static_cast<uint32_t>(mode);
   }

   inline ChannelSequenceMode getChannelSequenceMode() const {
      return static_cast<ChannelSequenceMode>(_adc.CFGR1 & ADC_CFGR1_CHSELRMOD);
   }

   inline void setScanDirection(ScanDirection direction) const {
      _adc.CFGR1 = (_adc.CFGR1 & ~ADC_CFGR1_SCANDIR) | static_cast<uint32_t>(direction);
   }

   inline ScanDirection getScanDirection() const {
      return static_cast<ScanDirection>(_adc.CFGR1 & ADC_CFGR1_SCANDIR);
   }

   inline void setSamplingResolution(SamplingResolution resolution) const {
      _adc.CFGR1 = (_adc.CFGR1 & ~ADC_CFGR1_RES) | static_cast<uint32_t>(resolution);
   }

   inline SamplingResolution getSamplingResolution() const {
      return static_cast<SamplingResolution>(_adc.CFGR1 & ADC_CFGR1_RES);
   }

   inline void enableWaitConversionMode() const {
      _adc.CFGR1 |= ADC_CFGR1_WAIT;
   }

   inline void disableWaitConversionMode() const {
      _adc.CFGR1 &= ~ADC_CFGR1_WAIT;
   }

   inline void enableLowFrequencyTriggerMode() const {
      _adc.CFGR2 |= ADC_CFGR2_LFTRIG;
   }

   inline void disableLowFrequencyTriggerMode() const {
      _adc.CFGR2 &= ~ADC_CFGR2_LFTRIG;
   }

   inline void setOverrunMode(OverrunMode mode) const {
      _adc.CFGR1 = (_adc.CFGR1 & ~ADC_CFGR1_OVRMOD) | static_cast<uint32_t>(mode);
   }
};

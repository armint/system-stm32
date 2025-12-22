#pragma once

#include <cmsis_device.h>
#include <cstdint>
#include <initializer_list>

class Adc {
   public:
   // Mapping of Channels to Standard Pins (STM32L073)
   enum class Channel : uint8_t {
      PA0 = 0,
      PA1 = 1,
      PA2 = 2,
      PA3 = 3,
      PA4 = 4,
      PA5 = 5,
      PA6 = 6,
      PA7 = 7,
      PB0 = 8,
      PB1 = 9,
      PC0 = 10,
      PC1 = 11,
      PC2 = 12,
      PC3 = 13,
      PC4 = 14,
      PC5 = 15,
      // Channel 16 is reserved/internal dependent on package
      VLCD = 16,       // Often mapped to Ch16 on L073 if LCD is present
      VRefInt = 17,    // Internal Voltage Reference
      TempSensor = 18, // Internal Temperature Sensor
   };

   enum class ClockMode : uint32_t {
      Async = 0,                                            // ADCCLK (Asynchronous clock mode)
      Pclk_Div1 = ADC_CFGR2_CKMODE_0,                       // PCLK / 1 (Synchronous)
      Pclk_Div2 = ADC_CFGR2_CKMODE_1,                       // PCLK / 2 (Synchronous)
      Pclk_Div4 = (ADC_CFGR2_CKMODE_0 | ADC_CFGR2_CKMODE_1) // PCLK / 4 (Synchronous)
   };

   // ADC_CFGR2_OVSR: Oversampling Ratio
   enum class OversamplingRatio : uint32_t {
      Ratio_2x = 0,
      Ratio_4x = ADC_CFGR2_OVSR_0,
      Ratio_8x = ADC_CFGR2_OVSR_1,
      Ratio_16x = (ADC_CFGR2_OVSR_0 | ADC_CFGR2_OVSR_1),
      Ratio_32x = ADC_CFGR2_OVSR_2,
      Ratio_64x = (ADC_CFGR2_OVSR_0 | ADC_CFGR2_OVSR_2),
      Ratio_128x = (ADC_CFGR2_OVSR_1 | ADC_CFGR2_OVSR_2),
      Ratio_256x = (ADC_CFGR2_OVSR_0 | ADC_CFGR2_OVSR_1 | ADC_CFGR2_OVSR_2)
   };

   // ADC_CFGR2_OVSS: Oversampling Shift (Right Shift)
   // Used to prevent register overflow or to scale the result back down.
   enum class OversamplingShift : uint32_t {
      Shift_0 = 0,
      Shift_1 = ADC_CFGR2_OVSS_0,
      Shift_2 = ADC_CFGR2_OVSS_1,
      Shift_3 = (ADC_CFGR2_OVSS_0 | ADC_CFGR2_OVSS_1),
      Shift_4 = ADC_CFGR2_OVSS_2,
      Shift_5 = (ADC_CFGR2_OVSS_0 | ADC_CFGR2_OVSS_2),
      Shift_6 = (ADC_CFGR2_OVSS_1 | ADC_CFGR2_OVSS_2),
      Shift_7 = (ADC_CFGR2_OVSS_0 | ADC_CFGR2_OVSS_1 | ADC_CFGR2_OVSS_2),
      Shift_8 = ADC_CFGR2_OVSS_3
   };

   // ADC_CFGR1_RES
   enum class Resolution : uint32_t {
      Bits_12 = 0,                                 // 12-bit
      Bits_10 = ADC_CFGR1_RES_0,                   // 10-bit
      Bits_8 = ADC_CFGR1_RES_1,                    // 8-bit
      Bits_6 = (ADC_CFGR1_RES_0 | ADC_CFGR1_RES_1) // 6-bit
   };

   // ADC_SMPR_SMP
   enum class SamplingTime : uint32_t {
      Cycles_1_5 = 0,
      Cycles_3_5 = ADC_SMPR_SMP_0,
      Cycles_7_5 = ADC_SMPR_SMP_1,
      Cycles_12_5 = (ADC_SMPR_SMP_0 | ADC_SMPR_SMP_1),
      Cycles_19_5 = ADC_SMPR_SMP_2,
      Cycles_39_5 = (ADC_SMPR_SMP_0 | ADC_SMPR_SMP_2),
      Cycles_79_5 = (ADC_SMPR_SMP_1 | ADC_SMPR_SMP_2),
      Cycles_160_5 = (ADC_SMPR_SMP_0 | ADC_SMPR_SMP_1 | ADC_SMPR_SMP_2)
   };


   ADC_TypeDef &adc;

   explicit Adc(ADC_TypeDef *adcBase)
      : adc(*adcBase) {
   }

   static void disableTempSensor();
   static void disableVRefInt();
   static void enableTempSensor();
   static void enableVRefInt();

   void calibrate() const;

   void clearFlag(uint32_t flag) const {
      adc.ISR = flag;
   }

   void disable() const;

   void disableContinuousMode() const {
      adc.CFGR1 &= ~ADC_CFGR1_CONT;
   }

   void disableDma() const {
      adc.CFGR1 &= ~ADC_CFGR1_DMAEN;
   }

   void disableOverrun() const {
      adc.CFGR1 &= ~ADC_CFGR1_OVRMOD;
   }

   /**
    * @brief Disables the Hardware Oversampler.
    */
   void disableOversampling() const;

   void enable() const;

   void enableContinuousMode() const {
      adc.CFGR1 |= ADC_CFGR1_CONT;
   }

   void enableDma() const {
      adc.CFGR1 |= ADC_CFGR1_DMAEN;
   }

   void enableOverrun() const {
      adc.CFGR1 |= ADC_CFGR1_OVRMOD;
   }

   /**
    * @brief Enables and configures the Hardware Oversampler.
    * @param ratio How many samples to accumulate (2x to 256x).
    * @param shift How many bits to right-shift the final result (0 to 8).
    * @note ADC must be disabled to change this. This method handles disable/enable automatically.
    */
   void enableOversampling(OversamplingRatio ratio, OversamplingShift shift) const;

   [[nodiscard]] uint16_t getData() const {
      return static_cast<uint16_t>(adc.DR);
   }

   // EOC = End of Conversion (One channel finished)
   [[nodiscard]] bool isConversionComplete() const {
      return (adc.ISR & ADC_ISR_EOC);
   }

   [[nodiscard]] bool isReady() const {
      return (adc.ISR & ADC_ISR_ADRDY);
   }

   // EOSEQ = End of Sequence (All selected channels finished)
   [[nodiscard]] bool isSequenceComplete() const {
      return (adc.ISR & ADC_ISR_EOSEQ);
   }

   void setClockMode(ClockMode mode) const;
   void setResolution(Resolution resolution) const;
   void setSamplingTime(SamplingTime cycles) const;

   /**
    * @brief Configures the Sequence of channels to scan.
    * @note On STM32L0, channels are always scanned in Ascending Order (0 -> 18).
    * @param channels List of channels, e.g., {Adc::Channel::PA0, Adc::Channel::VRefInt}
    */
   void setSequence(std::initializer_list<Channel> channels) const;

   /**
    * @brief Raw method to set sequence via bitmask.
    */
   void setSequence(uint32_t channelMask) const;

   void startConversion() const {
      adc.CR |= ADC_CR_ADSTART;
   }

   void stopConversion() const {
      adc.CR |= ADC_CR_ADSTP;
   }
};
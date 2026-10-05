#pragma once

#include <cmsis_device.h>
#include <cstdint>

/**
 * STM32F1 ADC (v1 block): single, software-triggered conversions.
 *
 * Usage:
 *   Adc::enableClock();
 *   Adc::configureClock();   // ADCPRE so that ADCCLK <= 14 MHz
 *   Adc adc(ADC1);
 *   adc.enable();
 *   adc.calibrate();
 *   adc.setSamplingTime(Adc::Channel::PB0, Adc::SamplingCycles::Cycles55_5);
 *   uint16_t raw;
 *   if(adc.convert(Adc::Channel::PB0, raw)) { ... }
 *
 * This is not the interface of the Cortex-M0+ families. Their ADC has a
 * different register layout (CFGR1/CHSELR/SMPR groups).
 */
struct Adc {
   /// Sample time in ADCCLK cycles. The channel sample time is 3 bits per channel.
   enum class SamplingCycles : uint32_t {
      Cycles1_5 = 0,
      Cycles7_5,
      Cycles13_5,
      Cycles28_5,
      Cycles41_5,
      Cycles55_5,
      Cycles71_5,
      Cycles239_5,
   };

   /// ADC1 input channels of STM32F103 (medium density).
   enum Channel : uint8_t {
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
      TempSensor = 16,
      Vrefint = 17,
   };

   /// Maximum ADC clock of the F1 ADC.
   static constexpr uint32_t maxClockHz = 14000000;
   /// Typical internal reference voltage in mV. The F1 has no factory calibration value.
   static constexpr uint32_t vrefintTypicalMv = 1200;
   /// Highest raw value of a 12-bit conversion.
   static constexpr uint32_t maxRaw = 4095;

   ADC_TypeDef &_adc;

   explicit Adc(ADC_TypeDef *adc)
      : _adc(*adc) {
   }

   Adc(const Adc &) = default;
   Adc(Adc &&) = default;
   Adc &operator=(const Adc &) = delete;
   Adc &operator=(Adc &&) = delete;

   /// Enable the ADC1 bus clock (APB2).
   static void enableClock() {
      RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
   }

   static void disableClock() {
      RCC->APB2ENR &= ~RCC_APB2ENR_ADC1EN;
   }

   /// Reset ADC1 through RCC.
   static void reset() {
      RCC->APB2RSTR |= RCC_APB2RSTR_ADC1RST;
      RCC->APB2RSTR &= ~RCC_APB2RSTR_ADC1RST;
   }

   /**
    * Set ADCPRE (RCC_CFGR) to the smallest divider (2, 4, 6 or 8) that keeps
    * ADCCLK at or below 14 MHz. The calculation uses SystemCoreClock and the
    * AHB/APB2 prescalers in RCC_CFGR. Call it again after a clock change.
    * No conversion may run while it changes.
    *
    * @return The resulting ADCCLK in Hz.
    */
   static uint32_t configureClock();

   /// Current ADCCLK in Hz, from SystemCoreClock and RCC_CFGR.
   static uint32_t clockHz();

   /// Switch the temperature sensor and Vrefint on (TSVREFE). ADC1 only.
   void enableInternalChannels() const {
      _adc.CR2 |= ADC_CR2_TSVREFE;
   }

   void disableInternalChannels() const {
      _adc.CR2 &= ~ADC_CR2_TSVREFE;
   }

   /// Power on the ADC (ADON) and wait for the stabilization time. Does nothing if it is on.
   void enable() const;

   /// Power the ADC down.
   void disable() const;

   /// Reset and run the calibration. The ADC must be enabled and must not convert.
   void calibrate() const;

   /// Set the sample time of one channel. Do not call it during a conversion.
   void setSamplingTime(Channel channel, SamplingCycles cycles) const;

   /**
    * One blocking, software-triggered conversion of a single channel, right aligned.
    *
    * @param channel The channel to convert.
    * @param value Receives the 12-bit result when the function returns true.
    * @return false if the conversion did not finish in a bounded time.
    */
   bool convert(Channel channel, uint16_t &value) const;

   /**
    * Convert Vrefint and calculate the supply voltage (VDDA) in mV from it.
    * It switches the internal channels on and uses the longest sample time (the
    * Vrefint input needs at least 17.1 us). The reference is the typical
    * 1.20 V, which has a spread of about +-3 % on the F1 (datasheet: 1.16 to
    * 1.24 V), so VDDA has the same relative error.
    *
    * @param vddaMv Receives VDDA in mV when the function returns true.
    * @return false if the conversion failed or returned 0.
    */
   bool readSupplyMillivolts(uint32_t &vddaMv) const;

   /// Convert a raw value to mV for a known reference (VDDA) in mV.
   static constexpr uint32_t toMillivolts(uint16_t raw, uint32_t vddaMv) {
      return (static_cast<uint32_t>(raw) * vddaMv + maxRaw / 2) / maxRaw;
   }

   /// VDDA in mV from a raw Vrefint reading (typical Vrefint).
   static constexpr uint32_t supplyFromVrefint(uint16_t rawVrefint) {
      return rawVrefint == 0 ? 0 : (vrefintTypicalMv * maxRaw + rawVrefint / 2) / rawVrefint;
   }
};

#pragma once

#include <cmsis_device.h>
#include <cstdint>

/**
 * STM32H5 ADC (RM0481 section 28): 12-bit SAR ADC with a regular group of
 * up to 16 ranks (SQR1..SQR4), an injected group of up to 4 ranks with a
 * context queue (JSQR), a sample time per channel, single-ended and
 * differential inputs, four offsets, three analog watchdogs, oversampling
 * and a dual mode for ADC1 + ADC2. This is the L4/G4 type of ADC, not the
 * G0 type (no CHSELR, no sample time groups), and not the F1 type.
 *
 * Usage (one software-triggered conversion):
 *   Adc::enableClock();
 *   Adc::setClockMode(Adc::ClockMode::HclkDiv4);   // 62.5 MHz at 250 MHz
 *   Adc adc(ADC1);
 *   if(!adc.powerUp() || !adc.calibrate() || !adc.enable()) { ... }
 *   adc.setSamplingTime(Adc::PB0, Adc::SamplingCycles::Cycles247_5);
 *   uint16_t raw;
 *   if(adc.convert(Adc::PB0, raw)) { ... }
 *
 * Write rules of the hardware (RM0481 section 28.4.10). The class does not
 * check them:
 * - ADC disabled (ADEN = 0): DIFSEL, the DEEPPWD/ADVREGEN bits, CALFACT
 *   write-back aside, and the common register CKMODE/PRESC/DUAL.
 * - No regular conversion (ADSTART = 0): CFGR regular fields, SQRx, SMPRx,
 *   CFGR2, TRx, AWDxCR, OFRx (also JADSTART = 0 for most of them).
 * - No injected conversion (JADSTART = 0): JSQR is a queue, see
 *   setInjectedSequence().
 *
 * Bounded waits: all functions that wait for the hardware return false
 * after `pollLimit` iterations instead of hanging.
 */
struct Adc {
   // ---------------------------------------------------------------------
   // Channels
   // ---------------------------------------------------------------------
   /// ADC1 inputs (VINP) of the STM32H533 (DS14539 table 14). PF11/PF12 exist
   /// only in the 100/144-pin packages. ADC2 has the same pins, except
   /// channel 2 (PF13) and channel 6 (PF14). On ADC2, channel 16 is VBAT/4
   /// and channel 17 is VDDCORE (needs ADC_OR.OP0 on ADC2).
   enum Channel : uint8_t {
      PA0 = 0, // needs ADC_OR.OP0: setChannel0Switch(), also for VINN1 (differential channel 1)
      PA1 = 1,
      PF11 = 2,
      PA6 = 3,
      PC4 = 4,
      PB1 = 5,
      PF12 = 6,
      PA7 = 7,
      PC5 = 8,
      PB0 = 9,
      PC0 = 10,
      PC1 = 11,
      PC2 = 12,
      PC3 = 13,
      PA2 = 14,
      PA3 = 15,
      PA4 = 18,
      PA5 = 19,
      Vsense = 16,   // ADC1: temperature sensor, enableTemperatureSensor()
      Vrefint = 17,  // ADC1: internal reference, enableVrefint()
      VbatDiv4 = 16, // ADC2: VBAT / 4, enableVbat()
      Vddcore = 17,  // ADC2: digital core supply, needs ADC2 ADC_OR.OP0
   };
   static constexpr uint8_t channelCount = 20;

   // ---------------------------------------------------------------------
   // Clock
   // ---------------------------------------------------------------------
   /// RCC_CCIPR5.ADCDACSEL: the asynchronous kernel clock (ADC and DAC).
   enum class KernelClock : uint32_t {
      Hclk = 0,
      Sysclk = 1,
      Pll2R = 2,
      Hse = 3,
      HsiKer = 4,
      CsiKer = 5,
   };

   /// ADC_CCR.CKMODE. Async uses the kernel clock and PRESC. HclkDiv1 needs
   /// HPRE = /1 (RM0481 section 28.4.3).
   enum class ClockMode : uint32_t {
      Async = 0,
      HclkDiv1 = ADC_CCR_CKMODE_0,
      HclkDiv2 = ADC_CCR_CKMODE_1,
      HclkDiv4 = ADC_CCR_CKMODE_1 | ADC_CCR_CKMODE_0,
   };

   /// ADC_CCR.PRESC, only with ClockMode::Async.
   enum class Prescaler : uint32_t {
      Div1 = 0,
      Div2,
      Div4,
      Div6,
      Div8,
      Div10,
      Div12,
      Div16,
      Div32,
      Div64,
      Div128,
      Div256,
   };

   /// Highest ADC clock (fadc_ker_ck, DS14539 table 179), lowest ADC clock.
   static constexpr uint32_t maxClockHz = 75000000;
   static constexpr uint32_t minClockHz = 1500000;

   // ---------------------------------------------------------------------
   // Conversion settings
   // ---------------------------------------------------------------------
   /// ADC_SMPRx.SMPy: sample time in ADC clock cycles. Cycles2_5 becomes 3.5
   /// cycles with setSamplingTimePlusOne(true).
   enum class SamplingCycles : uint32_t {
      Cycles2_5 = 0,
      Cycles6_5,
      Cycles12_5,
      Cycles24_5,
      Cycles47_5,
      Cycles92_5,
      Cycles247_5,
      Cycles640_5,
   };

   enum class Resolution : uint32_t {
      Bits12 = 0,
      Bits10 = ADC_CFGR_RES_0,
      Bits8 = ADC_CFGR_RES_1,
      Bits6 = ADC_CFGR_RES_1 | ADC_CFGR_RES_0,
   };

   enum class Alignment : uint32_t { Right = 0, Left = ADC_CFGR_ALIGN };

   enum class TriggerEdge : uint32_t { Software = 0, Rising = 1, Falling = 2, Both = 3 };

   /// Regular group triggers, ADC_CFGR.EXTSEL (RM0481 table 282).
   enum class RegularTrigger : uint32_t {
      Tim1Oc1 = 0,
      Tim1Oc2 = 1,
      Tim1Oc3 = 2,
      Tim2Oc2 = 3,
      Tim3Trgo = 4,
      Tim4Oc4 = 5,
      Exti11 = 6,
      Tim8Trgo = 7,
      Tim8Trgo2 = 8,
      Tim1Trgo = 9,
      Tim1Trgo2 = 10,
      Tim2Trgo = 11,
      Tim4Trgo = 12,
      Tim6Trgo = 13,
      Tim15Trgo = 14,
      Tim3Oc4 = 15,
      Exti15 = 16,
      Lptim1Ch1 = 18,
      Lptim2Ch1 = 19,
   };

   /// Injected group triggers, ADC_JSQR.JEXTSEL (RM0481 table 282).
   enum class InjectedTrigger : uint32_t {
      Tim1Trgo = 0,
      Tim1Oc4 = 1,
      Tim2Trgo = 2,
      Tim2Oc1 = 3,
      Tim3Oc4 = 4,
      Tim4Trgo = 5,
      Exti15 = 6,
      Tim8Oc4 = 7,
      Tim1Trgo2 = 8,
      Tim8Trgo = 9,
      Tim8Trgo2 = 10,
      Tim3Oc3 = 11,
      Tim3Trgo = 12,
      Tim3Oc1 = 13,
      Tim6Trgo = 14,
      Tim15Trgo = 15,
      Lptim1Ch1 = 18,
      Lptim2Ch1 = 19,
   };

   /// Single: one sequence per trigger. Continuous: CONT = 1.
   /// Discontinuous: DISCEN = 1, `n` ranks per trigger (setDiscontinuousCount()).
   enum class ConversionMode : uint32_t { Single = 0, Continuous = ADC_CFGR_CONT, Discontinuous = ADC_CFGR_DISCEN };

   /// Preserve (reset value): DR is a 3-stage FIFO; on overrun the new
   /// result is lost and up to 3 old results stay unread in the FIFO.
   /// Overwrite: no FIFO, DR always holds the latest result. Erratum ES0621
   /// 2.6.6: dual regular simultaneous mode with DMA needs Overwrite (or one
   /// DMA channel per ADC).
   enum class OverrunMode : uint32_t { Preserve = 0, Overwrite = ADC_CFGR_OVRMOD };

   /// ADC_CFGR.DMAEN/DMACFG.
   enum class DmaMode : uint32_t { Off = 0, OneShot = ADC_CFGR_DMAEN, Circular = ADC_CFGR_DMAEN | ADC_CFGR_DMACFG };

   enum class InputMode : bool { SingleEnded = false, Differential = true };

   enum class OversamplingRatio : uint32_t { X2 = 0, X4, X8, X16, X32, X64, X128, X256 };

   /// ADC_CFGR2.ROVSM: Continued keeps the partial sum when an injected
   /// sequence interrupts the regular oversampling, Resumed restarts it.
   enum class RegularOversamplingMode : uint32_t { Continued = 0, Resumed = ADC_CFGR2_ROVSM };

   /// The four offset units (ADC_OFR1..4).
   enum class Offset : uint8_t { Ofs1 = 0, Ofs2, Ofs3, Ofs4 };

   enum class Watchdog : uint8_t { Awd1 = 1, Awd2, Awd3 };

   /// ADC_CCR.DUAL, ADC1 is the master, ADC2 the slave.
   enum class DualMode : uint32_t {
      Independent = 0,
      RegularSimultaneousInjectedSimultaneous = 1,
      RegularSimultaneousAlternateTrigger = 2,
      InterleavedInjectedSimultaneous = 3,
      InjectedSimultaneous = 5,
      RegularSimultaneous = 6,
      Interleaved = 7,
      AlternateTrigger = 9,
   };

   /// ADC_CCR.MDMA: data format of ADC_CDR in dual mode.
   enum class DualDmaMode : uint32_t { Off = 0, Bits32 = 2, Bits8 = 3 };

   // ---------------------------------------------------------------------
   // Status and interrupts: ADC_ISR and ADC_IER have the same bit positions.
   // ---------------------------------------------------------------------
   enum Event : uint32_t {
      Ready = ADC_ISR_ADRDY,
      EndOfSampling = ADC_ISR_EOSMP,
      EndOfConversion = ADC_ISR_EOC,
      EndOfSequence = ADC_ISR_EOS,
      Overrun = ADC_ISR_OVR,
      InjectedEndOfConversion = ADC_ISR_JEOC,
      InjectedEndOfSequence = ADC_ISR_JEOS,
      Watchdog1 = ADC_ISR_AWD1,
      Watchdog2 = ADC_ISR_AWD2,
      Watchdog3 = ADC_ISR_AWD3,
      InjectedQueueOverflow = ADC_ISR_JQOVF,
   };

   // ---------------------------------------------------------------------
   // Factory calibration (read-only area, DS14539 tables 26 and 96)
   // ---------------------------------------------------------------------
   struct Calibration {
      // The area faults on 8-bit loads (#44): load the aligned 32-bit word
      // and keep the low half-word (little endian).
      static uint16_t read(uint32_t address) {
         return static_cast<uint16_t>(*reinterpret_cast<const volatile uint32_t *>(address));
      }
      /// Vrefint raw value at 30 degC, VDDA = 3.3 V.
      static uint16_t vrefint() {
         return read(0x08FFF810UL);
      }
      /// Temperature sensor raw values at 30 and 130 degC, VDDA = 3.3 V.
      static uint16_t ts1() {
         return read(0x08FFF814UL);
      }
      static uint16_t ts2() {
         return read(0x08FFF818UL);
      }
      static constexpr int32_t ts1Celsius = 30;
      static constexpr int32_t ts2Celsius = 130;
      static constexpr uint32_t vddaMv = 3300;
   };
   /// Compatible name for libArT's InternalTemperatureSensor.
   struct TemperatureSensorCalibration {
      static uint16_t cal1() {
         return Calibration::ts1();
      }
      static uint16_t cal2() {
         return Calibration::ts2();
      }
      static constexpr int32_t temp1 = Calibration::ts1Celsius;
      static constexpr int32_t temp2 = Calibration::ts2Celsius;
      static constexpr uint32_t vrefAnalog = Calibration::vddaMv;
   };

   static constexpr uint32_t maxRaw = 4095;
   /// Minimum sample time of Vrefint (DS14539: 4.3 us) and of the
   /// temperature sensor (9 us). After enableTemperatureSensor(), wait
   /// 25.2 us (sensor buffer start-up) before the first Vsense conversion.
   static constexpr uint32_t vrefintSampleNs = 4300;
   static constexpr uint32_t vsenseSampleNs = 9000;
   /// Bound of all poll loops: far more than a 640.5-cycle conversion at 1.5 MHz.
   static constexpr uint32_t pollLimit = 1000000;

   ADC_TypeDef &_adc;

   /// The ADC_CR bits of type rs (ADEN, ADDIS, ADSTART, JADSTART, ADSTP,
   /// JADSTP, ADCAL). A read-modify-write must write them as 0, or it starts
   /// a conversion that just ended again (ADC_CR_BITS_PROPERTY_RS in ST's HAL).
   static constexpr uint32_t crCommandBits =
   ADC_CR_ADEN | ADC_CR_ADDIS | ADC_CR_ADSTART | ADC_CR_JADSTART | ADC_CR_ADSTP | ADC_CR_JADSTP | ADC_CR_ADCAL;
   /// Sets one rs bit of ADC_CR.
   void command(uint32_t bit) const {
      _adc.CR = (_adc.CR & ~crCommandBits) | bit;
   }
   /// Writes the configuration bits `mask` of ADC_CR (DEEPPWD, ADVREGEN, ADCALDIF).
   void writeCrConfig(uint32_t mask, uint32_t value) const {
      _adc.CR = (_adc.CR & ~(crCommandBits | mask)) | value;
   }

   explicit Adc(ADC_TypeDef *adc)
      : _adc(*adc) {
   }

   Adc(const Adc &) = default;
   Adc(Adc &&) = default;
   Adc &operator=(const Adc &) = delete;
   Adc &operator=(Adc &&) = delete;

   // ---------------------------------------------------------------------
   // RCC and the common registers (ADC1 and ADC2 share them)
   // ---------------------------------------------------------------------
   static void enableClock() {
      RCC->AHB2ENR |= RCC_AHB2ENR_ADCEN;
      [[maybe_unused]] const uint32_t readBack = RCC->AHB2ENR;
   }
   static void disableClock() {
      RCC->AHB2ENR &= ~RCC_AHB2ENR_ADCEN;
   }
   /// Resets ADC1 and ADC2.
   static void reset() {
      RCC->AHB2RSTR |= RCC_AHB2RSTR_ADCRST;
      RCC->AHB2RSTR &= ~RCC_AHB2RSTR_ADCRST;
   }
   static void setKernelClock(KernelClock source) {
      RCC->CCIPR5 = (RCC->CCIPR5 & ~RCC_CCIPR5_ADCDACSEL) | static_cast<uint32_t>(source);
   }
   static KernelClock getKernelClock() {
      return static_cast<KernelClock>(RCC->CCIPR5 & RCC_CCIPR5_ADCDACSEL);
   }
   /// Both ADCs disabled.
   static void setClockMode(ClockMode mode, Prescaler prescaler = Prescaler::Div1) {
      ADC12_COMMON->CCR = (ADC12_COMMON->CCR & ~(ADC_CCR_CKMODE | ADC_CCR_PRESC)) | static_cast<uint32_t>(mode) |
                          (static_cast<uint32_t>(prescaler) << ADC_CCR_PRESC_Pos);
   }
   /**
    * The ADC clock in Hz from CKMODE, PRESC and the kernel clock mux. 0 for
    * a kernel clock that system-stm32 does not start (PLL2R) and for CSI or
    * HSE when unknown. It uses SystemCoreClock (hclk on the H5).
    */
   static uint32_t clockHz();

   /**
    * Selects the fastest synchronous clock (hclk / 1, 2 or 4) that does not
    * exceed maxClockHz. /1 only with HPRE = /1. Both ADCs disabled.
    * @return The ADC clock in Hz, or 0 if hclk / 4 is still too fast.
    */
   static uint32_t configureClock();

   static void enableVrefint() {
      ADC12_COMMON->CCR |= ADC_CCR_VREFEN;
   }
   static void disableVrefint() {
      ADC12_COMMON->CCR &= ~ADC_CCR_VREFEN;
   }
   static void enableTemperatureSensor() {
      ADC12_COMMON->CCR |= ADC_CCR_TSEN;
   }
   static void disableTemperatureSensor() {
      ADC12_COMMON->CCR &= ~ADC_CCR_TSEN;
   }
   static void enableVbat() {
      ADC12_COMMON->CCR |= ADC_CCR_VBATEN;
   }
   static void disableVbat() {
      ADC12_COMMON->CCR &= ~ADC_CCR_VBATEN;
   }

   /// Dual mode. Both ADCs disabled. `delayCycles` (0..15): ADC_CCR.DELAY,
   /// the interleaved delay in ADC clock cycles minus one.
   static void setDualMode(DualMode mode, DualDmaMode dma = DualDmaMode::Off, bool dmaCircular = false, uint8_t delayCycles = 0) {
      ADC12_COMMON->CCR = (ADC12_COMMON->CCR & ~(ADC_CCR_DUAL | ADC_CCR_DELAY | ADC_CCR_DMACFG | ADC_CCR_MDMA)) |
                          (static_cast<uint32_t>(mode) << ADC_CCR_DUAL_Pos) | ((delayCycles & 0xFU) << ADC_CCR_DELAY_Pos) |
                          (dmaCircular ? ADC_CCR_DMACFG : 0U) | (static_cast<uint32_t>(dma) << ADC_CCR_MDMA_Pos);
   }
   /// Dual mode data: master (ADC1) in bits 15:0, slave (ADC2) in bits 31:16.
   static uint32_t dualValue() {
      return ADC12_COMMON->CDR;
   }

   // ---------------------------------------------------------------------
   // Power, calibration, enable
   // ---------------------------------------------------------------------
   /**
    * Leaves deep power-down, starts the voltage regulator and waits
    * tADCVREG_STUP (10 us). Does nothing if the regulator is on.
    * @return false if the ADC is enabled (the bits are not writable then).
    */
   bool powerUp() const;

   /// Stops the regulator and enters deep power-down. Loses the calibration.
   /// @return false if the ADC does not disable in time.
   bool powerDown() const;

   /**
    * Runs the calibration for `mode`. Calibrate SingleEnded and
    * Differential separately if both are used. The ADC must be powered up.
    * If it is enabled, the function disables it and enables it again.
    * @return false on a timeout or without powerUp().
    */
   bool calibrate(InputMode mode = InputMode::SingleEnded) const;

   /// ADC_CALFACT: save after calibrate(), restore after a power cycle
   /// instead of a new calibration (ADEN = 1, no conversion).
   uint32_t getCalibrationFactors() const {
      return _adc.CALFACT & (ADC_CALFACT_CALFACT_S | ADC_CALFACT_CALFACT_D);
   }
   void setCalibrationFactors(uint32_t factors) const {
      _adc.CALFACT = factors & (ADC_CALFACT_CALFACT_S | ADC_CALFACT_CALFACT_D);
   }

   /// ADRDY procedure of RM0481 section 28.4.9. @return false on a timeout.
   bool enable() const;

   /// Stops regular and injected conversions, then ADDIS.
   /// @return false on a timeout.
   bool disable() const;

   bool isEnabled() const {
      return (_adc.CR & ADC_CR_ADEN) != 0;
   }

   // ---------------------------------------------------------------------
   // Channel setup
   // ---------------------------------------------------------------------
   void setSamplingTime(Channel channel, SamplingCycles cycles) const;

   /// ADC_SMPR1.SMPPLUS: Cycles2_5 becomes 3.5 cycles for all channels.
   void setSamplingTimePlusOne(bool on) const {
      _adc.SMPR1 = (_adc.SMPR1 & ~ADC_SMPR1_SMPPLUS) | (on ? ADC_SMPR1_SMPPLUS : 0U);
   }

   /// ADC_DIFSEL. ADC disabled. A differential channel i uses VINN[i] as the
   /// negative input; the pin of VINN[i] is then not usable as a channel.
   void setInputMode(Channel channel, InputMode mode) const {
      const uint32_t bit = 1UL << channel;
      _adc.DIFSEL = mode == InputMode::Differential ? (_adc.DIFSEL | bit) : (_adc.DIFSEL & ~bit);
   }

   /// ADC_OR.OP0. ADC1: the GPIO switch of VINP0/VINN1 (needed for PA0).
   /// ADC2: the VDDCORE channel.
   void setChannel0Switch(bool on) const {
      _adc.OR = (_adc.OR & ~ADC_OR_OP0) | (on ? ADC_OR_OP0 : 0U);
   }

   // ---------------------------------------------------------------------
   // Regular group
   // ---------------------------------------------------------------------
   /// Rank 1..16 of the regular sequence.
   void setRegularRank(uint8_t rank, Channel channel) const;

   /// Number of ranks, 1..16.
   void setRegularLength(uint8_t length) const {
      if(length == 0) {
         return;
      }
      _adc.SQR1 = (_adc.SQR1 & ~ADC_SQR1_L) | (static_cast<uint32_t>((length - 1U) & 0xFU) << ADC_SQR1_L_Pos);
   }

   void setResolution(Resolution resolution) const {
      _adc.CFGR = (_adc.CFGR & ~ADC_CFGR_RES) | static_cast<uint32_t>(resolution);
   }
   void setAlignment(Alignment alignment) const {
      _adc.CFGR = (_adc.CFGR & ~ADC_CFGR_ALIGN) | static_cast<uint32_t>(alignment);
   }
   void setConversionMode(ConversionMode mode) const {
      _adc.CFGR = (_adc.CFGR & ~(ADC_CFGR_CONT | ADC_CFGR_DISCEN)) | static_cast<uint32_t>(mode);
   }
   /// Ranks per trigger in ConversionMode::Discontinuous, 1..8.
   void setDiscontinuousCount(uint8_t ranks) const {
      if(ranks == 0) {
         return;
      }
      _adc.CFGR = (_adc.CFGR & ~ADC_CFGR_DISCNUM) | (static_cast<uint32_t>((ranks - 1U) & 0x7U) << ADC_CFGR_DISCNUM_Pos);
   }
   void setOverrunMode(OverrunMode mode) const {
      _adc.CFGR = (_adc.CFGR & ~ADC_CFGR_OVRMOD) | static_cast<uint32_t>(mode);
   }
   void setDmaMode(DmaMode mode) const {
      _adc.CFGR = (_adc.CFGR & ~(ADC_CFGR_DMAEN | ADC_CFGR_DMACFG)) | static_cast<uint32_t>(mode);
   }
   /// AUTDLY: the next conversion waits until DR (or JDRx) is read.
   void setAutoDelay(bool on) const {
      _adc.CFGR = (_adc.CFGR & ~ADC_CFGR_AUTDLY) | (on ? ADC_CFGR_AUTDLY : 0U);
   }
   /// TriggerEdge::Software: start() starts the conversion.
   void setRegularTrigger(RegularTrigger trigger, TriggerEdge edge) const {
      _adc.CFGR = (_adc.CFGR & ~(ADC_CFGR_EXTSEL | ADC_CFGR_EXTEN)) | (static_cast<uint32_t>(trigger) << ADC_CFGR_EXTSEL_Pos) |
                  (static_cast<uint32_t>(edge) << ADC_CFGR_EXTEN_Pos);
   }
   void setSoftwareTrigger() const {
      _adc.CFGR &= ~ADC_CFGR_EXTEN;
   }
   /// ADC_CFGR2.BULB: sampling starts directly after the previous conversion.
   void setBulbSampling(bool on) const {
      _adc.CFGR2 = (_adc.CFGR2 & ~ADC_CFGR2_BULB) | (on ? ADC_CFGR2_BULB : 0U);
   }
   /// ADC_CFGR2.SMPTRIG: the trigger rising edge starts the sampling, the
   /// falling edge the conversion. Needs TriggerEdge::Rising or the
   /// software trigger with setSamplingSoftwareTrigger(). Not with BULB.
   void setSamplingTimeControlTrigger(bool on) const {
      _adc.CFGR2 = (_adc.CFGR2 & ~ADC_CFGR2_SMPTRIG) | (on ? ADC_CFGR2_SMPTRIG : 0U);
   }
   /// ADC_CFGR2.SWTRIG with SMPTRIG and the software trigger: true starts the
   /// sampling, false starts the conversion.
   void setSamplingSoftwareTrigger(bool sample) const {
      _adc.CFGR2 = (_adc.CFGR2 & ~ADC_CFGR2_SWTRIG) | (sample ? ADC_CFGR2_SWTRIG : 0U);
   }

   void start() const {
      command(ADC_CR_ADSTART);
   }
   /// @return false on a timeout.
   bool stop() const;
   bool isConverting() const {
      return (_adc.CR & ADC_CR_ADSTART) != 0;
   }
   uint16_t value() const {
      return static_cast<uint16_t>(_adc.DR);
   }
   /// DR address for a DMA transfer.
   const volatile uint32_t *dataRegister() const {
      return &_adc.DR;
   }

   /**
    * One blocking, software-triggered conversion of `channel` as a regular
    * sequence of length 1. It changes SQR1 and the trigger; it keeps the
    * other settings (resolution, alignment, offsets, oversampling).
    * Needs an enabled ADC and no ongoing regular conversion.
    * @return false on a timeout or if the ADC is not enabled.
    */
   bool convert(Channel channel, uint16_t &value) const;

   /**
    * VDDA in mV from Vrefint and its factory value (VDDA = 3.3 V *
    * VREFINT_CAL / raw). ADC1 only, 12-bit, right aligned, no offset.
    * Switches Vrefint on and sets a sample time of at least 5 us for
    * channel 17 from clockHz().
    * @return false if the conversion failed or returned 0.
    */
   bool readSupplyMillivolts(uint32_t &vddaMv) const;

   /// Raw value (12 bit) to mV for a known VDDA in mV.
   static constexpr uint32_t toMillivolts(uint16_t raw, uint32_t vddaMv) {
      return (static_cast<uint32_t>(raw) * vddaMv + maxRaw / 2) / maxRaw;
   }
   /// VDDA in mV from a raw Vrefint value and the factory value.
   static constexpr uint32_t supplyFromVrefint(uint16_t rawVrefint, uint16_t vrefintCal) {
      return rawVrefint == 0 ? 0 : (Calibration::vddaMv * vrefintCal + rawVrefint / 2) / rawVrefint;
   }

   // ---------------------------------------------------------------------
   // Injected group
   // ---------------------------------------------------------------------
   /**
    * Writes ADC_JSQR: 1..4 channels and the trigger in one write. With the
    * queue on (setInjectedQueue()), each write adds a context to a 2-entry
    * queue; see the STM32H5 errata ES0621 2.6.1 and 2.6.2 for the queue.
    * `channels` points to `count` channels.
    * @return false (and no write) for a count outside 1..4, or for
    * TriggerEdge::Software with the queue on: then JEXTEN = 00 also
    * disables the software trigger (RM0481 section 28.6.16).
    */
   bool setInjectedSequence(const Channel *channels,
                            uint8_t count,
                            InjectedTrigger trigger = InjectedTrigger::Tim1Trgo,
                            TriggerEdge edge = TriggerEdge::Software) const;

   /// queue = false: JQDIS = 1 (reset value), one context. emptyStops = JQM.
   /// With the queue on, the injected group needs a hardware trigger.
   void setInjectedQueue(bool queue, bool emptyStops = false) const {
      _adc.CFGR =
      (_adc.CFGR & ~(ADC_CFGR_JQDIS | ADC_CFGR_JQM)) | (queue ? 0U : ADC_CFGR_JQDIS) | (emptyStops ? ADC_CFGR_JQM : 0U);
   }
   /// JAUTO: the injected group runs after each regular group.
   void setInjectedAuto(bool on) const {
      _adc.CFGR = (_adc.CFGR & ~ADC_CFGR_JAUTO) | (on ? ADC_CFGR_JAUTO : 0U);
   }
   /// JDISCEN: one injected rank per trigger.
   void setInjectedDiscontinuous(bool on) const {
      _adc.CFGR = (_adc.CFGR & ~ADC_CFGR_JDISCEN) | (on ? ADC_CFGR_JDISCEN : 0U);
   }
   void startInjected() const {
      command(ADC_CR_JADSTART);
   }
   bool isInjectedConverting() const {
      return (_adc.CR & ADC_CR_JADSTART) != 0;
   }
   /// @return false on a timeout. Erratum ES0621 2.6.5: wait for JEOS first.
   bool stopInjected() const;
   /// Rank 1..4. Unsigned, as value() (an oversampled result can use all
   /// 16 bits). With an offset and no saturation the result is signed:
   /// cast it to int16_t.
   uint16_t injectedValue(uint8_t rank) const;

   // ---------------------------------------------------------------------
   // Offsets
   // ---------------------------------------------------------------------
   /**
    * Subtracts (or adds, `add`) `value` (12 bit) from each conversion of
    * `channel`. `saturate` clamps the result to 0..4095 instead of a signed
    * result. Regular and injected conversions.
    */
   void setOffset(Offset unit, Channel channel, uint16_t value, bool add = false, bool saturate = false) const;
   void disableOffset(Offset unit) const;

   // ---------------------------------------------------------------------
   // Oversampling
   // ---------------------------------------------------------------------
   /**
    * ratio and shift (0..8 bits) apply to both groups.
    * `triggered`: TROVS, each oversampled conversion needs its own trigger.
    */
   void setOversampling(OversamplingRatio ratio,
                        uint8_t shift,
                        bool regular,
                        bool injected,
                        bool triggered = false,
                        RegularOversamplingMode mode = RegularOversamplingMode::Continued) const;
   void disableOversampling() const {
      _adc.CFGR2 &= ~(ADC_CFGR2_ROVSE | ADC_CFGR2_JOVSE);
   }

   // ---------------------------------------------------------------------
   // Analog watchdogs
   // ---------------------------------------------------------------------
   /**
    * AWD1: one channel (`allChannels` = false) or all channels, regular
    * and/or injected group. Thresholds are 12 bit. `filter` 0..7: the number
    * of consecutive out-of-window results minus one before AWD1 is set.
    */
   void setWatchdog1(Channel channel, bool allChannels, bool regular, bool injected, uint16_t low, uint16_t high, uint8_t filter = 0) const;
   void disableWatchdog1() const {
      _adc.CFGR &= ~(ADC_CFGR_AWD1EN | ADC_CFGR_JAWD1EN);
   }
   /**
    * AWD2 or AWD3: a set of channels (bit i = channel i, 0 = off), both
    * groups. Thresholds compare with the 8 most significant bits.
    */
   void setWatchdog23(Watchdog watchdog, uint32_t channelMask, uint8_t low, uint8_t high) const;

   // ---------------------------------------------------------------------
   // Flags and interrupts
   // ---------------------------------------------------------------------
   bool isSet(Event event) const {
      return (_adc.ISR & event) != 0;
   }
   /// Write-1-to-clear: clears exactly the given events.
   void clear(uint32_t events) const {
      _adc.ISR = events;
   }
   void enableInterrupts(uint32_t events) const {
      _adc.IER |= events;
   }
   void disableInterrupts(uint32_t events) const {
      _adc.IER &= ~events;
   }
};

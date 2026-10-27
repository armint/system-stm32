#include "Rtc.h"

namespace hw {

namespace {

// Timeout
constexpr uint32_t kTimeoutLoops = 100000u;

// RTC Write Protection Keys (RTC_WPR)
constexpr uint8_t kWriteProtectionKey1 = 0xCAu;
constexpr uint8_t kWriteProtectionKey2 = 0x53u;
constexpr uint8_t kWriteProtectionLockKey = 0xFFu;

// BCD Conversion
constexpr uint8_t kBcdTensShift = 4u;
constexpr uint8_t kBcdDigitMask = 0x0Fu;

// Calendar Prescalers (PRER)
// PREDIV_A = 127 gives f_async = 32768 / 128 = 256 Hz (or 32000 / 128 = 250 Hz)
constexpr uint32_t kPredivA = 127u;
#if defined(RTC_PRER_PREDIV_A_Pos)
constexpr uint32_t kPredivAPos = RTC_PRER_PREDIV_A_Pos;
#else
constexpr uint32_t kPredivAPos = 16u;
#endif

// Synchronous prescaler values to obtain 1 Hz ck_spre
constexpr uint32_t kPredivSLse = 255u; // (255 + 1) = 256 -> 256 Hz / 256 = 1 Hz
constexpr uint32_t kPredivSLsi = 249u; // (249 + 1) = 250 -> 250 Hz / 250 = 1 Hz
constexpr uint32_t kDefaultPredivS = 255u;

// Default Calendar Date: Year 00, Jan 01, Monday (WDU = 1)
constexpr uint32_t kDefaultDate = 0x00002101u;

// Clock Frequencies (Hz)
constexpr uint32_t kLseFrequencyHz = 32768u;
constexpr uint32_t kLsiFrequencyHz = 32000u;

// Wakeup Timer (WUT) Configuration
constexpr uint32_t kWutClockDivider = 16u;            // RTCCLK / 16
constexpr uint32_t kShortRangeMaxDurationMs = 32000u; // Max duration (ms) for RTCCLK/16 resolution
constexpr uint32_t kMaxWutrTicks = 0x10000u;          // Max period (ticks) representable by the
                                                      // 16-bit WUTR counter (0 to 0xFFFF, i.e.
                                                      // up to 0x10000 ticks)
constexpr uint32_t kMaxExtendedWutrTicks = 0x20000u;  // Max period (seconds) representable with the 2^16 WUTR offset
                                                      // added (17-bit range)
constexpr uint32_t kWutrExtendedOffset = 0x10000u;    // Offset WUCKSEL=110 adds to the WUTR counter value

// WUCKSEL clock selections in RTC_CR
constexpr uint32_t kWuckselRtcClkDiv16 = 0u; // 000: RTCCLK/16
#if defined(RTC_CR_WUCKSEL_2)
constexpr uint32_t kWuckselCkSpre = RTC_CR_WUCKSEL_2; // 100: ck_spre (1 Hz)
#else
constexpr uint32_t kWuckselCkSpre = (1u << 2u);
#endif
#if defined(RTC_CR_WUCKSEL_2) && defined(RTC_CR_WUCKSEL_1)
constexpr uint32_t kWuckselCkSpreExtended = RTC_CR_WUCKSEL_2 | RTC_CR_WUCKSEL_1; // 110: ck_spre with 2^16 added
#else
constexpr uint32_t kWuckselCkSpreExtended = (1u << 2u) | (1u << 1u);
#endif

// TR Register Bit Fields (Time Register)
constexpr uint32_t kTrHoursShift = 16u;
constexpr uint32_t kTrHoursMask = 0x3Fu;
constexpr uint32_t kTrMinutesShift = 8u;
constexpr uint32_t kTrMinutesMask = 0x7Fu;
constexpr uint32_t kTrSecondsMask = 0x7Fu;

// DR Register Bit Fields (Date Register)
constexpr uint32_t kDrYearsShift = 16u;
constexpr uint32_t kDrYearsMask = 0xFFu;
constexpr uint32_t kDrMonthShift = 8u;
constexpr uint32_t kDrMonthMask = 0x1Fu;
constexpr uint32_t kDrDateMask = 0x3Fu;

// Days elapsed before each month in a non-leap year
constexpr uint16_t kDaysBeforeMonth[12] = { 0u, 31u, 59u, 90u, 120u, 151u, 181u, 212u, 243u, 273u, 304u, 334u };

// Shadow Register Consistency
constexpr uint32_t kMaxShadowReadRetries = 5u;

// RCC RTCSEL clock source values (01: LSE)
constexpr uint32_t kRccRtcSelLse = 1u;

} // namespace

uint8_t Rtc::bcdToDec(uint8_t bcd) {
   return static_cast<uint8_t>((((bcd >> kBcdTensShift) & kBcdDigitMask) * 10u) + (bcd & kBcdDigitMask));
}

void Rtc::unlockWriteProtection() const {
#if defined(PWR_CR1_DBP)
   PWR->CR1 |= PWR_CR1_DBP;
#elif defined(PWR_CR_DBP)
   PWR->CR |= PWR_CR_DBP;
#elif defined(PWR_DBPCR_DBP)
   PWR->DBPCR |= PWR_DBPCR_DBP; // STM32H5
#endif
   rtc_.WPR = kWriteProtectionKey1;
   rtc_.WPR = kWriteProtectionKey2;
}

void Rtc::lockWriteProtection() const {
   rtc_.WPR = kWriteProtectionLockKey;
}

bool Rtc::isInitMode() const {
#if defined(RTC_ICSR_INITF)
   return (rtc_.ICSR & RTC_ICSR_INITF) != 0u;
#elif defined(RTC_ISR_INITF)
   return (rtc_.ISR & RTC_ISR_INITF) != 0u;
#else
   return false;
#endif
}

bool Rtc::isWakeupTimerWriteAllowed() const {
#if defined(RTC_ICSR_WUTWF)
   return (rtc_.ICSR & RTC_ICSR_WUTWF) != 0u;
#elif defined(RTC_ISR_WUTWF)
   return (rtc_.ISR & RTC_ISR_WUTWF) != 0u;
#else
   return false;
#endif
}

bool Rtc::initCalendar(ClockSource clockSource) const {
   unlockWriteProtection();

   // Enter initialization mode
#if defined(RTC_ICSR_INIT)
   rtc_.ICSR |= RTC_ICSR_INIT;
   uint32_t timeout = kTimeoutLoops;
   while(!(rtc_.ICSR & RTC_ICSR_INITF)) {
      if(--timeout == 0u) {
         lockWriteProtection();
         return false;
      }
   }
#elif defined(RTC_ISR_INIT)
   rtc_.ISR |= RTC_ISR_INIT;
   uint32_t timeout = kTimeoutLoops;
   while(!(rtc_.ISR & RTC_ISR_INITF)) {
      if(--timeout == 0u) {
         lockWriteProtection();
         return false;
      }
   }
#endif

   // Prescalers: PREDIV_A = 127, PREDIV_S = 255 (for LSE 32.768 kHz) or 249 (for
   // LSI 32.000 kHz)
   const uint32_t predivS = (clockSource == ClockSource::Lse) ? kPredivSLse : kPredivSLsi;
   rtc_.PRER = (kPredivA << kPredivAPos) | predivS;

   // Reset TR and DR registers
   rtc_.TR = 0u;
   rtc_.DR = kDefaultDate; // Year 00, Jan 01, Monday

   // Exit initialization mode
#if defined(RTC_ICSR_INIT)
   rtc_.ICSR &= ~RTC_ICSR_INIT;
#elif defined(RTC_ISR_INIT)
   rtc_.ISR &= ~RTC_ISR_INIT;
#endif

#if defined(RTC_CR_BYPSHAD)
   // Bypass shadow registers so that calendar and subsecond registers are read
   // directly from the live counters, avoiding frozen/stale data after low-power modes.
   rtc_.CR |= RTC_CR_BYPSHAD;
#endif

   lockWriteProtection();
   return true;
}

#if defined(RTC_CR_WUTE)
bool Rtc::armWakeup(uint32_t durationMs) const {
   unlockWriteProtection();

   // Disable Wakeup Timer to configure it
   rtc_.CR &= ~(RTC_CR_WUTE | RTC_CR_WUTIE);

   // Wait for WUTWF (Wakeup Timer Write Flag)
   uint32_t timeout = kTimeoutLoops;
   while(!isWakeupTimerWriteAllowed()) {
      if(--timeout == 0u) {
         lockWriteProtection();
         return false;
      }
   }

   const ClockSource cs = getClockSource();
   const uint32_t rtcFreq = (cs == ClockSource::Lse) ? kLseFrequencyHz : kLsiFrequencyHz;

   uint32_t wucksel = 0u;
   uint32_t wutr = 0u;

   if(durationMs <= kShortRangeMaxDurationMs) {
      // Short range: RTCCLK / 16 (~2048 Hz for LSE, 2000 Hz for LSI)
      // tick frequency = rtcFreq / 16 (2048 Hz or 2000 Hz)
      const uint32_t wutFreq = rtcFreq / kWutClockDivider;
      // Round up: ceil(durationMs * wutFreq / 1000)
      uint32_t ticks = static_cast<uint32_t>((static_cast<uint64_t>(durationMs) * wutFreq + 999u) / 1000u);
      if(ticks == 0u) {
         ticks = 1u;
      }
      if(ticks > kMaxWutrTicks) {
         ticks = kMaxWutrTicks;
      }
      wucksel = kWuckselRtcClkDiv16; // 000: RTCCLK/16
      wutr = ticks - 1u;
   } else {
      // Long range: ck_spre (1 Hz clock)
      // Round up to next whole second
      uint32_t seconds = (durationMs + 999u) / 1000u;
      if(seconds <= kMaxWutrTicks) {
         wucksel = kWuckselCkSpre; // 100: ck_spre (1 Hz), 0 to 65535 s
         wutr = seconds - 1u;
      } else {
         // Extended 17-bit range: adds 2^16 to WUTR counter value
         if(seconds > kMaxExtendedWutrTicks) {
            seconds = kMaxExtendedWutrTicks;
         }
         wucksel = kWuckselCkSpreExtended; // 110: ck_spre with 2^16 added
         wutr = seconds - kWutrExtendedOffset - 1u;
      }
   }

   rtc_.WUTR = wutr;

   // Set WUCKSEL in CR, preserve other config, enable WUTE and WUTIE
   rtc_.CR = (rtc_.CR & ~RTC_CR_WUCKSEL) | wucksel;

   clearWakeupFlag();

   rtc_.CR |= (RTC_CR_WUTE | RTC_CR_WUTIE);

   lockWriteProtection();
   return true;
}

void Rtc::disarmWakeup() const {
   unlockWriteProtection();
   rtc_.CR &= ~(RTC_CR_WUTE | RTC_CR_WUTIE);
   clearWakeupFlag();
   lockWriteProtection();
}
#endif

bool Rtc::isWakeupFlagSet() const {
#if defined(RTC_SR_WUTF)
   return (rtc_.SR & RTC_SR_WUTF) != 0u;
#elif defined(RTC_ISR_WUTF)
   return (rtc_.ISR & RTC_ISR_WUTF) != 0u;
#else
   return false;
#endif
}

void Rtc::clearWakeupFlag() const {
#if defined(RTC_SCR_CWUTF)
   rtc_.SCR = RTC_SCR_CWUTF;
#elif defined(RTC_ISR_WUTF)
   rtc_.ISR &= ~RTC_ISR_WUTF;
#endif
}

uint64_t Rtc::getSubSecondTimestamp() const {
   uint32_t ssr1 = 0u;
   uint32_t ssr2 = 0u;
   uint32_t tr = 0u;
   uint32_t dr = 0u;

   const uint32_t prer = rtc_.PRER;
   uint32_t prediv_s = prer & RTC_PRER_PREDIV_S;
   if(prediv_s == 0u) {
      prediv_s = kDefaultPredivS;
   }

   // Read SSR, TR, DR, SSR ensuring no second rollover occurred between reads
   for(uint32_t retry = 0u; retry < kMaxShadowReadRetries; ++retry) {
      ssr1 = rtc_.SSR;
      tr = rtc_.TR;
      dr = rtc_.DR; // Reading DR also unlocks shadow registers
      ssr2 = rtc_.SSR;
      if(ssr1 >= ssr2) {
         break; // Normal decrement within the same second
      }
   }

   uint32_t year = bcdToDec(static_cast<uint8_t>((dr >> kDrYearsShift) & kDrYearsMask));
   uint32_t month = bcdToDec(static_cast<uint8_t>((dr >> kDrMonthShift) & kDrMonthMask));
   uint32_t date = bcdToDec(static_cast<uint8_t>(dr & kDrDateMask));

   if(month < 1u || month > 12u) {
      month = 1u;
   }
   if(date < 1u || date > 31u) {
      date = 1u;
   }
   if(year > 99u) {
      year = 99u;
   }

   const bool isLeapYear = (year % 4u == 0u);
   const uint32_t leapYearsBefore = (year + 3u) / 4u;
   const uint32_t daysBeforeYear = year * 365u + leapYearsBefore;
   const uint32_t daysInYear = kDaysBeforeMonth[month - 1u] + (date - 1u) + ((isLeapYear && month > 2u) ? 1u : 0u);
   const uint64_t totalDays = daysBeforeYear + daysInYear;

   const uint32_t hours = bcdToDec(static_cast<uint8_t>((tr >> kTrHoursShift) & kTrHoursMask));
   const uint32_t minutes = bcdToDec(static_cast<uint8_t>((tr >> kTrMinutesShift) & kTrMinutesMask));
   const uint32_t seconds = bcdToDec(static_cast<uint8_t>(tr & kTrSecondsMask));

   const uint64_t totalSeconds =
   totalDays * 86400u + static_cast<uint64_t>(hours) * 3600u + static_cast<uint64_t>(minutes) * 60u + seconds;

   // Sub-second fraction: SSR decrements from prediv_s down to 0
   const uint32_t fractionMs = ((prediv_s - ssr2) * 1000u) / (prediv_s + 1u);

   return totalSeconds * 1000u + fractionMs;
}

Rtc::ClockSource Rtc::getClockSource() {
#if defined(RCC_BDCR_RTCSEL)
   const uint32_t sel = (RCC->BDCR & RCC_BDCR_RTCSEL) >> RCC_BDCR_RTCSEL_Pos;
   if(sel == kRccRtcSelLse) {
      return ClockSource::Lse;
   }
   return ClockSource::Lsi;
#elif defined(RCC_CSR_RTCSEL)
   const uint32_t sel = (RCC->CSR & RCC_CSR_RTCSEL) >> RCC_CSR_RTCSEL_Pos;
   if(sel == kRccRtcSelLse) {
      return ClockSource::Lse;
   }
   return ClockSource::Lsi;
#elif defined(RCC_CSR1_RTCSEL)
   const uint32_t sel = (RCC->CSR1 & RCC_CSR1_RTCSEL) >> RCC_CSR1_RTCSEL_Pos; // STM32C0
   if(sel == kRccRtcSelLse) {
      return ClockSource::Lse;
   }
   return ClockSource::Lsi;
#else
   return ClockSource::Lse;
#endif
}

} // namespace hw

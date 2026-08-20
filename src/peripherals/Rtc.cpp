#include "Rtc.h"

namespace hw {

namespace {
constexpr uint32_t kTimeoutLoops = 100000u;
}

uint8_t Rtc::bcdToDec(uint8_t bcd) {
  return static_cast<uint8_t>((((bcd >> 4u) & 0x0Fu) * 10u) + (bcd & 0x0Fu));
}

void Rtc::unlockWriteProtection() const {
#if defined(PWR_CR1_DBP)
  PWR->CR1 |= PWR_CR1_DBP;
#elif defined(PWR_CR_DBP)
  PWR->CR |= PWR_CR_DBP;
#endif
  rtc_.WPR = 0xCAu;
  rtc_.WPR = 0x53u;
}

void Rtc::lockWriteProtection() const { rtc_.WPR = 0xFFu; }

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
  while (!(rtc_.ICSR & RTC_ICSR_INITF)) {
    if (--timeout == 0u) {
      lockWriteProtection();
      return false;
    }
  }
#elif defined(RTC_ISR_INIT)
  rtc_.ISR |= RTC_ISR_INIT;
  uint32_t timeout = kTimeoutLoops;
  while (!(rtc_.ISR & RTC_ISR_INITF)) {
    if (--timeout == 0u) {
      lockWriteProtection();
      return false;
    }
  }
#endif

  // Prescalers: PREDIV_A = 127, PREDIV_S = 255 (for LSE 32.768 kHz) or 249 (for
  // LSI 32.000 kHz)
  if (clockSource == ClockSource::Lse) {
    rtc_.PRER = (127u << 16u) | 255u;
  } else {
    rtc_.PRER = (127u << 16u) | 249u;
  }

  // Reset TR and DR registers
  rtc_.TR = 0u;
  rtc_.DR = 0x00002101u; // Year 01, Jan 01, Monday

  // Exit initialization mode
#if defined(RTC_ICSR_INIT)
  rtc_.ICSR &= ~RTC_ICSR_INIT;
#elif defined(RTC_ISR_INIT)
  rtc_.ISR &= ~RTC_ISR_INIT;
#endif

  lockWriteProtection();
  return true;
}

bool Rtc::armWakeup(uint32_t durationMs) const {
  unlockWriteProtection();

  // Disable Wakeup Timer to configure it
  rtc_.CR &= ~(RTC_CR_WUTE | RTC_CR_WUTIE);

  // Wait for WUTWF (Wakeup Timer Write Flag)
  uint32_t timeout = kTimeoutLoops;
  while (!isWakeupTimerWriteAllowed()) {
    if (--timeout == 0u) {
      lockWriteProtection();
      return false;
    }
  }

  const ClockSource cs = getClockSource();
  const uint32_t rtcFreq = (cs == ClockSource::Lse) ? 32768u : 32000u;

  uint32_t wucksel = 0u;
  uint32_t wutr = 0u;

  if (durationMs <= 32000u) {
    // Short range: RTCCLK / 16 (~2048 Hz for LSE, 2000 Hz for LSI)
    // tick frequency = rtcFreq / 16 (2048 Hz or 2000 Hz)
    const uint32_t wutFreq = rtcFreq / 16u;
    // Round up: ceil(durationMs * wutFreq / 1000)
    uint32_t ticks = static_cast<uint32_t>(
        (static_cast<uint64_t>(durationMs) * wutFreq + 999u) / 1000u);
    if (ticks == 0u) {
      ticks = 1u;
    }
    if (ticks > 0x10000u) {
      ticks = 0x10000u;
    }
    wucksel = 0u; // 000: RTCCLK/16
    wutr = ticks - 1u;
  } else {
    // Long range: ck_spre (1 Hz clock)
    // Round up to next whole second
    uint32_t seconds = (durationMs + 999u) / 1000u;
    if (seconds <= 0x10000u) {
      wucksel = RTC_CR_WUCKSEL_2; // 100: ck_spre (1 Hz), 0 to 65535 s
      wutr = seconds - 1u;
    } else {
      // Extended 17-bit range: adds 2^16 to WUTR counter value
      if (seconds > 0x20000u) {
        seconds = 0x20000u;
      }
      wucksel =
          RTC_CR_WUCKSEL_2 | RTC_CR_WUCKSEL_1; // 110: ck_spre with 2^16 added
      wutr = seconds - 0x10000u - 1u;
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

  const uint32_t prer = rtc_.PRER;
  uint32_t prediv_s = prer & RTC_PRER_PREDIV_S;
  if (prediv_s == 0u) {
    prediv_s = 255u;
  }

  // Read SSR, TR, DR, SSR ensuring no second rollover occurred between reads
  for (uint32_t retry = 0u; retry < 5u; ++retry) {
    ssr1 = rtc_.SSR;
    tr = rtc_.TR;
    (void)rtc_.DR; // Read DR to unlock shadow registers
    ssr2 = rtc_.SSR;
    if (ssr1 >= ssr2) {
      break; // Normal decrement within the same second
    }
  }

  const uint32_t hours = bcdToDec(static_cast<uint8_t>((tr >> 16u) & 0x3Fu));
  const uint32_t minutes = bcdToDec(static_cast<uint8_t>((tr >> 8u) & 0x7Fu));
  const uint32_t seconds = bcdToDec(static_cast<uint8_t>(tr & 0x7Fu));

  const uint64_t totalSeconds = static_cast<uint64_t>(hours) * 3600u +
                                static_cast<uint64_t>(minutes) * 60u + seconds;

  // Sub-second fraction: SSR decrements from prediv_s down to 0
  const uint32_t fractionMs = ((prediv_s - ssr2) * 1000u) / (prediv_s + 1u);

  return totalSeconds * 1000u + fractionMs;
}

Rtc::ClockSource Rtc::getClockSource() {
#if defined(RCC_BDCR_RTCSEL)
  const uint32_t sel = (RCC->BDCR & RCC_BDCR_RTCSEL) >> RCC_BDCR_RTCSEL_Pos;
  if (sel == 1u) { // 01: LSE
    return ClockSource::Lse;
  }
  return ClockSource::Lsi;
#elif defined(RCC_CSR_RTCSEL)
  const uint32_t sel = (RCC->CSR & RCC_CSR_RTCSEL) >> RCC_CSR_RTCSEL_Pos;
  if (sel == 1u) { // 01: LSE
    return ClockSource::Lse;
  }
  return ClockSource::Lsi;
#else
  return ClockSource::Lse;
#endif
}

} // namespace hw

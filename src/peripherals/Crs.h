#pragma once

#include <cmsis_device.h>
#include <cstdint>

// Clock Recovery System (CRS): trims the HSI48 oscillator against a
// synchronisation signal, normally the 1 kHz USB start-of-frame (SOF).
// The register layout is the same on STM32L0 (RM0367 section 8),
// STM32G0B1/G0C1 (RM0444 section 6) and STM32H5 (RM0481 section 12). Only the TRIM field width differs
// (6 bits on L0 and H5, 7 bits on G0), which TrimMax/TrimDefault take from CMSIS.
//
// The caller enables the CRS bus clock (Rcc::enableCRSClock()) and HSI48
// (Rcc::enableHSI48()) first. Typical USB use:
//
//    Rcc::enableCRSClock();
//    hw::Crs::configure({});   // defaults: USB SOF, 48 MHz / 1 kHz
//    hw::Crs::enable();        // counter on, automatic trimming on
#if defined(CRS)

namespace hw::Crs {

enum class SyncSource : uint32_t {
   Gpio = 0,                   // CRS_SYNC pin
   Lse = CRS_CFGR_SYNCSRC_0,   // LSE oscillator output
   UsbSof = CRS_CFGR_SYNCSRC_1 // USB start-of-frame packets (reset default)
};

enum class SyncDivider : uint32_t {
   Div1 = 0,
   Div2 = 1UL << CRS_CFGR_SYNCDIV_Pos,
   Div4 = 2UL << CRS_CFGR_SYNCDIV_Pos,
   Div8 = 3UL << CRS_CFGR_SYNCDIV_Pos,
   Div16 = 4UL << CRS_CFGR_SYNCDIV_Pos,
   Div32 = 5UL << CRS_CFGR_SYNCDIV_Pos,
   Div64 = 6UL << CRS_CFGR_SYNCDIV_Pos,
   Div128 = 7UL << CRS_CFGR_SYNCDIV_Pos,
};

// RELOAD = fTARGET / fSYNC - 1 = 48 MHz / 1 kHz - 1 (reset value).
constexpr uint16_t ReloadUsbSof = 48000 - 1;
// FELIM = fTARGET / fSYNC * step / 2, rounded up. The HSI48 trimming step
// differs by family, so the default does too.
#if defined(STM32H5)
// H5: 0.175 % typical step (DS14539 table 43): 48000 * 0.175 % / 2 = 42.
// The maximum step of 0.25 % would give 60.
constexpr uint8_t FrequencyErrorLimitDefault = 42;
#else
// L0: 0.14 % typical step: 48000 * 0.14 % / 2 = 33.6 -> 34 (reset value).
// G0B1 has a smaller step (0.11 %, DS13560 table 45), so 34 is safe there too.
constexpr uint8_t FrequencyErrorLimitDefault = 34;
#endif

constexpr uint32_t TrimMax = CRS_CR_TRIM_Msk >> CRS_CR_TRIM_Pos;
// Middle of the trimming range: 32 on L0, 64 on G0 (reset value).
constexpr uint32_t TrimDefault = (TrimMax + 1) / 2;

struct Config {
   SyncSource source = SyncSource::UsbSof;
   SyncDivider divider = SyncDivider::Div1;
   bool fallingEdge = false;
   uint16_t reload = ReloadUsbSof;
   uint8_t frequencyErrorLimit = FrequencyErrorLimitDefault;
};

/**
 * @brief Writes CRS_CFGR.
 * @return false if the frequency error counter is enabled (CEN set). The
 * hardware write-protects CRS_CFGR while CEN is set; call disable() first.
 */
[[nodiscard]] inline bool configure(const Config &config) {
   if(CRS->CR & CRS_CR_CEN) {
      return false;
   }
   CRS->CFGR = static_cast<uint32_t>(config.source) | static_cast<uint32_t>(config.divider) |
               (config.fallingEdge ? CRS_CFGR_SYNCPOL : 0u) |
               (static_cast<uint32_t>(config.frequencyErrorLimit) << CRS_CFGR_FELIM_Pos) |
               (static_cast<uint32_t>(config.reload) << CRS_CFGR_RELOAD_Pos);
   return true;
}

/**
 * @brief Starts the frequency error counter (CEN) and, if autoTrim is true,
 * automatic trimming of HSI48 (AUTOTRIMEN).
 */
inline void enable(bool autoTrim = true) {
   uint32_t cr = CRS->CR | CRS_CR_CEN;
   if(autoTrim) {
      cr |= CRS_CR_AUTOTRIMEN;
   } else {
      cr &= ~CRS_CR_AUTOTRIMEN;
   }
   CRS->CR = cr;
}

/**
 * @brief Stops the frequency error counter and automatic trimming. The TRIM
 * value stays as it is.
 */
inline void disable() {
   CRS->CR &= ~(CRS_CR_CEN | CRS_CR_AUTOTRIMEN);
}

inline bool isEnabled() {
   return (CRS->CR & CRS_CR_CEN) != 0;
}

inline bool isAutoTrimEnabled() {
   return (CRS->CR & CRS_CR_AUTOTRIMEN) != 0;
}

/**
 * @brief Sets the HSI48 trimming value (0 .. TrimMax). The hardware ignores
 * the write while automatic trimming is enabled.
 */
inline void setTrim(uint32_t trim) {
   CRS->CR = (CRS->CR & ~CRS_CR_TRIM) | ((trim << CRS_CR_TRIM_Pos) & CRS_CR_TRIM);
}

inline uint32_t getTrim() {
   return (CRS->CR & CRS_CR_TRIM) >> CRS_CR_TRIM_Pos;
}

// Result of the last SYNC event, from the CRS_ISR flags.
enum class SyncStatus {
   None,    // no SYNC event and no error since the flags were last cleared
   Ok,      // SYNCOKF: frequency error below the warning limit
   Warning, // SYNCWARNF: frequency error above the warning limit, still trimmable
   Error,   // ERRF: SYNC error, missed SYNC or trimming overflow (see Flag)
};
// With CEN set and no SYNC signal (no USB SOF: before the first SOF, during
// suspend, after an unplug), the counter reaches the OUTRANGE limit
// (128 * FELIM) and the hardware sets SYNCMISS, so ERRF. This is expected and
// does not mean that HSI48 is broken. The flags are sticky: call clearFlags()
// before a new status is read, e.g. after a USB reset or resume.

// CRS_ISR flags. ERRF is the OR of SyncError, SyncMissed and TrimOverflow.
enum class Flag : uint32_t {
   SyncOk = CRS_ISR_SYNCOKF,
   SyncWarning = CRS_ISR_SYNCWARNF,
   Error = CRS_ISR_ERRF,
   ExpectedSync = CRS_ISR_ESYNCF,
   SyncError = CRS_ISR_SYNCERR,
   SyncMissed = CRS_ISR_SYNCMISS,
   TrimOverflow = CRS_ISR_TRIMOVF,
};

inline bool isFlagSet(Flag flag) {
   return (CRS->ISR & static_cast<uint32_t>(flag)) != 0;
}

/**
 * @brief Returns the worst status flagged since the flags were last cleared
 * (Error before Warning before Ok). Does not clear the flags. Error with
 * Flag::SyncMissed is normal while there is no SYNC signal (see SyncStatus).
 */
inline SyncStatus getSyncStatus() {
   uint32_t isr = CRS->ISR;
   if(isr & CRS_ISR_ERRF) {
      return SyncStatus::Error;
   }
   if(isr & CRS_ISR_SYNCWARNF) {
      return SyncStatus::Warning;
   }
   if(isr & CRS_ISR_SYNCOKF) {
      return SyncStatus::Ok;
   }
   return SyncStatus::None;
}

/**
 * @brief Clears all CRS_ISR flags. ERRC also clears SYNCERR, SYNCMISS and
 * TRIMOVF.
 */
inline void clearFlags() {
   CRS->ICR = CRS_ICR_SYNCOKC | CRS_ICR_SYNCWARNC | CRS_ICR_ERRC | CRS_ICR_ESYNCC;
}

/**
 * @brief Frequency error counter value latched at the last SYNC event
 * (FECAP), in HSI48 clock ticks.
 */
inline uint16_t getFrequencyErrorCapture() {
   return static_cast<uint16_t>((CRS->ISR & CRS_ISR_FECAP) >> CRS_ISR_FECAP_Pos);
}

/**
 * @brief FEDIR at the last SYNC event: true if HSI48 was below the target
 * frequency, false if it was above.
 */
inline bool isFrequencyBelowTarget() {
   return (CRS->ISR & CRS_ISR_FEDIR) != 0;
}

} // namespace hw::Crs

#endif

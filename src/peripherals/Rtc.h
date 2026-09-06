#pragma once

#include <cmsis_device.h>
#include <cstdint>

namespace hw {

/**
 * @brief Hardware Real-Time Clock (RTC) driver with calendar and Wakeup Timer
 * (WUT).
 *
 * Supports sub-second monotonic timestamp reading and wakeup timer arming
 * across STM32 families (STM32WL, STM32G0, STM32L0, STM32C0).
 */
class Rtc {
public:
  enum class ClockSource : uint8_t { Lse = 0, Lsi = 1 };

  RTC_TypeDef &rtc_;

  explicit Rtc(RTC_TypeDef *rtcBase = RTC) : rtc_(*rtcBase) {}

  Rtc(const Rtc &) = default;
  Rtc(Rtc &&) = default;
  Rtc &operator=(const Rtc &) = delete;
  Rtc &operator=(Rtc &&) = delete;

  /**
   * @brief Unlocks RTC write protection (WPR) and enables backup domain write
   * access.
   */
  void unlockWriteProtection() const;

  /**
   * @brief Re-locks RTC write protection (WPR).
   */
  void lockWriteProtection() const;

  /**
   * @brief Initializes the calendar prescalers and resets TR/DR registers.
   * @param clockSource Selected clock source (LSE 32.768 kHz or LSI 32 kHz).
   * @return true if initialization succeeded, false on timeout.
   */
  bool initCalendar(ClockSource clockSource) const;

  /**
   * @brief Arms the RTC Wakeup Timer for the requested duration.
   *
   * Automatically selects between RTCCLK/16 (~488 us resolution, <= 32 s) and
   * ck_spre (1 Hz, > 32 s) to support sleep durations up to ~36 hours.
   * Rounds up requested duration so the timer never fires earlier than
   * requested.
   *
   * @param durationMs Desired sleep duration in milliseconds.
   * @return true if timer was armed successfully, false on timeout.
   */
  bool armWakeup(uint32_t durationMs) const;

  /**
   * @brief Disarms and disables the Wakeup Timer and its interrupt.
   */
  void disarmWakeup() const;

  /**
   * @brief Checks if the Wakeup Timer flag (WUTF) is set.
   */
  bool isWakeupFlagSet() const;

  /**
   * @brief Clears the Wakeup Timer flag (WUTF).
   */
  void clearWakeupFlag() const;

  /**
   * @brief Reads the monotonic sub-second timestamp in milliseconds since
   * calendar reset.
   *
   * Composed from DR (year, month, date), TR (hours, minutes, seconds), and
   * SSR (subsecond downcounter), performing consistency checks across second
   * and day rollovers. Monotonic across midnight transitions.
   */
  uint64_t getSubSecondTimestamp() const;

  /**
   * @brief Returns the currently active RTC clock source configured in RCC.
   */
  static ClockSource getClockSource();

  /**
   * @brief Checks if the initialization mode flag (INITF) is active.
   */
  bool isInitMode() const;

  /**
   * @brief Checks if the Wakeup Timer write allowed flag (WUTWF) is active.
   */
  bool isWakeupTimerWriteAllowed() const;

private:
  static uint8_t bcdToDec(uint8_t bcd);
};

} // namespace hw

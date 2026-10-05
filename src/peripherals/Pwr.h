#pragma once

#include <cmsis_device.h>
#include <cstdint>

namespace hw::Pwr {

enum class StopMode : uint32_t {
   Stop0 = 0,
#if defined(PWR_CR1_LPMS)
   Stop1 = PWR_CR1_LPMS_0,
#if defined(STM32WL)
   Stop2 = PWR_CR1_LPMS_1,
#else
   Stop2 = Stop1,
#endif
#else
   Stop1 = 0,
   Stop2 = 0,
#endif
};

/**
 * @brief Enables write access to the backup domain (RTC registers, RTC backup
 * registers, and RCC backup domain control registers).
 */
inline void enableBackupDomainAccess() {
#if defined(PWR_CR1_DBP)
   PWR->CR1 |= PWR_CR1_DBP;
#elif defined(PWR_CR_DBP)
   PWR->CR |= PWR_CR_DBP;
#endif
}

/**
 * @brief Disables write access to the backup domain.
 */
inline void disableBackupDomainAccess() {
#if defined(PWR_CR1_DBP)
   PWR->CR1 &= ~PWR_CR1_DBP;
#elif defined(PWR_CR_DBP)
   PWR->CR &= ~PWR_CR_DBP;
#endif
}

/**
 * @brief Checks if write access to the backup domain is enabled.
 */
inline bool isBackupDomainAccessEnabled() {
#if defined(PWR_CR1_DBP)
   return (PWR->CR1 & PWR_CR1_DBP) != 0u;
#elif defined(PWR_CR_DBP)
   return (PWR->CR & PWR_CR_DBP) != 0u;
#else
   return false;
#endif
}

/**
 * @brief Sets the Stop mode variant (Stop 0, Stop 1, Stop 2).
 */
inline void setStopMode(StopMode mode) {
#if defined(PWR_CR1_LPMS)
   PWR->CR1 = (PWR->CR1 & ~PWR_CR1_LPMS) | static_cast<uint32_t>(mode);
#elif defined(PWR_CR_PDDS) && defined(PWR_CR_LPSDSR)
   // STM32L0 and similar
   (void)mode;
   PWR->CR &= ~(PWR_CR_PDDS | PWR_CR_LPSDSR);
#elif defined(PWR_CR_PDDS) && defined(PWR_CR_LPDS)
   // STM32F1: Stop with the voltage regulator in run mode (LPDS = 0)
   (void)mode;
   PWR->CR &= ~(PWR_CR_PDDS | PWR_CR_LPDS);
#endif
}

/**
 * @brief Enables Cortex-M deep sleep mode (SLEEPDEEP bit in SCB->SCR).
 */
inline void enableDeepSleep() {
#if defined(SCB_SCR_SLEEPDEEP_Msk)
   SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
#endif
}

/**
 * @brief Disables Cortex-M deep sleep mode (clears SLEEPDEEP bit in SCB->SCR).
 */
inline void disableDeepSleep() {
#if defined(SCB_SCR_SLEEPDEEP_Msk)
   SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
#endif
}

} // namespace hw::Pwr

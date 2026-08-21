#pragma once

#include <cmsis_device.h>
#include <cstdint>

namespace hw {

/**
 * @brief Power (PWR) controller peripheral wrapper.
 *
 * Provides backup domain access control and low-power mode configuration
 * across STM32 families (STM32WL, STM32G0, STM32L0, STM32C0).
 */
struct Pwr {
   enum class StopMode : uint32_t {
#if defined(PWR_CR1_LPMS)
      Stop0 = 0,
      Stop1 = PWR_CR1_LPMS_0,
      Stop2 = PWR_CR1_LPMS_1,
#endif
   };

   /**
    * @brief Enables write access to the backup domain (RTC registers, RTC backup
    * registers, and RCC backup domain control registers).
    */
   static inline void enableBackupDomainAccess() {
#if defined(PWR_CR1_DBP)
      PWR->CR1 |= PWR_CR1_DBP;
#elif defined(PWR_CR_DBP)
      PWR->CR |= PWR_CR_DBP;
#endif
   }

   /**
    * @brief Disables write access to the backup domain.
    */
   static inline void disableBackupDomainAccess() {
#if defined(PWR_CR1_DBP)
      PWR->CR1 &= ~PWR_CR1_DBP;
#elif defined(PWR_CR_DBP)
      PWR->CR &= ~PWR_CR_DBP;
#endif
   }

   /**
    * @brief Checks if write access to the backup domain is enabled.
    */
   static inline bool isBackupDomainAccessEnabled() {
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
   static inline void setStopMode(StopMode mode) {
#if defined(PWR_CR1_LPMS)
      PWR->CR1 = (PWR->CR1 & ~PWR_CR1_LPMS) | static_cast<uint32_t>(mode);
#endif
   }

   /**
    * @brief Enables Cortex-M deep sleep mode (SLEEPDEEP bit in SCB->SCR).
    */
   static inline void enableDeepSleep() {
#if defined(SCB_SCR_SLEEPDEEP_Msk)
      SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
#endif
   }

   /**
    * @brief Disables Cortex-M deep sleep mode (clears SLEEPDEEP bit in SCB->SCR).
    */
   static inline void disableDeepSleep() {
#if defined(SCB_SCR_SLEEPDEEP_Msk)
      SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
#endif
   }
};

} // namespace hw

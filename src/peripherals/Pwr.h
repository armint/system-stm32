#pragma once

#include <cmsis_device.h>
#include <cstdint>

namespace hw {

/**
 * @brief Power (PWR) controller peripheral wrapper.
 *
 * Provides backup domain access control across STM32 families
 * (STM32WL, STM32G0, STM32L0, STM32C0).
 */
struct Pwr {
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
};

} // namespace hw

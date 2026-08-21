#pragma once

#include <cmsis_device.h>
#include <cstdint>

namespace hw::SysTickTimer {

/**
 * @brief Enables the SysTick counter and interrupt.
 */
inline void enable() {
#if defined(SysTick_CTRL_ENABLE_Msk)
   SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
#endif
}

/**
 * @brief Disables the SysTick counter.
 */
inline void disable() {
#if defined(SysTick_CTRL_ENABLE_Msk)
   SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
#endif
}

/**
 * @brief Checks if the SysTick counter is enabled.
 */
inline bool isEnabled() {
#if defined(SysTick_CTRL_ENABLE_Msk)
   return (SysTick->CTRL & SysTick_CTRL_ENABLE_Msk) != 0u;
#else
   return false;
#endif
}

/**
 * @brief Resets the current SysTick counter value to 0.
 */
inline void reset() {
#if defined(SysTick)
   SysTick->VAL = 0u;
#endif
}

/**
 * @brief Reads the current SysTick counter value.
 */
inline uint32_t getValue() {
#if defined(SysTick)
   return SysTick->VAL;
#else
   return 0u;
#endif
}

} // namespace hw::SysTickTimer

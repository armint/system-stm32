#pragma once

#include <cmsis_device.h>

namespace hw::Dbg {

/**
 * @brief Freezes the Independent Watchdog (IWDG) counter when core is halted in debug mode.
 */
inline void freezeIwdgOnDebug() {
#if defined(DBGMCU_APB1_FZ_DBG_IWDG_STOP)
   DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_IWDG_STOP;
#elif defined(DBG_APB_FZ1_DBG_IWDG_STOP)
   DBG->APBFZ1 |= DBG_APB_FZ1_DBG_IWDG_STOP;
#endif
}

/**
 * @brief Freezes the Window Watchdog (WWDG) counter when core is halted in debug mode.
 */
inline void freezeWwdgOnDebug() {
#if defined(DBGMCU_APB1_FZ_DBG_WWDG_STOP)
   DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_WWDG_STOP;
#elif defined(DBG_APB_FZ1_DBG_WWDG_STOP)
   DBG->APBFZ1 |= DBG_APB_FZ1_DBG_WWDG_STOP;
#endif
}

} // namespace hw::Dbg

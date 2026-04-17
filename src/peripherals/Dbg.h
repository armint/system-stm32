#pragma once

#include <cmsis_device.h>

namespace hw {

/**
 * Debug MCU peripheral wrapper.
 *
 * Provides a uniform API across STM32 families that name the peripheral
 * differently (DBGMCU on L0, DBG on G0/C0).
 */
struct Dbg {
   static inline void freezeIwdgOnDebug() {
#if defined(DBGMCU_APB1_FZ_DBG_IWDG_STOP)
      DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_IWDG_STOP;
#elif defined(DBG_APB_FZ1_DBG_IWDG_STOP)
      DBG->APBFZ1 |= DBG_APB_FZ1_DBG_IWDG_STOP;
#endif
   }

   static inline void freezeWwdgOnDebug() {
#if defined(DBGMCU_APB1_FZ_DBG_WWDG_STOP)
      DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_WWDG_STOP;
#elif defined(DBG_APB_FZ1_DBG_WWDG_STOP)
      DBG->APBFZ1 |= DBG_APB_FZ1_DBG_WWDG_STOP;
#endif
   }
};

} // namespace hw

#pragma once

#include "peripherals/Pwr.h"
#include "peripherals/Rcc.h"
#include <cmsis_device.h>
#include <cstdint>

// STM32F1 backup registers (BKP_DRx, RM0008 section 6). The data registers
// keep their content over a system reset and, with VBAT supplied, over a
// power loss. They are 16 bits wide in a 32-bit slot. A typical use is a
// flag that survives NVIC_SystemReset(), e.g. "enter the bootloader".
namespace hw::Bkp {

/**
 * @brief Enables the PWR and BKP clocks and sets PWR_CR.DBP, so the backup
 * registers become writable. Reads only need the clocks; call this before
 * the first read or write.
 */
inline void enable() {
   Rcc::enablePWRClock();
   Rcc::enableBKPClock();
   hw::Pwr::enableBackupDomainAccess();
}

/**
 * @brief Reads backup data register 1.
 */
inline uint16_t readDR1() {
   return static_cast<uint16_t>(BKP->DR1 & BKP_DR1_D);
}

/**
 * @brief Writes backup data register 1. Needs enable() first.
 */
inline void writeDR1(uint16_t value) {
   BKP->DR1 = value;
}

} // namespace hw::Bkp

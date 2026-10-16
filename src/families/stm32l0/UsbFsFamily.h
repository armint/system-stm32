#pragma once

// STM32L0 access layer of the shared USB full-speed device driver
// (peripherals/usbfs/UsbDeviceCore). Only the differences to the other
// families are here. See RM0367 section 32.

#include <cmsis_device.h>
#include <cstdint>

#if defined(USB_BASE)

#define SYSTEM_STM32_USBFS 1

namespace hw::usbfs {

constexpr IRQn_Type Irq = USB_IRQn;

// 1024 bytes at USB_PMAADDR, byte addresses, 8- or 16-bit access only (no
// 32-bit access).
constexpr uint16_t PacketMemorySize = 1024;

inline volatile uint16_t &pma16(uint16_t offset) {
   return *reinterpret_cast<volatile uint16_t *>(USB_PMAADDR + offset);
}

constexpr uint16_t CntrLowPowerMode = USB_CNTR_LPMODE;

// Internal D+ pull-up: USB_BCDR.DPPU.
constexpr bool HasInternalPullup = true;

// BCD and the pull-up off, before the power-up sequence.
inline void beforePowerUp() {
   USB->BCDR = 0;
}

inline void setInternalPullup(bool on) {
   if(on) {
      USB->BCDR |= USB_BCDR_DPPU;
   } else {
      USB->BCDR &= static_cast<uint16_t>(~USB_BCDR_DPPU);
   }
}

inline bool internalPullup() {
   return (USB->BCDR & USB_BCDR_DPPU) != 0;
}

} // namespace hw::usbfs

#include "peripherals/usbfs/UsbFs16Access.h"

#endif

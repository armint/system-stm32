#pragma once

// STM32F1 access layer of the shared USB full-speed device driver
// (peripherals/usbfs/UsbDeviceCore). Only the differences to the other
// families are here. See RM0008 section 23.

#include <cmsis_device.h>
#include <cstdint>

#if defined(USB_BASE)

namespace hw::usbfs {

// The packet memory is shared with bxCAN, so do not use CAN at the same
// time.
constexpr IRQn_Type Irq = USB_LP_CAN1_RX0_IRQn;

// 512 bytes, addressed by the USB peripheral with byte offsets. The CPU sees
// them as 256 16-bit words at a 32-bit stride: USB offset o (even) is at
// USB_PMAADDR + o * 2, and the upper half of each 32-bit slot is unused.
constexpr uint16_t PacketMemorySize = 512;

inline volatile uint16_t &pma16(uint16_t offset) {
   return *reinterpret_cast<volatile uint16_t *>(USB_PMAADDR + offset * 2u);
}

constexpr uint16_t CntrLowPowerMode = USB_CNTR_LP_MODE;

// No USB_BCDR, so no internal D+ pull-up: the board switches an external
// pull-up through UsbDevice::setPullupCallback().
constexpr bool HasInternalPullup = false;

inline void resetBcdr() {}
inline void setInternalPullup(bool) {}
inline bool internalPullup() {
   return false;
}

} // namespace hw::usbfs

#endif

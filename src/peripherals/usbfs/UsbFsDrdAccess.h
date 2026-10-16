#pragma once

// Access functions of the shared USB full-speed device driver for the USB
// DRD block in Device mode (STM32H5: RM0481 section 61; the STM32G0B1 and
// the STM32U5 have the same block): 32-bit USB_CHEPnR registers, no
// USB_BTABLE register, 32-bit buffer descriptors, and 32-bit packet memory
// access only. The family access layer (UsbFsFamily.h) includes this header
// and adds the IRQ, the power-up and the errata of the family.

#include <cmsis_device.h>
#include <cstdint>

namespace hw::usbfs {

constexpr uint16_t PacketMemorySize = USB_DRD_PMA_SIZE;

constexpr uint16_t BufferAlignment = 4;

// SUSPRDY has the position of LPMODE on the USB block. RM0481 is not
// consistent: the register description (61.6.1) gives it as read-only,
// the suspend procedure (61.5.7) lets software set it. The driver never
// sets it and clears it on wakeup, as LPMODE on STM32L0.
constexpr uint16_t CntrLowPowerMode = USB_CNTR_SUSPRDY;

inline USB_DRD_TypeDef &regs() {
   return *USB_DRD_FS;
}

inline uint16_t readEpr(uint8_t n) {
   return static_cast<uint16_t>(*reinterpret_cast<volatile uint32_t *>(USB_DRD_BASE + 4u * n));
}

inline void writeEpr(uint8_t n, uint16_t value) {
   *reinterpret_cast<volatile uint32_t *>(USB_DRD_BASE + 4u * n) = value;
}

inline volatile uint32_t &pma32(uint16_t offset) {
   return *reinterpret_cast<volatile uint32_t *>(USB_DRD_PMAADDR + (offset & ~3u));
}

inline void resetBtable() {}

// A 16-bit field of the buffer descriptor table (offset even): the low or
// the high half of a TXBD or RXBD word. The driver writes a descriptor
// only while the hardware does not use it, so the read-modify-write of the
// word is safe.
inline uint16_t readBd(uint16_t offset) {
   uint32_t word = pma32(offset);
   return static_cast<uint16_t>((offset & 2u) != 0 ? word >> 16 : word);
}

inline void writeBd(uint16_t offset, uint16_t value) {
   volatile uint32_t &word = pma32(offset);
   if((offset & 2u) != 0) {
      word = (word & 0x0000FFFFu) | (static_cast<uint32_t>(value) << 16);
   } else {
      word = (word & 0xFFFF0000u) | value;
   }
}

// offset is a buffer start, so a multiple of 4. The last word can hold
// bytes after the packet: the buffer covers them (BufferAlignment).
inline void copyToPma(uint16_t offset, const uint8_t *data, uint16_t size) {
   for(uint16_t i = 0; i < size; i += 4) {
      uint32_t word = 0;
      for(uint16_t k = 0; k < 4 && i + k < size; ++k) {
         word |= static_cast<uint32_t>(data[i + k]) << (8u * k);
      }
      pma32(static_cast<uint16_t>(offset + i)) = word;
   }
}

inline void copyFromPma(uint16_t offset, uint8_t *data, uint16_t size) {
   for(uint16_t i = 0; i < size; i += 4) {
      uint32_t word = pma32(static_cast<uint16_t>(offset + i));
      for(uint16_t k = 0; k < 4 && i + k < size; ++k) {
         data[i + k] = static_cast<uint8_t>(word >> (8u * k));
      }
   }
}

// Internal D+ pull-up: USB_BCDR.DPPU.
constexpr bool HasInternalPullup = true;

inline void setInternalPullup(bool on) {
   if(on) {
      USB_DRD_FS->BCDR |= USB_BCDR_DPPU;
   } else {
      USB_DRD_FS->BCDR &= ~USB_BCDR_DPPU;
   }
}

inline bool internalPullup() {
   return (USB_DRD_FS->BCDR & USB_BCDR_DPPU) != 0;
}

} // namespace hw::usbfs

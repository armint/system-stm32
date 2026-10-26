#pragma once

// Access functions of the shared USB full-speed device driver for the USB
// block with a 16-bit packet memory interface (STM32L0, STM32F1): 16-bit
// USB_EPnR registers, the USB_BTABLE register, and 16-bit packet memory
// access. The family access layer (UsbFsFamily.h) defines usbfs::pma16()
// and includes this header after it.

#include <cmsis_device.h>
#include <cstdint>

namespace hw::usbfs {

// Packet buffers start at a half-word boundary.
constexpr uint16_t BufferAlignment = 2;

inline USB_TypeDef &regs() {
   return *USB;
}

inline uint16_t readEpr(uint8_t n) {
   return *reinterpret_cast<volatile uint16_t *>(USB_BASE + 4u * n);
}

inline void writeEpr(uint8_t n, uint16_t value) {
   *reinterpret_cast<volatile uint16_t *>(USB_BASE + 4u * n) = value;
}

// The buffer descriptor table starts at offset 0.
inline void resetBtable() {
   USB->BTABLE = 0;
}

// A 16-bit field of the buffer descriptor table (offset even).
inline uint16_t readBd(uint16_t offset) {
   return pma16(offset);
}

inline void writeBd(uint16_t offset, uint16_t value) {
   pma16(offset) = value;
}

inline void copyToPma(uint16_t offset, const uint8_t *data, uint16_t size) {
   for(uint16_t i = 0; i + 1 < size; i += 2) {
      pma16(static_cast<uint16_t>(offset + i)) = static_cast<uint16_t>(data[i] | (data[i + 1] << 8));
   }
   if(size & 1u) {
      pma16(static_cast<uint16_t>(offset + size - 1)) = data[size - 1];
   }
}

inline void copyFromPma(uint16_t offset, uint8_t *data, uint16_t size) {
   for(uint16_t i = 0; i + 1 < size; i += 2) {
      uint16_t w = pma16(static_cast<uint16_t>(offset + i));
      data[i] = static_cast<uint8_t>(w);
      data[i + 1] = static_cast<uint8_t>(w >> 8);
   }
   if(size & 1u) {
      data[size - 1] = static_cast<uint8_t>(pma16(static_cast<uint16_t>(offset + size - 1)));
   }
}

// The packet is complete in packet memory when CTR_RX is set.
inline void waitForRxPacket() {
}

} // namespace hw::usbfs

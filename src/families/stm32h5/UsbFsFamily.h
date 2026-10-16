#pragma once

// STM32H5 access layer of the shared USB full-speed device driver
// (peripherals/usbfs/UsbDeviceCore): the USB DRD block (RM0481 section 61),
// used in Device mode. The G0B1 and the U5 have the same block.
//
// Differences to the USB block of STM32L0/F1:
//  - 2048 bytes of packet memory. The CPU must use 32-bit access only
//    (RM0481 61.7), and every packet buffer starts at a word boundary.
//  - The buffer descriptors are 32-bit words: TXBD (ADDR_TX, COUNT_TX) and
//    RXBD (ADDR_RX, COUNT_RX, NUM_BLOCK, BLSIZE). The fields have the same
//    bit layout as the 16-bit descriptor fields of the USB block.
//  - USB_CHEPnR is 32 bits wide. Its low 16 bits are the same as USB_EPnR.
//    The upper bits are for Host mode.
//  - No USB_BTABLE register: the descriptor table is at offset 0.
//  - The transceiver has its own supply, VDDUSB. USB33SV must be set before
//    the USB peripheral works (RM0481 10.4.6).
//  - Errata ES0621 2.19.1: after CTR for an OUT or SETUP packet, the last
//    packet memory writes take up to 800 ns more.

#include "delaybusywait.h"
#include <cmsis_device.h>
#include <cstdint>

#if defined(USB_DRD_BASE)

#define SYSTEM_STM32_USBFS 1

namespace hw::usbfs {

constexpr IRQn_Type Irq = USB_DRD_FS_IRQn;

constexpr uint16_t PacketMemorySize = USB_DRD_PMA_SIZE;

constexpr uint16_t BufferAlignment = 4;

// SUSPRDY (the bit of LPMODE on the USB block) is read-only, and the
// hardware clears SUSPEN when it sets WKUP. Nothing to clear on wakeup.
constexpr uint16_t CntrLowPowerMode = 0;

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
// only while the hardware does not use it (see the core), so the
// read-modify-write of the word is safe.
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

// Errata ES0621 2.19.1: wait 800 ns (full speed) after CTR_RX before the
// driver reads COUNT_RX or the packet.
inline void waitForRxPacket() {
   delayuS(1);
}

// VDDUSB valid (RM0481 10.4.6), then BCD and the pull-up off. ST's HAL sets
// USB33SV without a check of USB33RDY. This code waits up to 100 us for
// USB33RDY, then sets USB33SV in all cases: without VDDUSB the USB does
// not work either way.
inline void beforePowerUp() {
   PWR->USBSCR |= PWR_USBSCR_USB33DEN;
   for(int i = 0; i < 100 && (PWR->VMSR & PWR_VMSR_USB33RDY) == 0; ++i) {
      delayuS(1);
   }
   PWR->USBSCR |= PWR_USBSCR_USB33SV;
   USB_DRD_FS->BCDR = 0;
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

#endif

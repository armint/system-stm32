#pragma once

// STM32H5 access layer of the shared USB full-speed device driver
// (peripherals/usbfs/UsbDeviceCore): the USB DRD block (RM0481 section 61),
// used in Device mode. The access functions of the block are in
// peripherals/usbfs/UsbFsDrdAccess.h; this file adds the IRQ, the VDDUSB
// power-up and the errata of the H5.
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

#include "peripherals/usbfs/UsbFsDrdAccess.h"

namespace hw::usbfs {

constexpr IRQn_Type Irq = USB_DRD_FS_IRQn;

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

} // namespace hw::usbfs

#endif

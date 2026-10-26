#pragma once

// Host stand-in for the STM32H5 CMSIS device header: only what the USB
// host test uses. Values from stm32h533xx.h.

#include <cstdint>

#define STM32H5

enum IRQn_Type { USB_DRD_FS_IRQn = 74, TIM2_IRQn = 45 };

// NVIC: the host tests do not check it.
inline void NVIC_EnableIRQ(IRQn_Type) {
}
inline void NVIC_DisableIRQ(IRQn_Type) {
}
inline void NVIC_ClearPendingIRQ(IRQn_Type) {
}

struct PWR_TypeDef {
   volatile uint32_t USBSCR;
   volatile uint32_t VMSR;
};

extern PWR_TypeDef hostPwr;
#define PWR (&hostPwr)
#define PWR_USBSCR_USB33DEN 0x01000000UL
#define PWR_USBSCR_USB33SV 0x02000000UL
#define PWR_VMSR_USB33RDY 0x01000000UL

// USB DRD peripheral. The registers and the packet memory are plain host
// memory (defined by the USB test), so the toggle and rc_w0 bits of the
// hardware are not modelled.
struct USB_DRD_TypeDef {
   volatile uint32_t CHEP0R;
   volatile uint32_t CHEP1R;
   volatile uint32_t CHEP2R;
   volatile uint32_t CHEP3R;
   volatile uint32_t CHEP4R;
   volatile uint32_t CHEP5R;
   volatile uint32_t CHEP6R;
   volatile uint32_t CHEP7R;
   volatile uint32_t RESERVED0[8];
   volatile uint32_t CNTR;
   volatile uint32_t ISTR;
   volatile uint32_t FNR;
   volatile uint32_t DADDR;
   volatile uint32_t RESERVED1;
   volatile uint32_t LPMCSR;
   volatile uint32_t BCDR;
};

using HostUsbTypeDef = USB_DRD_TypeDef;

// 2048 bytes of packet memory. The driver uses 32-bit access only. The
// test reads it as 16-bit words (the array is 4-byte aligned).
#define HOST_USB_PMA_WORDS 1024
extern HostUsbTypeDef hostUsbRegisters;
extern uint16_t hostUsbPacketMemory[HOST_USB_PMA_WORDS];

inline volatile uint32_t &hostEpr(unsigned n) {
   return (&hostUsbRegisters.CHEP0R)[n];
}

#define USB_DRD_BASE (reinterpret_cast<uintptr_t>(&hostUsbRegisters))
#define USB_DRD_PMAADDR (reinterpret_cast<uintptr_t>(hostUsbPacketMemory))
#define USB_DRD_FS (reinterpret_cast<USB_DRD_TypeDef *>(USB_DRD_BASE))
#define USB_DRD_PMA_SIZE (2048U)

#define USB_EP_VTRX 0x00008000UL
#define USB_EP_DTOG_RX 0x00004000UL
#define USB_CHEP_RX_STRX 0x00003000UL
#define USB_EP_SETUP 0x00000800UL
#define USB_EP_UTYPE 0x00000600UL
#define USB_EP_KIND 0x00000100UL
#define USB_EP_VTTX 0x00000080UL
#define USB_EP_DTOG_TX 0x00000040UL
#define USB_CHEP_TX_STTX 0x00000030UL
#define USB_CHEP_ADDR 0x0000000FUL
#define USB_EP_BULK 0x00000000UL
#define USB_EP_CONTROL 0x00000200UL
#define USB_EP_ISOCHRONOUS 0x00000400UL
#define USB_EP_INTERRUPT 0x00000600UL
#define USB_EP_TX_DIS 0x00000000UL
#define USB_EP_TX_STALL 0x00000010UL
#define USB_EP_TX_NAK 0x00000020UL
#define USB_EP_TX_VALID 0x00000030UL
#define USB_EP_RX_DIS 0x00000000UL
#define USB_EP_RX_STALL 0x00001000UL
#define USB_EP_RX_NAK 0x00002000UL
#define USB_EP_RX_VALID 0x00003000UL

#define USB_CNTR_USBRST 0x00000001UL
#define USB_CNTR_PDWN 0x00000002UL
#define USB_CNTR_SUSPRDY 0x00000004UL
#define USB_CNTR_SUSPEN 0x00000008UL
#define USB_CNTR_RESETM 0x00000400UL
#define USB_CNTR_SUSPM 0x00000800UL
#define USB_CNTR_WKUPM 0x00001000UL
#define USB_CNTR_CTRM 0x00008000UL

#define USB_ISTR_IDN 0x0000000FUL
#define USB_ISTR_ESOF 0x00000100UL
#define USB_ISTR_SOF 0x00000200UL
#define USB_ISTR_RESET 0x00000400UL
#define USB_ISTR_SUSP 0x00000800UL
#define USB_ISTR_WKUP 0x00001000UL
#define USB_ISTR_ERR 0x00002000UL
#define USB_ISTR_PMAOVR 0x00004000UL
#define USB_ISTR_CTR 0x00008000UL

#define USB_DADDR_ADD 0x0000007FUL
#define USB_DADDR_EF 0x00000080UL

#define USB_BCDR_DPPU 0x00008000UL

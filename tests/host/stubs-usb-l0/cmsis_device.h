#pragma once

// Host stand-in for the STM32L0 CMSIS device header: only what the USB
// host test uses. Values from stm32l073xx.h.

#include <cstdint>

#define STM32L0

enum IRQn_Type { USB_IRQn = 31, TIM2_IRQn = 15 };

// NVIC: the host tests do not check it.
inline void NVIC_EnableIRQ(IRQn_Type) {}
inline void NVIC_DisableIRQ(IRQn_Type) {}
inline void NVIC_ClearPendingIRQ(IRQn_Type) {}

// USB device peripheral. The registers and the packet memory are plain host
// memory (defined by the USB test), so the toggle and rc_w0 bits of the
// hardware are not modelled.
struct USB_TypeDef {
   volatile uint16_t EP0R;
   volatile uint16_t RESERVED0;
   volatile uint16_t EP1R;
   volatile uint16_t RESERVED1;
   volatile uint16_t EP2R;
   volatile uint16_t RESERVED2;
   volatile uint16_t EP3R;
   volatile uint16_t RESERVED3;
   volatile uint16_t EP4R;
   volatile uint16_t RESERVED4;
   volatile uint16_t EP5R;
   volatile uint16_t RESERVED5;
   volatile uint16_t EP6R;
   volatile uint16_t RESERVED6;
   volatile uint16_t EP7R;
   volatile uint16_t RESERVED7[17];
   volatile uint16_t CNTR;
   volatile uint16_t RESERVED8;
   volatile uint16_t ISTR;
   volatile uint16_t RESERVED9;
   volatile uint16_t FNR;
   volatile uint16_t RESERVEDA;
   volatile uint16_t DADDR;
   volatile uint16_t RESERVEDB;
   volatile uint16_t BTABLE;
   volatile uint16_t RESERVEDC;
   volatile uint16_t LPMCSR;
   volatile uint16_t RESERVEDD;
   volatile uint16_t BCDR;
   volatile uint16_t RESERVEDE;
};

// 1024 bytes of packet memory at a 16-bit stride. It is an array of 16-bit
// words because the driver uses 16-bit access only.
extern USB_TypeDef hostUsbRegisters;
extern uint16_t hostUsbPacketMemory[512];

#define USB_BASE (reinterpret_cast<uintptr_t>(&hostUsbRegisters))
#define USB_PMAADDR (reinterpret_cast<uintptr_t>(hostUsbPacketMemory))
#define USB (reinterpret_cast<USB_TypeDef *>(USB_BASE))

#define USB_EP_CTR_RX 0x00008000U
#define USB_EP_DTOG_RX 0x00004000U
#define USB_EPRX_STAT 0x00003000U
#define USB_EP_SETUP 0x00000800U
#define USB_EP_T_FIELD 0x00000600U
#define USB_EP_KIND 0x00000100U
#define USB_EP_CTR_TX 0x00000080U
#define USB_EP_DTOG_TX 0x00000040U
#define USB_EPTX_STAT 0x00000030U
#define USB_EPADDR_FIELD 0x0000000FU
#define USB_EP_BULK 0x00000000U
#define USB_EP_CONTROL 0x00000200U
#define USB_EP_ISOCHRONOUS 0x00000400U
#define USB_EP_INTERRUPT 0x00000600U
#define USB_EP_TX_DIS 0x00000000U
#define USB_EP_TX_STALL 0x00000010U
#define USB_EP_TX_NAK 0x00000020U
#define USB_EP_TX_VALID 0x00000030U
#define USB_EP_RX_DIS 0x00000000U
#define USB_EP_RX_STALL 0x00001000U
#define USB_EP_RX_NAK 0x00002000U
#define USB_EP_RX_VALID 0x00003000U

#define USB_CNTR_FRES 0x00000001UL
#define USB_CNTR_PDWN 0x00000002UL
#define USB_CNTR_LPMODE 0x00000004UL
#define USB_CNTR_FSUSP 0x00000008UL
#define USB_CNTR_RESETM 0x00000400UL
#define USB_CNTR_SUSPM 0x00000800UL
#define USB_CNTR_WKUPM 0x00001000UL
#define USB_CNTR_CTRM 0x00008000UL

#define USB_ISTR_EP_ID 0x0000000FUL
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

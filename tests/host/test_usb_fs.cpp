// Host tests for the shared USB full-speed device driver
// (peripherals/usbfs/UsbDeviceCore): the COUNTn_RX encoding, the packet
// memory allocation, the buffer descriptor table, and the packet copy. CMake
// builds this file once per family access layer (UsbFsFamily.h), with the
// stub CMSIS header of that family: STM32F1 (512 bytes at a 32-bit CPU
// stride), STM32L0 (1024 bytes at a 16-bit stride) and STM32H5 (USB DRD
// block: 2048 bytes with 32-bit access, 32-bit USB_CHEPnR). The test reads
// the packet memory as 16-bit words at the USB offset.
//
// The registers and the packet memory are plain host memory, so a register
// holds the value of the last write. The hardware toggle bits (STAT, DTOG)
// and rc_w0 bits (CTR) are not modelled. For a call with one EPnR write, the
// test sets a known EPnR value and checks the written value against the
// hardware encoding (testEndpointRegisterWrites). After more than one write,
// the register holds the last written value, not the hardware state; the
// tests check only that value. The interrupt handler runs only for the
// double-buffered OUT tests: the test sets the CTR bits, and the event
// callback clears ISTR, as the hardware does when no CTR bit is left. Where
// a test needs an endpoint state, it writes the register itself.

#include "UsbDevice.h"
#include "delaybusywait.h"
#include "utils/Interrupt.h"

#include <array>
#include <cstdio>
#include <cstring>
#include <functional>
#include <vector>

HostUsbTypeDef hostUsbRegisters;
alignas(4) uint16_t hostUsbPacketMemory[HOST_USB_PMA_WORDS];

#if defined(STM32F1)
constexpr IRQn_Type UsbIrq = USB_LP_CAN1_RX0_IRQn;
// CPU bytes per USB packet memory byte
constexpr unsigned PmaStride = 2;
#elif defined(STM32H5)
PWR_TypeDef hostPwr;
constexpr IRQn_Type UsbIrq = USB_DRD_FS_IRQn;
constexpr unsigned PmaStride = 1;
// The USB DRD names of the bits that the test uses (same positions).
#define USB_EP_CTR_RX USB_EP_VTRX
#define USB_EP_CTR_TX USB_EP_VTTX
#define USB_EPRX_STAT USB_CHEP_RX_STRX
#define USB_EPTX_STAT USB_CHEP_TX_STTX
#define USB_EP_T_FIELD USB_EP_UTYPE
#define USB_EPADDR_FIELD USB_CHEP_ADDR
#else
constexpr IRQn_Type UsbIrq = USB_IRQn;
constexpr unsigned PmaStride = 1;
#endif

// Packet buffers start at a multiple of this (USB DRD: 32-bit words).
constexpr uint16_t align(uint16_t bytes) {
   return static_cast<uint16_t>((bytes + hw::usbfs::BufferAlignment - 1u) & ~(hw::usbfs::BufferAlignment - 1u));
}

namespace {

int failures = 0;

#define CHECK(cond)                                                  \
   do {                                                              \
      if(!(cond)) {                                                  \
         std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
         ++failures;                                                 \
      }                                                              \
   } while(0)

int blockingDepth = 0;
int blockingSections = 0;
IRQn_Type connectedIrq = TIM2_IRQn;

// Runs once at the next delayuS() call, then clears itself: the test uses
// it for hardware writes that come during a delay of the driver.
std::function<void()> delayHook;

using hw::UsbDevice;
using hw::usb::EndpointType;

constexpr uint16_t BtableSize = UsbDevice::EndpointCount * 8;

void resetFakes() {
   std::memset(const_cast<HostUsbTypeDef *>(&hostUsbRegisters), 0, sizeof(hostUsbRegisters));
   std::memset(hostUsbPacketMemory, 0, sizeof(hostUsbPacketMemory));
#if defined(STM32H5)
   std::memset(const_cast<PWR_TypeDef *>(&hostPwr), 0, sizeof(hostPwr));
#endif
}

// The 16-bit word at USB-local offset `offset` (even), as the CPU sees it.
uint16_t &pma(uint16_t offset) {
   return hostUsbPacketMemory[offset * PmaStride / 2];
}

uint16_t addrTx(uint8_t n) {
   return pma(static_cast<uint16_t>(n * 8));
}
uint16_t countTx(uint8_t n) {
   return pma(static_cast<uint16_t>(n * 8 + 2));
}
uint16_t addrRx(uint8_t n) {
   return pma(static_cast<uint16_t>(n * 8 + 4));
}
uint16_t &countRx(uint8_t n) {
   return pma(static_cast<uint16_t>(n * 8 + 6));
}

// Writes bytes i and i + 1 of a test packet (first, first + 1, ...) at the
// USB offset `pma` + i.
void pma16Write(uint16_t pmaOffset, uint16_t i, uint8_t first, uint16_t size) {
   auto byte = [&](uint16_t k) { return static_cast<uint8_t>(k < size ? first + k : 0); };
   pma(static_cast<uint16_t>(pmaOffset + i)) = static_cast<uint16_t>(byte(i) | byte(static_cast<uint16_t>(i + 1)) << 8);
}

auto &epr(uint8_t n) {
   return hostEpr(n);
}

void setEprTxStatus(uint8_t n, uint16_t status) {
   epr(n) = static_cast<uint16_t>((epr(n) & ~USB_EPTX_STAT) | status);
}

// The CPU stride: the upper half of every 32-bit slot stays unused. Always
// true for a 16-bit stride.
bool upperHalvesUnused() {
   if(PmaStride == 1) {
      return true;
   }
   for(size_t word = 1; word < 512; word += 2) {
      if(hostUsbPacketMemory[word] != 0) {
         return false;
      }
   }
   return true;
}

void onEvent(void *, const hw::usb::Event &) {}

// Size in bytes of an RX buffer from its COUNTn_RX value (RM0367 32.6.2,
// RM0008 23.5.3).
uint16_t decodeRxCount(uint16_t count) {
   uint16_t blocks = (count >> 10) & 0x1F;
   return (count & 0x8000) ? static_cast<uint16_t>((blocks + 1) * 32) : static_cast<uint16_t>(blocks * 2);
}

void testRxBufferSize() {
   // BLSIZE 1 and NUM_BLOCK 31 encode the largest buffer: 1024 bytes.
   constexpr uint16_t Largest = UsbDevice::PacketMemorySize < 1024 ? UsbDevice::PacketMemorySize : 1024;
   for(uint16_t size = 1; size <= Largest; ++size) {
      auto rx = UsbDevice::rxBufferSize(size);
      bool largeBlocks = (rx.count & 0x8000) != 0;
      CHECK((rx.count & 0x03FF) == 0); // COUNT is written by the hardware
      CHECK(largeBlocks == (size > 62));
      CHECK(decodeRxCount(rx.count) == rx.pmaBytes);
      CHECK(rx.pmaBytes >= size);
      CHECK(rx.pmaBytes - size < (largeBlocks ? 32 : 2));
      CHECK(rx.pmaBytes % 2 == 0);
   }
}

void testBeginLayout() {
   resetFakes();
   UsbDevice usb;
   usb.begin(&onEvent, nullptr);

   CHECK(connectedIrq == UsbIrq);
#if !defined(STM32H5) // the USB DRD block has no USB_BTABLE
   CHECK(hostUsbRegisters.BTABLE == 0);
#endif
   CHECK(hostUsbRegisters.DADDR == USB_DADDR_EF);
   CHECK(hostUsbRegisters.CNTR == (USB_CNTR_CTRM | USB_CNTR_RESETM | USB_CNTR_SUSPM | USB_CNTR_WKUPM));
   // EP0 buffers directly after the buffer descriptor table, OUT first.
   CHECK(addrRx(0) == BtableSize);
   CHECK(countRx(0) == UsbDevice::rxBufferSize(64).count);
   CHECK(addrTx(0) == BtableSize + 64);
   CHECK(countTx(0) == 0);
   CHECK((epr(0) & (USB_EP_T_FIELD | USB_EPADDR_FIELD)) == USB_EP_CONTROL);
   CHECK(upperHalvesUnused());
   CHECK(blockingDepth == 0);
}

void testOpenEndpointLayout() {
   resetFakes();
   UsbDevice usb;
   usb.begin(&onEvent, nullptr);
   const uint16_t first = BtableSize + 128;

   CHECK(usb.openEndpoint(0x01, EndpointType::Bulk, 64));
   CHECK(usb.openEndpoint(0x81, EndpointType::Bulk, 64));
   CHECK(addrRx(1) == first);
   CHECK(countRx(1) == 0x8400);
   CHECK(addrTx(1) == first + 64);
   CHECK(countTx(1) == 0);
   CHECK((epr(1) & (USB_EP_T_FIELD | USB_EPADDR_FIELD)) == (USB_EP_BULK | 1));

   // Odd sizes take an aligned buffer: half-word, or word on USB DRD.
   CHECK(usb.openEndpoint(0x82, EndpointType::Interrupt, 9));
   CHECK(usb.openEndpoint(0x02, EndpointType::Interrupt, 9));
   CHECK(addrTx(2) == first + 128);
   CHECK(addrRx(2) == first + 128 + align(9));
   CHECK(countRx(2) == (5u << 10));
   CHECK((epr(2) & (USB_EP_T_FIELD | USB_EPADDR_FIELD)) == (USB_EP_INTERRUPT | 2));

   // Reopening with the same size reuses the buffer.
   CHECK(usb.openEndpoint(0x01, EndpointType::Bulk, 64));
   CHECK(addrRx(1) == first);
   CHECK(usb.openEndpoint(0x83, EndpointType::Bulk, 8));
   CHECK(addrTx(3) == first + 128 + align(9) + align(10));

   CHECK(upperHalvesUnused());
   CHECK(blockingDepth == 0);
}

void testOpenEndpointRejects() {
   resetFakes();
   UsbDevice usb;
   usb.begin(&onEvent, nullptr);

   CHECK(!usb.openEndpoint(0x08, EndpointType::Bulk, 64)); // only EP0..EP7
   CHECK(!usb.openEndpoint(0x88, EndpointType::Bulk, 64));
   CHECK(!usb.openEndpoint(0x01, EndpointType::Isochronous, 64));
   CHECK(!usb.openEndpoint(0x01, EndpointType::Control, 64)); // EP0 only
   CHECK(!usb.openEndpoint(0x01, EndpointType::Bulk, 0));
   CHECK(!usb.openEndpoint(0x01, EndpointType::Bulk, 65));

   CHECK(usb.openEndpoint(0x03, EndpointType::Bulk, 64));
   CHECK(!usb.openEndpoint(0x83, EndpointType::Interrupt, 64)); // type differs
   CHECK(!usb.openEndpoint(0x03, EndpointType::Bulk, 32));      // size differs

   // A rejected call allocates nothing.
   CHECK(usb.openEndpoint(0x84, EndpointType::Bulk, 64));
   CHECK(addrTx(4) == BtableSize + 128 + 64);
   CHECK(blockingDepth == 0);
}

void testPacketMemoryFull() {
   resetFakes();
   UsbDevice usb;
   usb.begin(&onEvent, nullptr);

   // PacketMemorySize - 64 (table) - 128 (EP0) bytes of 64-byte buffers:
   // five on STM32F1 (320 bytes), 13 on STM32L0 (832 bytes). Directions
   // in the order 0x01, 0x81, 0x02, 0x82, ... The 14 directions of EP1..EP7
   // do not fill the 2048 bytes of the USB DRD block.
   constexpr unsigned Directions = 14;
   const unsigned fit = (UsbDevice::PacketMemorySize - BtableSize - 128) / 64;
   static_assert((UsbDevice::PacketMemorySize - BtableSize - 128) % 64 == 0);
   auto address = [](unsigned i) { return static_cast<uint8_t>((i / 2 + 1) | (i % 2 ? 0x80 : 0)); };
   const unsigned opened = fit < Directions ? fit : Directions;
   for(unsigned i = 0; i < opened; ++i) {
      CHECK(usb.openEndpoint(address(i), EndpointType::Bulk, 64));
   }
   const uint8_t last = address(opened - 1);
   const uint8_t n = last & 0x7F;
   CHECK(((last & 0x80) ? addrTx(n) : addrRx(n)) == BtableSize + 128 + 64 * (opened - 1));
   if(fit < Directions) {
      CHECK(addrRx(n) == UsbDevice::PacketMemorySize - 64);
      CHECK(!usb.openEndpoint(address(fit), EndpointType::Bulk, 2));
   }

   // closeAllEndpoints() frees everything after the EP0 buffers.
   usb.closeAllEndpoints();
   CHECK(addrRx(0) == BtableSize);
   CHECK(usb.openEndpoint(0x83, EndpointType::Bulk, 2));
   CHECK(addrTx(3) == BtableSize + 128);
   CHECK(usb.openEndpoint(0x01, EndpointType::Bulk, 64));
   CHECK(addrRx(1) == BtableSize + 128 + align(2));
   CHECK(blockingDepth == 0);
}

void testStartInCopiesPacket() {
   resetFakes();
   UsbDevice usb;
   usb.begin(&onEvent, nullptr);
   CHECK(usb.openEndpoint(0x81, EndpointType::Bulk, 64));
   const uint16_t buffer = addrTx(1);

   const std::array<uint8_t, 5> data{ 0x11, 0x22, 0x33, 0x44, 0x55 };
   CHECK(usb.startIn(0x81, data));
   CHECK(countTx(1) == 5);
   CHECK(pma(buffer) == 0x2211);
   CHECK(pma(buffer + 2) == 0x4433);
   CHECK(pma(buffer + 4) == 0x0055);
   CHECK(upperHalvesUnused());

   // A packet is still pending (STAT_TX VALID).
   setEprTxStatus(1, USB_EP_TX_VALID);
   CHECK(!usb.startIn(0x81, data));

   setEprTxStatus(1, USB_EP_TX_NAK);
   std::array<uint8_t, 65> tooLarge{};
   CHECK(!usb.startIn(0x81, tooLarge));
   CHECK(!usb.startIn(0x01, data)); // OUT address
   CHECK(!usb.startIn(0x82, data)); // not open
   CHECK(countTx(1) == 5);

   // A full packet ends exactly at the end of the buffer.
   std::array<uint8_t, 64> full{};
   for(size_t i = 0; i < full.size(); ++i) {
      full[i] = static_cast<uint8_t>(i + 1);
   }
   CHECK(usb.startIn(0x81, full));
   CHECK(countTx(1) == 64);
   CHECK(pma(buffer) == 0x0201);
   CHECK(pma(buffer + 62) == 0x403F);
   CHECK(pma(buffer + 64) == 0); // the next buffer is untouched

   // Zero-length packet.
   setEprTxStatus(1, USB_EP_TX_NAK);
   CHECK(usb.startIn(0x81, hw::span<const uint8_t>()));
   CHECK(countTx(1) == 0);
   CHECK(blockingDepth == 0);
}

void testReadOutCopiesPacket() {
   resetFakes();
   UsbDevice usb;
   usb.begin(&onEvent, nullptr);
   CHECK(usb.openEndpoint(0x01, EndpointType::Bulk, 64));
   const uint16_t buffer = addrRx(1);

   // The hardware writes the received byte count into COUNTn_RX.
   pma(buffer) = 0xBBAA;
   pma(buffer + 2) = 0xDDCC;
   pma(buffer + 4) = 0x77EE; // byte 0x77 is not part of the packet
   countRx(1) = static_cast<uint16_t>(countRx(1) | 5);

   std::array<uint8_t, 64> out{};
   CHECK(usb.readOut(0x01, out) == 5);
   CHECK(out[0] == 0xAA && out[1] == 0xBB && out[2] == 0xCC && out[3] == 0xDD && out[4] == 0xEE);
   CHECK(out[5] == 0);

   std::array<uint8_t, 3> small{};
   CHECK(usb.readOut(0x01, small) == 3);
   CHECK(small[0] == 0xAA && small[1] == 0xBB && small[2] == 0xCC);

   CHECK(usb.readOut(0x81, out) == 0); // IN address
   CHECK(usb.readOut(0x02, out) == 0); // not open
   CHECK(blockingDepth == 0);
}

// EPnR write encoding (RM0367 32.6.1, RM0008 23.5.2): CTR_RX and CTR_TX are written as 1
// (rc_w0: no change), STAT is written as old ^ wanted, DTOG is written as 0
// (no change), and EP_TYPE, EP_KIND and EA keep their value.
constexpr uint16_t CtrKeep = USB_EP_CTR_RX | USB_EP_CTR_TX;

void testEndpointRegisterWrites() {
   resetFakes();
   UsbDevice usb;
   usb.begin(&onEvent, nullptr);

   // After begin(): EP0 is a control endpoint and its last write makes OUT
   // VALID for the first SETUP. That write saw STAT_RX 0 in the fake
   // register and writes STAT_TX as 0 (no change).
   CHECK(epr(0) == (CtrKeep | USB_EP_CONTROL | USB_EP_RX_VALID));

   CHECK(usb.openEndpoint(0x01, EndpointType::Bulk, 64));
   CHECK(usb.openEndpoint(0x81, EndpointType::Bulk, 64));
   const std::array<uint8_t, 2> data{ 1, 2 };

   // startIn(): STAT_TX from NAK to VALID. The other bits do not change.
   epr(1) = USB_EP_BULK | 1 | USB_EP_TX_NAK | USB_EP_DTOG_TX | USB_EP_RX_NAK | USB_EP_DTOG_RX;
   CHECK(usb.startIn(0x81, data));
   CHECK(epr(1) == (CtrKeep | USB_EP_BULK | 1 | (USB_EP_TX_NAK ^ USB_EP_TX_VALID)));

   // armOut(): STAT_RX from NAK to VALID. EP_KIND keeps its value.
   epr(1) = USB_EP_BULK | USB_EP_KIND | 1 | USB_EP_RX_NAK | USB_EP_DTOG_RX | USB_EP_TX_VALID;
   CHECK(usb.armOut(0x01));
   CHECK(epr(1) == (CtrKeep | USB_EP_BULK | USB_EP_KIND | 1 | (USB_EP_RX_NAK ^ USB_EP_RX_VALID)));

   // setStall(true): from NAK to STALL.
   epr(1) = USB_EP_BULK | 1 | USB_EP_TX_NAK | USB_EP_RX_NAK;
   usb.setStall(0x81, true);
   CHECK(epr(1) == (CtrKeep | USB_EP_BULK | 1 | (USB_EP_TX_NAK ^ USB_EP_TX_STALL)));
   epr(1) = USB_EP_BULK | 1 | USB_EP_TX_NAK | USB_EP_RX_NAK;
   usb.setStall(0x01, true);
   CHECK(epr(1) == (CtrKeep | USB_EP_BULK | 1 | (USB_EP_RX_NAK ^ USB_EP_RX_STALL)));

   // setNak(): from STALL to NAK. A pending CTR is written as 1, so it stays.
   epr(1) = USB_EP_CTR_TX | USB_EP_BULK | 1 | USB_EP_TX_STALL;
   usb.setNak(0x81);
   CHECK(epr(1) == (CtrKeep | USB_EP_BULK | 1 | (USB_EP_TX_STALL ^ USB_EP_TX_NAK)));
   epr(1) = USB_EP_CTR_RX | USB_EP_BULK | 1 | USB_EP_RX_VALID;
   usb.setNak(0x01);
   CHECK(epr(1) == (CtrKeep | USB_EP_BULK | 1 | (USB_EP_RX_VALID ^ USB_EP_RX_NAK)));

   // setStall(false) writes twice: DTOG to DATA0, then STAT. The fake
   // register holds the second write, which saw STAT 0 in the first one.
   epr(1) = USB_EP_BULK | 1 | USB_EP_TX_STALL | USB_EP_DTOG_TX;
   usb.setStall(0x81, false);
   CHECK(epr(1) == (CtrKeep | USB_EP_BULK | 1 | USB_EP_TX_NAK));
   epr(1) = USB_EP_BULK | 1 | USB_EP_RX_STALL | USB_EP_DTOG_RX;
   usb.setStall(0x01, false);
   CHECK(epr(1) == (CtrKeep | USB_EP_BULK | 1 | USB_EP_RX_NAK));
   // EP0 OUT goes to VALID, ready for the next SETUP.
   epr(0) = USB_EP_CONTROL | USB_EP_RX_STALL | USB_EP_DTOG_RX;
   usb.setStall(0x00, false);
   CHECK(epr(0) == (CtrKeep | USB_EP_CONTROL | USB_EP_RX_VALID));

   // openEndpoint() of the second direction: DTOG to DATA0, then STAT to
   // NAK. Without the DTOG write, the STAT write would see the old NAK and
   // write STAT_TX 0.
   CHECK(usb.openEndpoint(0x02, EndpointType::Bulk, 64));
   epr(2) = USB_EP_BULK | 2 | USB_EP_TX_NAK | USB_EP_DTOG_TX | USB_EP_RX_NAK;
   CHECK(usb.openEndpoint(0x82, EndpointType::Bulk, 64));
   CHECK(epr(2) == (CtrKeep | USB_EP_BULK | 2 | USB_EP_TX_NAK));

   // No write for an endpoint that is not open.
   epr(3) = USB_EP_TX_NAK;
   usb.setStall(0x83, true);
   usb.setNak(0x83);
   CHECK(!usb.armOut(0x03));
   CHECK(epr(3) == USB_EP_TX_NAK);

   // No write when startIn() rejects the packet.
   epr(1) = USB_EP_BULK | 1 | USB_EP_TX_VALID;
   CHECK(!usb.startIn(0x81, data));
   CHECK(epr(1) == (USB_EP_BULK | 1 | USB_EP_TX_VALID));
   CHECK(blockingDepth == 0);
}

// A device with endpoint numbers 0..4 has a 40-byte buffer descriptor
// table, so the endpoint buffers start at offset 40.
void testSmallBufferDescriptorTable() {
   resetFakes();
   UsbDevice usb{ 5 };
   usb.begin(&onEvent, nullptr);
   constexpr uint16_t btable = 5 * 8;
   CHECK(addrRx(0) == btable);
   CHECK(addrTx(0) == btable + 64);
   CHECK(!usb.openEndpoint(0x05, EndpointType::Bulk, 64)); // only EP0..EP4
   CHECK(!usb.openEndpoint(0x85, EndpointType::Bulk, 64));
   CHECK(usb.openEndpoint(0x84, EndpointType::Bulk, 64));
   CHECK(addrTx(4) == btable + 128);

   // closeAllEndpoints() frees everything after the EP0 buffers.
   usb.closeAllEndpoints();
   CHECK(usb.openEndpoint(0x01, EndpointType::Bulk, 64));
   CHECK(addrRx(1) == btable + 128);

   // A count of 0 or above 8 gives 8.
   resetFakes();
   UsbDevice all{ 9 };
   all.begin(&onEvent, nullptr);
   CHECK(addrRx(0) == BtableSize);
   CHECK(all.openEndpoint(0x07, EndpointType::Bulk, 8));
   resetFakes();
   UsbDevice none{ 0 };
   none.begin(&onEvent, nullptr);
   CHECK(addrRx(0) == BtableSize);
   CHECK(blockingDepth == 0);
}

// The layout of a device with endpoint numbers 0..4 and a double-buffered
// bulk OUT endpoint: the Versaloon dongle (vendor OUT 0x03 double-buffered,
// IN 0x82, CDC notification 0x81, CDC data 0x04 and 0x84).
void testDoubleBufferedOutLayout() {
   resetFakes();
   UsbDevice usb{ 5 };
   usb.begin(&onEvent, nullptr);
   constexpr uint16_t btable = 5 * 8;
   CHECK(addrRx(0) == btable);
   CHECK(addrTx(0) == btable + 64);

   const uint16_t first = btable + 128;
   CHECK(usb.openEndpoint(0x03, EndpointType::Bulk, 64, true));
   CHECK(addrTx(3) == first); // buffer 0
   CHECK(addrRx(3) == first + 64); // buffer 1
   CHECK(countTx(3) == UsbDevice::rxBufferSize(64).count);
   CHECK(countRx(3) == UsbDevice::rxBufferSize(64).count);
   CHECK((epr(3) & (USB_EP_T_FIELD | USB_EP_KIND | USB_EPADDR_FIELD)) == (USB_EP_BULK | USB_EP_KIND | 3));

   CHECK(usb.openEndpoint(0x82, EndpointType::Bulk, 64));
   CHECK(usb.openEndpoint(0x81, EndpointType::Interrupt, 16));
   CHECK(usb.openEndpoint(0x04, EndpointType::Bulk, 64));
   CHECK(usb.openEndpoint(0x84, EndpointType::Bulk, 64));
   // 504 bytes, so it fits on STM32F1 (512 bytes)
   CHECK(addrTx(4) == first + 128 + 64 + 16 + 64);
   CHECK(addrTx(4) + 64 <= UsbDevice::PacketMemorySize);
   // A normal endpoint has no DBL_BUF.
   CHECK((epr(4) & USB_EP_KIND) == 0);

   // Reopening keeps the buffers, also double-buffered.
   CHECK(usb.openEndpoint(0x03, EndpointType::Bulk, 64, true));
   CHECK(addrTx(3) == first);
   CHECK(addrRx(3) == first + 64);
   CHECK(upperHalvesUnused());
   CHECK(blockingDepth == 0);
}

void testDoubleBufferedOutRejects() {
   resetFakes();
   UsbDevice usb{ 5 };
   usb.begin(&onEvent, nullptr);

   CHECK(!usb.openEndpoint(0x81, EndpointType::Bulk, 64, true)); // IN
   CHECK(!usb.openEndpoint(0x01, EndpointType::Interrupt, 64, true)); // not bulk

   CHECK(usb.openEndpoint(0x81, EndpointType::Bulk, 64));
   CHECK(!usb.openEndpoint(0x01, EndpointType::Bulk, 64, true)); // IN is open

   CHECK(usb.openEndpoint(0x02, EndpointType::Bulk, 64, true));
   CHECK(!usb.openEndpoint(0x82, EndpointType::Bulk, 64)); // unidirectional
   CHECK(!usb.openEndpoint(0x02, EndpointType::Bulk, 64)); // doubleBuffered differs
   CHECK(!usb.openEndpoint(0x02, EndpointType::Bulk, 32, true)); // size differs

   CHECK(blockingDepth == 0);
}

std::vector<hw::usb::Event> events;
// When set: the STAT_RX that the hardware has when the driver reports a
// packet. The fake register loses STAT on each write, so the callback puts
// it back, before the driver checks for the NAK of the first packet.
constexpr uint16_t NoStat = 0xFFFF;
uint16_t rxStatAtEvent = NoStat;

// The interrupt clears ISTR.CTR when no CTR bit is left. The fake does it
// when the driver reports an event. The fake register keeps a CTR bit that
// the driver writes as 1 (no change), so the handler can see a CTR_TX that
// the hardware does not set. The test records only the Out events.
void recordEvent(void *, const hw::usb::Event &event) {
   if(event.type == hw::usb::EventType::Out) {
      events.push_back(event);
      if(rxStatAtEvent != NoStat) {
         epr(event.endpoint) = static_cast<uint16_t>((epr(event.endpoint) & ~USB_EPRX_STAT) | rxStatAtEvent);
      }
   }
   hostUsbRegisters.ISTR = 0;
}

// The hardware receives a packet into `buffer` of double-buffered endpoint
// n, toggles DTOG_RX, sets CTR_RX, and raises the interrupt.
void receiveDoubleBuffered(UsbDevice &usb, uint8_t n, uint8_t buffer, uint8_t first, uint16_t size) {
   uint16_t address = buffer == 0 ? addrTx(n) : addrRx(n);
   for(uint16_t i = 0; i < size; i += 2) {
      pma16Write(address, i, first, size);
   }
   uint16_t &count = buffer == 0 ? pma(static_cast<uint16_t>(n * 8 + 2)) : countRx(n);
   count = static_cast<uint16_t>((count & ~0x03FFu) | size);
   // After the packet, DTOG_RX selects the other buffer.
   uint16_t dtog = buffer == 0 ? USB_EP_DTOG_RX : 0;
   epr(n) = static_cast<uint16_t>(USB_EP_CTR_RX | USB_EP_BULK | USB_EP_KIND | n | USB_EP_RX_VALID | dtog);
   hostUsbRegisters.ISTR = static_cast<uint16_t>(USB_ISTR_CTR | n);
   usb.handleInterrupt();
}

void testDoubleBufferedOutFlow() {
   resetFakes();
   events.clear();
   UsbDevice usb{ 5 };
   usb.begin(&recordEvent, nullptr);
   CHECK(usb.openEndpoint(0x03, EndpointType::Bulk, 64, true));
   std::array<uint8_t, 64> out{};

   // Before the first packet: no packet to read.
   CHECK(usb.armOut(0x03));
   CHECK(usb.readOut(0x03, out) == 0);

   // Packet A in buffer 0: reported at once. The driver toggles SW_BUF, so
   // the hardware can receive into buffer 1.
   receiveDoubleBuffered(usb, 3, 0, 0xA0, 64);
   CHECK(events.size() == 1);
   CHECK(events.back().type == hw::usb::EventType::Out && events.back().endpoint == 3 && events.back().size == 64);
   CHECK(epr(3) == (CtrKeep | USB_EP_BULK | USB_EP_KIND | 3 | USB_EP_DTOG_TX));
   CHECK(usb.readOut(0x03, out) == 64);
   CHECK(out[0] == 0xA0 && out[63] == 0xA0 + 63);

   // Packet B in buffer 1 before armOut(): no event and no SW_BUF toggle.
   // The software still reads packet A.
   receiveDoubleBuffered(usb, 3, 1, 0xB0, 10);
   CHECK(events.size() == 1);
   CHECK((epr(3) & USB_EP_DTOG_TX) == 0);
   CHECK(usb.readOut(0x03, out) == 64);
   CHECK(out[0] == 0xA0);

   // armOut() frees buffer 0 and reports packet B.
   CHECK(usb.armOut(0x03));
   CHECK(events.size() == 2);
   CHECK(events.back().size == 10);
   CHECK(usb.readOut(0x03, out) == 10);
   CHECK(out[0] == 0xB0 && out[9] == 0xB9);

   // armOut() with no waiting packet reports nothing. Packet C in buffer 0
   // is reported at once.
   CHECK(usb.armOut(0x03));
   CHECK(events.size() == 2);
   CHECK(usb.readOut(0x03, out) == 0);
   receiveDoubleBuffered(usb, 3, 0, 0xC0, 1);
   CHECK(events.size() == 3);
   CHECK(usb.readOut(0x03, out) == 1);
   CHECK(out[0] == 0xC0);

   // A clear of STALL drops a waiting packet and starts at buffer 0 again:
   // DTOG_RX 0 and SW_BUF 1.
   receiveDoubleBuffered(usb, 3, 1, 0xD0, 4);
   CHECK(events.size() == 3);
   epr(3) = USB_EP_BULK | USB_EP_KIND | 3 | USB_EP_RX_STALL | USB_EP_DTOG_RX;
   usb.setStall(0x03, false);
   CHECK(usb.readOut(0x03, out) == 0);
   CHECK(usb.armOut(0x03));
   CHECK(events.size() == 3);
   CHECK(blockingDepth == 0);
}

// The first packet after DBL_BUF sets STAT_RX to NAK (RM0367 32.5.3,
// RM0008 23.4.3). The
// interrupt makes an armed endpoint VALID again, and leaves a NAK of
// setNak().
void testDoubleBufferedOutFirstPacketNak() {
   resetFakes();
   events.clear();
   UsbDevice usb{ 5 };
   usb.begin(&recordEvent, nullptr);
   CHECK(usb.openEndpoint(0x03, EndpointType::Bulk, 64, true));
   CHECK(usb.armOut(0x03));

   rxStatAtEvent = USB_EP_RX_NAK;
   receiveDoubleBuffered(usb, 3, 0, 0xA0, 8);
   CHECK(events.size() == 1);
   // STAT from NAK to VALID; the DTOG bits are written as 0 (no change).
   CHECK(epr(3) == (CtrKeep | USB_EP_BULK | USB_EP_KIND | 3 | (USB_EP_RX_NAK ^ USB_EP_RX_VALID)));

   // After setNak() the endpoint is not armed: the NAK stays.
   CHECK(usb.armOut(0x03));
   usb.setNak(0x03);
   receiveDoubleBuffered(usb, 3, 1, 0xB0, 8);
   CHECK(events.size() == 2);
   CHECK(epr(3) == (CtrKeep | USB_EP_BULK | USB_EP_KIND | 3 | USB_EP_DTOG_TX | USB_EP_RX_NAK));
   rxStatAtEvent = NoStat;
   CHECK(blockingDepth == 0);
}

// The toggle writes: DTOG_RX to 0 and SW_BUF (DTOG_TX) to 1, as 1 where
// the bit must change.
void testDoubleBufferedOutToggles() {
   resetFakes();
   UsbDevice usb{ 5 };
   usb.begin(&onEvent, nullptr);
   CHECK(usb.openEndpoint(0x03, EndpointType::Bulk, 64, true));

   epr(3) = USB_EP_BULK | USB_EP_KIND | 3 | USB_EP_RX_STALL | USB_EP_DTOG_RX;
   usb.setStall(0x03, false);
   // Two writes; the fake holds the second (STAT to NAK), which saw STAT 0.
   CHECK(epr(3) == (CtrKeep | USB_EP_BULK | USB_EP_KIND | 3 | USB_EP_RX_NAK));
   CHECK(usb.openEndpoint(0x03, EndpointType::Bulk, 64, true));
   CHECK(blockingDepth == 0);
}

void testSetAddressAndStall() {
   resetFakes();
   UsbDevice usb;
   usb.begin(&onEvent, nullptr);

   usb.setAddress(0x85); // only ADD[6:0]
   CHECK(hostUsbRegisters.DADDR == (USB_DADDR_EF | 0x05));

   setEprTxStatus(1, USB_EP_TX_STALL);
   CHECK(usb.isStalled(0x81));
   CHECK(!usb.isStalled(0x01));
   CHECK(!usb.isStalled(0x88));
}

int pullupCalls = 0;
bool pullupOn = false;

void recordPullup(void *, bool enable) {
   ++pullupCalls;
   pullupOn = enable;
}

// STM32L0, STM32H5: begin() clears USB_BCDR, connect()/disconnect() switch
// USB_BCDR.DPPU, and isConnected() reads it back. STM32F1: no USB_BCDR, so
// isConnected() is the requested state. Both: the callback runs, if set.
void testPullup() {
   resetFakes();
   pullupCalls = 0;
   UsbDevice usb;
#if defined(USB_BCDR_DPPU)
   hostUsbRegisters.BCDR = USB_BCDR_DPPU | 1u;
#endif
   usb.begin(&onEvent, nullptr);
#if defined(USB_BCDR_DPPU)
   CHECK(hostUsbRegisters.BCDR == 0);
#endif
   CHECK(!usb.isConnected());

   usb.connect(); // no callback
   CHECK(usb.isConnected());
#if defined(USB_BCDR_DPPU)
   CHECK(hostUsbRegisters.BCDR == USB_BCDR_DPPU);
   // The state comes from the register.
   hostUsbRegisters.BCDR = 0;
   CHECK(!usb.isConnected());
   hostUsbRegisters.BCDR = USB_BCDR_DPPU;
#endif

   usb.setPullupCallback(&recordPullup, nullptr);
   usb.disconnect();
   CHECK(!usb.isConnected());
   CHECK(pullupCalls == 1 && !pullupOn);
   usb.connect();
   CHECK(usb.isConnected());
   CHECK(pullupCalls == 2 && pullupOn);
#if defined(USB_BCDR_DPPU)
   CHECK(hostUsbRegisters.BCDR == USB_BCDR_DPPU);
#endif

   usb.end();
   CHECK(!usb.isConnected());
   CHECK(pullupCalls == 3 && !pullupOn);
#if defined(USB_BCDR_DPPU)
   CHECK(hostUsbRegisters.BCDR == 0);
#endif
}

// Every buffer starts at BufferAlignment, also the second buffer of a
// double-buffered endpoint with an odd size.
void testBufferAlignment() {
   resetFakes();
   UsbDevice usb{ 5 };
   usb.begin(&onEvent, nullptr);
   CHECK(usb.openEndpoint(0x81, EndpointType::Interrupt, 9));
   CHECK(usb.openEndpoint(0x02, EndpointType::Bulk, 9, true));
   CHECK(usb.openEndpoint(0x83, EndpointType::Bulk, 3));
   for(uint16_t a : { addrTx(1), addrTx(2), addrRx(2), addrTx(3) }) {
      CHECK(a % hw::usbfs::BufferAlignment == 0);
   }
   CHECK(addrRx(2) == addrTx(2) + align(UsbDevice::rxBufferSize(9).pmaBytes));
   CHECK(blockingDepth == 0);
}

#if defined(STM32H5)
// USB DRD block: begin() validates VDDUSB, and the driver writes the upper
// (Host mode) bits of USB_CHEPnR as 0.
void testUsbDrd() {
   resetFakes();
   UsbDevice usb;
   usb.begin(&onEvent, nullptr);
   CHECK((hostPwr.USBSCR & (PWR_USBSCR_USB33DEN | PWR_USBSCR_USB33SV)) == (PWR_USBSCR_USB33DEN | PWR_USBSCR_USB33SV));

   CHECK(usb.openEndpoint(0x81, EndpointType::Bulk, 64));
   epr(1) = 0x06000000u | USB_EP_BULK | 1 | USB_EP_TX_STALL;
   usb.setNak(0x81);
   CHECK(epr(1) == (CtrKeep | USB_EP_BULK | 1 | (USB_EP_TX_STALL ^ USB_EP_TX_NAK)));

   // A packet of 5 bytes fills two words; the rest of the last word is 0.
   const uint16_t buffer = addrTx(1);
   pma(static_cast<uint16_t>(buffer + 6)) = 0xFFFF;
   const std::array<uint8_t, 5> data{ 0x11, 0x22, 0x33, 0x44, 0x55 };
   epr(1) = USB_EP_BULK | 1 | USB_EP_TX_NAK;
   CHECK(usb.startIn(0x81, data));
   CHECK(pma(buffer) == 0x2211 && pma(static_cast<uint16_t>(buffer + 2)) == 0x4433);
   CHECK(pma(static_cast<uint16_t>(buffer + 4)) == 0x0055 && pma(static_cast<uint16_t>(buffer + 6)) == 0);
   // COUNT_TX in the high half of TXBD; ADDR_TX in the low half is kept.
   CHECK(countTx(1) == 5 && addrTx(1) == buffer);
   CHECK(blockingDepth == 0);
}

// begin() enables the VDDUSB monitor, waits for USB33RDY, then sets
// USB33SV. The hook runs at the first wait: DEN is set, SV not yet.
void testVddUsbOrder() {
   resetFakes();
   bool denBeforeSv = false;
   delayHook = [&] {
      denBeforeSv = (hostPwr.USBSCR & PWR_USBSCR_USB33DEN) != 0 && (hostPwr.USBSCR & PWR_USBSCR_USB33SV) == 0;
      hostPwr.VMSR = PWR_VMSR_USB33RDY;
   };
   UsbDevice usb;
   usb.begin(&onEvent, nullptr);
   CHECK(denBeforeSv);
   CHECK(!delayHook); // the hook ran
   CHECK((hostPwr.USBSCR & PWR_USBSCR_USB33SV) != 0);
}

// Errata ES0621 2.19.1: COUNT_RX and the last packet bytes arrive after
// CTR_RX. The hook writes them at the wait of the driver, so the event and
// readOut() see the old values if the driver reads before the wait.
void testRxWaitBeforeRead() {
   resetFakes();
   events.clear();
   UsbDevice usb{ 5 };
   usb.begin(&recordEvent, nullptr);
   std::array<uint8_t, 64> out{};

   // Single-buffered bulk OUT.
   CHECK(usb.openEndpoint(0x01, EndpointType::Bulk, 64));
   CHECK(usb.armOut(0x01));
   const uint16_t rx = addrRx(1);
   delayHook = [&] {
      pma(rx) = 0xBBAA;
      pma(static_cast<uint16_t>(rx + 2)) = 0x00CC;
      countRx(1) = static_cast<uint16_t>((countRx(1) & ~0x03FFu) | 3);
   };
   epr(1) = USB_EP_CTR_RX | USB_EP_BULK | 1 | USB_EP_RX_NAK;
   hostUsbRegisters.ISTR = USB_ISTR_CTR | 1;
   usb.handleInterrupt();
   CHECK(!delayHook);
   CHECK(events.size() == 1 && events.back().size == 3);
   CHECK(usb.readOut(0x01, out) == 3);
   CHECK(out[0] == 0xAA && out[1] == 0xBB && out[2] == 0xCC);

   // Double-buffered bulk OUT: the packet goes into buffer 0 (COUNT in the
   // TXBD half).
   CHECK(usb.openEndpoint(0x03, EndpointType::Bulk, 64, true));
   CHECK(usb.armOut(0x03));
   const uint16_t buffer0 = addrTx(3);
   delayHook = [&] {
      pma(buffer0) = 0x2211;
      uint16_t &count = pma(static_cast<uint16_t>(3 * 8 + 2));
      count = static_cast<uint16_t>((count & ~0x03FFu) | 2);
   };
   epr(3) = USB_EP_CTR_RX | USB_EP_BULK | USB_EP_KIND | 3 | USB_EP_RX_VALID | USB_EP_DTOG_RX;
   hostUsbRegisters.ISTR = USB_ISTR_CTR | 3;
   usb.handleInterrupt();
   CHECK(!delayHook);
   CHECK(events.size() == 2 && events.back().endpoint == 3 && events.back().size == 2);
   CHECK(usb.readOut(0x03, out) == 2);
   CHECK(out[0] == 0x11 && out[1] == 0x22);
   CHECK(blockingDepth == 0);
}
#endif

} // namespace

// Host stand-ins for utils/Interrupt.cpp and the delay code.
namespace Interrupt {
BlockingSection::BlockingSection(IRQn_Type IRQn)
   : irqn(static_cast<int8_t>(IRQn)) {
   ++blockingSections;
   ++blockingDepth;
}

BlockingSection::~BlockingSection() {
   --blockingDepth;
}

void connectHandler(IRQn_Type IRQn, AdvancedHandler_t, void *) {
   connectedIrq = IRQn;
}

void releaseHandler(IRQn_Type) {}
} // namespace Interrupt

extern "C" void delayuS(duration_t) {
   if(delayHook) {
      auto hook = std::move(delayHook);
      delayHook = nullptr;
      hook();
   }
}

int main() {
   testRxBufferSize();
   testBeginLayout();
   testOpenEndpointLayout();
   testOpenEndpointRejects();
   testPacketMemoryFull();
   testStartInCopiesPacket();
   testReadOutCopiesPacket();
   testEndpointRegisterWrites();
   testSetAddressAndStall();
   testSmallBufferDescriptorTable();
   testDoubleBufferedOutLayout();
   testDoubleBufferedOutRejects();
   testDoubleBufferedOutFlow();
   testDoubleBufferedOutFirstPacketNak();
   testDoubleBufferedOutToggles();
   testPullup();
   testBufferAlignment();
#if defined(STM32H5)
   testUsbDrd();
   testVddUsbOrder();
   testRxWaitBeforeRead();
#endif
   if(failures != 0) {
      std::printf("%d check(s) failed\n", failures);
      return 1;
   }
#if defined(STM32F1)
   const char *family = "STM32F1";
#elif defined(STM32H5)
   const char *family = "STM32H5";
#else
   const char *family = "STM32L0";
#endif
   std::printf("all USB FS host tests passed for %s (%d blocking sections)\n", family, blockingSections);
   return 0;
}

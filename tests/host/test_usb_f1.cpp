// Host tests for the STM32F1 USB device driver: the COUNTn_RX encoding, the
// packet memory allocation, the buffer descriptor table, and the packet copy
// at the 32-bit CPU stride of the packet memory.
//
// The registers and the packet memory are plain host memory. The hardware
// toggle bits (STAT, DTOG) and rc_w0 bits (CTR) are not modelled, so these
// tests do not check endpoint states or the interrupt handler. Where a test
// needs an endpoint state, it writes the register itself.

#include "UsbDevice.h"
#include "delaybusywait.h"
#include "utils/Interrupt.h"

#include <array>
#include <cstdio>
#include <cstring>

USB_TypeDef hostUsbRegisters;
uint16_t hostUsbPacketMemory[512];

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

using hw::UsbDevice;
using hw::usb::EndpointType;

constexpr uint16_t BtableSize = UsbDevice::EndpointCount * 8;

void resetFakes() {
   std::memset(const_cast<USB_TypeDef *>(&hostUsbRegisters), 0, sizeof(hostUsbRegisters));
   std::memset(hostUsbPacketMemory, 0, sizeof(hostUsbPacketMemory));
}

// The 16-bit word at USB-local offset `offset` (even), as the CPU sees it.
uint16_t &pma(uint16_t offset) {
   return hostUsbPacketMemory[offset];
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

volatile uint16_t &epr(uint8_t n) {
   return (&hostUsbRegisters.EP0R)[n * 2];
}

void setEprTxStatus(uint8_t n, uint16_t status) {
   epr(n) = static_cast<uint16_t>((epr(n) & ~USB_EPTX_STAT) | status);
}

// The CPU stride: the upper half of every 32-bit slot stays unused.
bool upperHalvesUnused() {
   for(size_t word = 1; word < 512; word += 2) {
      if(hostUsbPacketMemory[word] != 0) {
         return false;
      }
   }
   return true;
}

void onEvent(void *, const hw::usb::Event &) {}

// Size in bytes of an RX buffer from its COUNTn_RX value (RM0008 23.5.3).
uint16_t decodeRxCount(uint16_t count) {
   uint16_t blocks = (count >> 10) & 0x1F;
   return (count & 0x8000) ? static_cast<uint16_t>((blocks + 1) * 32) : static_cast<uint16_t>(blocks * 2);
}

void testRxBufferSize() {
   for(uint16_t size = 1; size <= UsbDevice::PacketMemorySize; ++size) {
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

   CHECK(connectedIrq == USB_LP_CAN1_RX0_IRQn);
   CHECK(hostUsbRegisters.BTABLE == 0);
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

   // Odd sizes take a half-word aligned buffer.
   CHECK(usb.openEndpoint(0x82, EndpointType::Interrupt, 9));
   CHECK(usb.openEndpoint(0x02, EndpointType::Interrupt, 9));
   CHECK(addrTx(2) == first + 128);
   CHECK(addrRx(2) == first + 128 + 10);
   CHECK(countRx(2) == (5u << 10));
   CHECK((epr(2) & (USB_EP_T_FIELD | USB_EPADDR_FIELD)) == (USB_EP_INTERRUPT | 2));

   // Reopening with the same size reuses the buffer.
   CHECK(usb.openEndpoint(0x01, EndpointType::Bulk, 64));
   CHECK(addrRx(1) == first);
   CHECK(usb.openEndpoint(0x83, EndpointType::Bulk, 8));
   CHECK(addrTx(3) == first + 148);

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

   // 512 - 64 (table) - 128 (EP0) = 320 bytes: five 64-byte buffers.
   CHECK(usb.openEndpoint(0x01, EndpointType::Bulk, 64));
   CHECK(usb.openEndpoint(0x81, EndpointType::Bulk, 64));
   CHECK(usb.openEndpoint(0x02, EndpointType::Bulk, 64));
   CHECK(usb.openEndpoint(0x82, EndpointType::Bulk, 64));
   CHECK(usb.openEndpoint(0x03, EndpointType::Bulk, 64));
   CHECK(addrRx(3) == UsbDevice::PacketMemorySize - 64);
   CHECK(!usb.openEndpoint(0x83, EndpointType::Bulk, 2));

   // closeAllEndpoints() frees everything after the EP0 buffers.
   usb.closeAllEndpoints();
   CHECK(addrRx(0) == BtableSize);
   CHECK(usb.openEndpoint(0x83, EndpointType::Bulk, 2));
   CHECK(addrTx(3) == BtableSize + 128);
   CHECK(usb.openEndpoint(0x01, EndpointType::Bulk, 64));
   CHECK(addrRx(1) == BtableSize + 130);
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

extern "C" void delayuS(duration_t) {}

int main() {
   testRxBufferSize();
   testBeginLayout();
   testOpenEndpointLayout();
   testOpenEndpointRejects();
   testPacketMemoryFull();
   testStartInCopiesPacket();
   testReadOutCopiesPacket();
   testSetAddressAndStall();
   if(failures != 0) {
      std::printf("%d check(s) failed\n", failures);
      return 1;
   }
   std::printf("all USB F1 host tests passed (%d blocking sections)\n", blockingSections);
   return 0;
}

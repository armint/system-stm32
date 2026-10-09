#include "UsbDevice.h"
#include "delaybusywait.h"
#include "utils/Interrupt.h"
#include <initializer_list>

#if defined(USB_BASE)

namespace hw {

namespace {

using usb::EndpointType;
using usb::EventType;

constexpr IRQn_Type UsbIrq = USB_LP_CAN1_RX0_IRQn;

// --- Endpoint registers (USB_EPnR, RM0008 23.5.2) ----------------------------
// Bit types: CTR_RX/CTR_TX are rc_w0 (write 1 = no change), DTOG_x and
// STAT_x are toggle bits (write 0 = no change), SETUP is read-only. Every
// write therefore uses these "invariant" values for the bits it does not
// want to change, and never a read-modify-write of the raw value.

// Bits that keep their value when written back as read.
constexpr uint16_t EpConfigBits = USB_EP_T_FIELD | USB_EP_KIND | USB_EPADDR_FIELD;
// Write 1 to the CTR bits so a CTR set by hardware meanwhile is not lost.
constexpr uint16_t EpCtrKeep = USB_EP_CTR_RX | USB_EP_CTR_TX;

volatile uint16_t &epr(uint8_t n) {
   return *reinterpret_cast<volatile uint16_t *>(USB_BASE + 4u * n);
}

void setTxStatus(uint8_t n, uint16_t status) {
   uint16_t v = epr(n);
   epr(n) = static_cast<uint16_t>((v & EpConfigBits) | EpCtrKeep | ((v & USB_EPTX_STAT) ^ status));
}

void setRxStatus(uint8_t n, uint16_t status) {
   uint16_t v = epr(n);
   epr(n) = static_cast<uint16_t>((v & EpConfigBits) | EpCtrKeep | ((v & USB_EPRX_STAT) ^ status));
}

// Data toggle to DATA0 (toggle bit: write 1 only if it is 1 now).
void resetTxToggle(uint8_t n) {
   uint16_t v = epr(n);
   epr(n) = static_cast<uint16_t>((v & EpConfigBits) | EpCtrKeep | (v & USB_EP_DTOG_TX));
}

void resetRxToggle(uint8_t n) {
   uint16_t v = epr(n);
   epr(n) = static_cast<uint16_t>((v & EpConfigBits) | EpCtrKeep | (v & USB_EP_DTOG_RX));
}

void clearCtrRx(uint8_t n) {
   uint16_t v = epr(n);
   epr(n) = static_cast<uint16_t>((v & EpConfigBits) | USB_EP_CTR_TX);
}

void clearCtrTx(uint8_t n) {
   uint16_t v = epr(n);
   epr(n) = static_cast<uint16_t>((v & EpConfigBits) | USB_EP_CTR_RX);
}

// Type and address are plain rw bits; the toggle bits are written as 0.
void setTypeAndAddress(uint8_t n, EndpointType type) {
   uint16_t t = USB_EP_BULK;
   switch(type) {
      case EndpointType::Control:
         t = USB_EP_CONTROL;
         break;
      case EndpointType::Interrupt:
         t = USB_EP_INTERRUPT;
         break;
      case EndpointType::Isochronous:
         t = USB_EP_ISOCHRONOUS;
         break;
      case EndpointType::Bulk:
         break;
   }
   epr(n) = static_cast<uint16_t>(EpCtrKeep | t | n);
}

// --- Packet memory (RM0008 23.5, 23.5.3) -------------------------------------
// 512 bytes, addressed by the USB peripheral with byte offsets. The CPU sees
// them as 256 16-bit words at a 32-bit stride: USB offset o (even) is at
// USB_PMAADDR + o * 2, and the upper half of each 32-bit slot is unused.
// The driver uses 16-bit access. The buffer descriptor table is at offset 0
// (USB_BTABLE). Every offset passed to pma16() is even.

volatile uint16_t &pma16(uint16_t offset) {
   return *reinterpret_cast<volatile uint16_t *>(USB_PMAADDR + offset * 2u);
}

volatile uint16_t &addrTx(uint8_t n) {
   return pma16(static_cast<uint16_t>(n * 8u));
}
volatile uint16_t &countTx(uint8_t n) {
   return pma16(static_cast<uint16_t>(n * 8u + 2u));
}
volatile uint16_t &addrRx(uint8_t n) {
   return pma16(static_cast<uint16_t>(n * 8u + 4u));
}
volatile uint16_t &countRx(uint8_t n) {
   return pma16(static_cast<uint16_t>(n * 8u + 6u));
}

constexpr uint16_t CountMask = 0x03FF;

void copyToPma(uint16_t offset, const uint8_t *data, uint16_t size) {
   for(uint16_t i = 0; i + 1 < size; i += 2) {
      pma16(offset + i) = static_cast<uint16_t>(data[i] | (data[i + 1] << 8));
   }
   if(size & 1u) {
      pma16(offset + size - 1) = data[size - 1];
   }
}

void copyFromPma(uint16_t offset, uint8_t *data, uint16_t size) {
   for(uint16_t i = 0; i + 1 < size; i += 2) {
      uint16_t w = pma16(offset + i);
      data[i] = static_cast<uint8_t>(w);
      data[i + 1] = static_cast<uint8_t>(w >> 8);
   }
   if(size & 1u) {
      data[size - 1] = static_cast<uint8_t>(pma16(offset + size - 1));
   }
}

constexpr uint16_t InterruptMask = USB_CNTR_CTRM | USB_CNTR_RESETM | USB_CNTR_SUSPM | USB_CNTR_WKUPM;

} // namespace

// Compile-time checks of the COUNTn_RX encoding (RM0008 23.5.3).
static_assert(UsbDevice::rxBufferSize(8).count == (4u << 10) && UsbDevice::rxBufferSize(8).pmaBytes == 8);
static_assert(UsbDevice::rxBufferSize(9).pmaBytes == 10);
static_assert(UsbDevice::rxBufferSize(62).count == (31u << 10) && UsbDevice::rxBufferSize(62).pmaBytes == 62);
static_assert(UsbDevice::rxBufferSize(63).count == 0x8400 && UsbDevice::rxBufferSize(63).pmaBytes == 64);
static_assert(UsbDevice::rxBufferSize(64).count == 0x8400 && UsbDevice::rxBufferSize(64).pmaBytes == 64);
static_assert(UsbDevice::rxBufferSize(0).pmaBytes == 2);

UsbDevice::UsbDevice(uint8_t endpointCount)
   : _endpointCount(endpointCount == 0 || endpointCount > EndpointCount ? EndpointCount : endpointCount)
   , _btableSize(static_cast<uint16_t>(_endpointCount * 8u))
   , _pmaNext(_btableSize) {
}

void UsbDevice::begin(usb::EventCallback callback, void *context) {
   _callback = callback;
   _context = context;

   // Power-up sequence (RM0008 23.4.2): clear PDWN with FRES still set,
   // wait tSTARTUP (1 us max, STM32F103x8/B datasheet), then release FRES
   // and clear spurious interrupt flags.
   USB->CNTR = USB_CNTR_FRES;
   delayuS(10);
   USB->CNTR = 0;
   USB->ISTR = 0;
   USB->BTABLE = 0;
   resetEndpoints();
   USB->CNTR = InterruptMask;

   Interrupt::connectMemberHandler<UsbDevice, &UsbDevice::handleInterrupt>(UsbIrq, *this);
   NVIC_ClearPendingIRQ(UsbIrq);
   NVIC_EnableIRQ(UsbIrq);
}

void UsbDevice::end() {
   disconnect();
   NVIC_DisableIRQ(UsbIrq);
   USB->CNTR = USB_CNTR_FRES;
   USB->ISTR = 0;
   USB->CNTR = USB_CNTR_FRES | USB_CNTR_PDWN;
   Interrupt::releaseHandler(UsbIrq);
   _callback = nullptr;
}

void UsbDevice::setPullupCallback(PullupCallback callback, void *context) {
   _pullupCallback = callback;
   _pullupContext = context;
}

void UsbDevice::connect() {
   _connected = true;
   if(_pullupCallback != nullptr) {
      _pullupCallback(_pullupContext, true);
   }
}

void UsbDevice::disconnect() {
   _connected = false;
   if(_pullupCallback != nullptr) {
      _pullupCallback(_pullupContext, false);
   }
}

bool UsbDevice::isConnected() const {
   return _connected;
}

void UsbDevice::setAddress(uint8_t address) {
   USB->DADDR = static_cast<uint16_t>(USB_DADDR_EF | (address & USB_DADDR_ADD));
}

UsbDevice::Direction *UsbDevice::direction(uint8_t address) {
   uint8_t n = usb::endpointNumber(address);
   if(n >= _endpointCount) {
      return nullptr;
   }
   return usb::isInEndpoint(address) ? &_endpoints[n].in : &_endpoints[n].out;
}

const UsbDevice::Direction *UsbDevice::direction(uint8_t address) const {
   return const_cast<UsbDevice *>(this)->direction(address);
}

uint16_t UsbDevice::allocate(uint16_t bytes) {
   bytes = static_cast<uint16_t>((bytes + 1u) & ~1u); // half-word aligned
   if(bytes > PacketMemorySize - _pmaNext) {
      return 0;
   }
   uint16_t address = _pmaNext;
   _pmaNext = static_cast<uint16_t>(_pmaNext + bytes);
   return address;
}

bool UsbDevice::openEndpoint(uint8_t address, usb::EndpointType type, uint16_t maxPacketSize) {
   Interrupt::BlockingSection block(UsbIrq);
   uint8_t n = usb::endpointNumber(address);
   Direction *dir = direction(address);
   if(dir == nullptr || type == EndpointType::Isochronous || (type == EndpointType::Control && n != 0) ||
      maxPacketSize == 0 || maxPacketSize > 64) {
      return false;
   }
   Endpoint &ep = _endpoints[n];
   const Direction &other = usb::isInEndpoint(address) ? ep.out : ep.in;
   if(other.open && ep.type != type) {
      return false;
   }

   bool in = usb::isInEndpoint(address);
   // Reopening with the same size reuses the buffer. A different size would
   // need a new buffer and leak the old one until closeAllEndpoints().
   if(dir->open && dir->maxPacketSize != maxPacketSize) {
      return false;
   }
   if(!dir->open) {
      uint16_t bytes = in ? maxPacketSize : rxBufferSize(maxPacketSize).pmaBytes;
      uint16_t pma = allocate(bytes);
      if(pma == 0) {
         return false;
      }
      dir->pmaAddress = pma;
      dir->maxPacketSize = maxPacketSize;
   }
   dir->open = true;
   ep.type = type;

   if(!other.open) {
      setTypeAndAddress(n, type);
   }
   if(in) {
      addrTx(n) = dir->pmaAddress;
      countTx(n) = 0;
      resetTxToggle(n);
      setTxStatus(n, USB_EP_TX_NAK);
   } else {
      addrRx(n) = dir->pmaAddress;
      countRx(n) = rxBufferSize(maxPacketSize).count;
      resetRxToggle(n);
      setRxStatus(n, USB_EP_RX_NAK);
   }
   return true;
}

void UsbDevice::closeAllEndpoints() {
   Interrupt::BlockingSection block(UsbIrq);
   for(uint8_t n = 1; n < EndpointCount; ++n) {
      setTxStatus(n, USB_EP_TX_DIS);
      setRxStatus(n, USB_EP_RX_DIS);
      _endpoints[n] = {};
   }
   // EP0 buffers are allocated first, so everything after them is free.
   uint16_t end = _btableSize;
   for(const Direction *d : { &_endpoints[0].in, &_endpoints[0].out }) {
      if(d->open) {
         uint16_t bytes = d == &_endpoints[0].in ? d->maxPacketSize : rxBufferSize(d->maxPacketSize).pmaBytes;
         uint16_t last = static_cast<uint16_t>(d->pmaAddress + ((bytes + 1u) & ~1u));
         if(last > end) {
            end = last;
         }
      }
   }
   _pmaNext = end;
}

void UsbDevice::resetEndpoints() {
   for(uint8_t n = 0; n < EndpointCount; ++n) {
      _endpoints[n] = {};
   }
   _pmaNext = _btableSize;
   // EP0: control, OUT VALID so the first SETUP is received, IN NAK.
   (void)openEndpoint(0x00, EndpointType::Control, Ep0MaxPacketSize);
   (void)openEndpoint(0x80, EndpointType::Control, Ep0MaxPacketSize);
   setRxStatus(0, USB_EP_RX_VALID);
   USB->DADDR = USB_DADDR_EF;
}

bool UsbDevice::startIn(uint8_t address, hw::span<const uint8_t> data) {
   Interrupt::BlockingSection block(UsbIrq);
   if(!usb::isInEndpoint(address)) {
      return false;
   }
   const Direction *dir = direction(address);
   uint8_t n = usb::endpointNumber(address);
   if(dir == nullptr || !dir->open || data.size() > dir->maxPacketSize ||
      (epr(n) & USB_EPTX_STAT) == USB_EP_TX_VALID) {
      return false;
   }
   uint16_t size = static_cast<uint16_t>(data.size());
   copyToPma(dir->pmaAddress, data.data(), size);
   countTx(n) = size;
   setTxStatus(n, USB_EP_TX_VALID);
   return true;
}

bool UsbDevice::armOut(uint8_t address) {
   Interrupt::BlockingSection block(UsbIrq);
   const Direction *dir = direction(address);
   if(usb::isInEndpoint(address) || dir == nullptr || !dir->open) {
      return false;
   }
   setRxStatus(usb::endpointNumber(address), USB_EP_RX_VALID);
   return true;
}

uint16_t UsbDevice::readOut(uint8_t address, hw::span<uint8_t> buffer) const {
   Interrupt::BlockingSection block(UsbIrq);
   const Direction *dir = direction(address);
   if(usb::isInEndpoint(address) || dir == nullptr || !dir->open) {
      return 0;
   }
   uint8_t n = usb::endpointNumber(address);
   uint16_t size = n == 0 ? _ep0OutSize : countRx(n) & CountMask;
   if(size > buffer.size()) {
      size = static_cast<uint16_t>(buffer.size());
   }
   if(n == 0) {
      for(uint16_t i = 0; i < size; ++i) {
         buffer[i] = _ep0Out[i];
      }
   } else {
      copyFromPma(dir->pmaAddress, buffer.data(), size);
   }
   return size;
}

void UsbDevice::setStall(uint8_t address, bool stall) {
   Interrupt::BlockingSection block(UsbIrq);
   const Direction *dir = direction(address);
   if(dir == nullptr || !dir->open) {
      return;
   }
   uint8_t n = usb::endpointNumber(address);
   if(usb::isInEndpoint(address)) {
      if(stall) {
         setTxStatus(n, USB_EP_TX_STALL);
      } else {
         resetTxToggle(n);
         setTxStatus(n, USB_EP_TX_NAK);
      }
   } else {
      if(stall) {
         setRxStatus(n, USB_EP_RX_STALL);
      } else {
         resetRxToggle(n);
         // EP0 OUT must stay ready for the next SETUP.
         setRxStatus(n, n == 0 ? USB_EP_RX_VALID : USB_EP_RX_NAK);
      }
   }
}

bool UsbDevice::isStalled(uint8_t address) const {
   uint8_t n = usb::endpointNumber(address);
   if(n >= EndpointCount) {
      return false;
   }
   if(usb::isInEndpoint(address)) {
      return (epr(n) & USB_EPTX_STAT) == USB_EP_TX_STALL;
   }
   return (epr(n) & USB_EPRX_STAT) == USB_EP_RX_STALL;
}

void UsbDevice::setNak(uint8_t address) {
   Interrupt::BlockingSection block(UsbIrq);
   const Direction *dir = direction(address);
   if(dir == nullptr || !dir->open) {
      return;
   }
   uint8_t n = usb::endpointNumber(address);
   if(usb::isInEndpoint(address)) {
      setTxStatus(n, USB_EP_TX_NAK);
   } else {
      setRxStatus(n, USB_EP_RX_NAK);
   }
}

void UsbDevice::report(usb::EventType type, uint8_t endpoint, uint16_t size) {
   if(_callback != nullptr) {
      usb::Event event{ type, endpoint, size, {} };
      _callback(_context, event);
   }
}

void UsbDevice::handleInterrupt() {
   // ISTR flags are rc_w0: write 0 to clear a flag, 1 leaves the others.
   uint16_t istr = USB->ISTR;

   if(istr & USB_ISTR_RESET) {
      USB->ISTR = static_cast<uint16_t>(~USB_ISTR_RESET);
      USB->CNTR = static_cast<uint16_t>(USB->CNTR & ~USB_CNTR_FSUSP);
      resetEndpoints();
      report(EventType::Reset, 0);
   }

   // One endpoint per iteration, highest priority first (ISTR.EP_ID).
   while((istr = USB->ISTR) & USB_ISTR_CTR) {
      uint8_t n = istr & USB_ISTR_EP_ID;
      uint16_t v = epr(n);
      if((v & (USB_EP_CTR_RX | USB_EP_CTR_TX)) == 0) {
         // ISTR.CTR without a CTR bit in the register: nothing to do. A
         // CTR that comes later raises the interrupt again.
         break;
      }
      // TX before RX: if both CTR bits are set, the IN transaction came
      // first. After a SETUP the hardware sets STAT_TX to NAK, so no IN can
      // complete while that CTR_RX is pending. Example: the host ACKs the
      // last IN data packet and sends the OUT status packet before this
      // handler runs. The layer above must see InComplete, then Out (or a
      // stale InComplete before a new Setup).
      if(v & USB_EP_CTR_TX) {
         clearCtrTx(n);
         report(EventType::InComplete, static_cast<uint8_t>(n | usb::EndpointDirectionIn));
      }
      if(v & USB_EP_CTR_RX) {
         uint16_t size = countRx(n) & CountMask;
         if(v & USB_EP_SETUP) {
            // Copy before CTR_RX is cleared: a control endpoint accepts the
            // next SETUP even while it answers NAK, which would overwrite it.
            usb::Event event{ EventType::Setup, n, 8, {} };
            copyFromPma(_endpoints[n].out.pmaAddress, event.setup.data(), 8);
            clearCtrRx(n);
            if(_callback != nullptr) {
               _callback(_context, event);
            }
         } else {
            if(n == 0) {
               // Same reason as SETUP: copy before CTR_RX is cleared. While
               // CTR_RX is set, the hardware discards a new SETUP.
               _ep0OutSize = size > Ep0MaxPacketSize ? Ep0MaxPacketSize : size;
               copyFromPma(_endpoints[0].out.pmaAddress, _ep0Out, _ep0OutSize);
            }
            clearCtrRx(n);
            report(EventType::Out, n, size);
         }
      }
   }

   if(istr & USB_ISTR_SUSP) {
      // FSUSP stops further SUSP interrupts while suspended (RM0008 23.4.5).
      USB->CNTR = static_cast<uint16_t>(USB->CNTR | USB_CNTR_FSUSP);
      USB->ISTR = static_cast<uint16_t>(~USB_ISTR_SUSP);
      report(EventType::Suspend, 0);
   }

   if(istr & USB_ISTR_WKUP) {
      USB->CNTR = static_cast<uint16_t>(USB->CNTR & ~(USB_CNTR_FSUSP | USB_CNTR_LP_MODE));
      USB->ISTR = static_cast<uint16_t>(~USB_ISTR_WKUP);
      report(EventType::Resume, 0);
   }

   // Masked sources can still be flagged; clear them so they stay quiet.
   USB->ISTR = static_cast<uint16_t>(~(USB_ISTR_ERR | USB_ISTR_PMAOVR | USB_ISTR_SOF | USB_ISTR_ESOF));
}

} // namespace hw

#endif

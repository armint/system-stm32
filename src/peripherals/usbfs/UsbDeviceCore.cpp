#include "UsbDevice.h"
#include "delaybusywait.h"
#include "utils/Interrupt.h"
#include <initializer_list>

#if defined(SYSTEM_STM32_USBFS)

namespace hw {

namespace {

using usb::EndpointType;
using usb::EventType;

constexpr IRQn_Type UsbIrq = usbfs::Irq;

// --- Register bits ------------------------------------------------------------
// The bits below have other CMSIS names on the USB DRD block (STM32H5:
// USB_EP_VTRX, USB_CHEP_RX_STRX, USB_CNTR_USBRST, ...) than on the USB block
// (STM32L0, STM32F1: USB_EP_CTR_RX, USB_EPRX_STAT, USB_CNTR_FRES, ...). The
// bit positions are the same. The other bits have the same CMSIS names.
constexpr uint16_t EpCtrRx = 0x8000;   // CTR_RX, VTRX
constexpr uint16_t EpRxStat = 0x3000;  // STAT_RX, STATRX
constexpr uint16_t EpType = 0x0600;    // EP_TYPE, UTYPE
constexpr uint16_t EpCtrTx = 0x0080;   // CTR_TX, VTTX
constexpr uint16_t EpTxStat = 0x0030;  // STAT_TX, STATTX
constexpr uint16_t EpAddress = 0x000F; // EA
constexpr uint16_t CntrReset = 0x0001;   // FRES, USBRST
constexpr uint16_t CntrSuspend = 0x0008; // FSUSP, SUSPEN
constexpr uint16_t IstrEndpoint = 0x000F; // EP_ID, IDN

#if defined(USB_EP_CTR_RX)
static_assert(EpCtrRx == USB_EP_CTR_RX && EpRxStat == USB_EPRX_STAT && EpType == USB_EP_T_FIELD);
static_assert(EpCtrTx == USB_EP_CTR_TX && EpTxStat == USB_EPTX_STAT && EpAddress == USB_EPADDR_FIELD);
static_assert(CntrReset == USB_CNTR_FRES && CntrSuspend == USB_CNTR_FSUSP && IstrEndpoint == USB_ISTR_EP_ID);
#else
static_assert(EpCtrRx == USB_EP_VTRX && EpRxStat == USB_CHEP_RX_STRX && EpType == USB_EP_UTYPE);
static_assert(EpCtrTx == USB_EP_VTTX && EpTxStat == USB_CHEP_TX_STTX && EpAddress == USB_CHEP_ADDR);
static_assert(CntrReset == USB_CNTR_USBRST && CntrSuspend == USB_CNTR_SUSPEN && IstrEndpoint == USB_ISTR_IDN);
#endif

// --- Endpoint registers (USB_EPnR, RM0367 32.6.1, RM0008 23.5.2; USB_CHEPnR,
// RM0481 61.6.7) ----------------------------------------------------------------
// Bit types: CTR_RX/CTR_TX are rc_w0 (write 1 = no change), DTOG_x and
// STAT_x are toggle bits (write 0 = no change), SETUP is read-only. Every
// write therefore uses these "invariant" values for the bits it does not
// want to change, and never a read-modify-write of the raw value. The
// driver uses only the low 16 bits; the upper bits of USB_CHEPnR are for
// Host mode and are written as 0.

// Bits that keep their value when written back as read.
constexpr uint16_t EpConfigBits = EpType | USB_EP_KIND | EpAddress;
// Write 1 to the CTR bits so a CTR set by hardware meanwhile is not lost.
constexpr uint16_t EpCtrKeep = EpCtrRx | EpCtrTx;

uint16_t epr(uint8_t n) {
   return usbfs::readEpr(n);
}

void setTxStatus(uint8_t n, uint16_t status) {
   uint16_t v = epr(n);
   usbfs::writeEpr(n, static_cast<uint16_t>((v & EpConfigBits) | EpCtrKeep | ((v & EpTxStat) ^ status)));
}

void setRxStatus(uint8_t n, uint16_t status) {
   uint16_t v = epr(n);
   usbfs::writeEpr(n, static_cast<uint16_t>((v & EpConfigBits) | EpCtrKeep | ((v & EpRxStat) ^ status)));
}

// Data toggle to DATA0 (toggle bit: write 1 only if it is 1 now).
void resetTxToggle(uint8_t n) {
   uint16_t v = epr(n);
   usbfs::writeEpr(n, static_cast<uint16_t>((v & EpConfigBits) | EpCtrKeep | (v & USB_EP_DTOG_TX)));
}

void resetRxToggle(uint8_t n) {
   uint16_t v = epr(n);
   usbfs::writeEpr(n, static_cast<uint16_t>((v & EpConfigBits) | EpCtrKeep | (v & USB_EP_DTOG_RX)));
}

// Double-buffered OUT (RM0367 32.5.3, RM0008 23.4.3): DTOG_RX selects the
// buffer of the hardware, SW_BUF (the DTOG_TX bit) the buffer of the
// software. Start with DTOG_RX = 0 and SW_BUF = 1: the hardware receives
// into buffer 0 first.
void resetDoubleBufferToggles(uint8_t n) {
   uint16_t v = epr(n);
   usbfs::writeEpr(n, static_cast<uint16_t>((v & EpConfigBits) | EpCtrKeep | (v & USB_EP_DTOG_RX) | (~v & USB_EP_DTOG_TX)));
}

// Gives one buffer back to the hardware.
void toggleSwBuf(uint8_t n) {
   uint16_t v = epr(n);
   usbfs::writeEpr(n, static_cast<uint16_t>((v & EpConfigBits) | EpCtrKeep | USB_EP_DTOG_TX));
}

void clearCtrRx(uint8_t n) {
   uint16_t v = epr(n);
   usbfs::writeEpr(n, static_cast<uint16_t>((v & EpConfigBits) | EpCtrTx));
}

void clearCtrTx(uint8_t n) {
   uint16_t v = epr(n);
   usbfs::writeEpr(n, static_cast<uint16_t>((v & EpConfigBits) | EpCtrRx));
}

// Type, kind and address are plain rw bits; the toggle bits are written as
// 0. kind: DBL_BUF for a bulk endpoint.
void setTypeAndAddress(uint8_t n, EndpointType type, bool kind) {
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
   usbfs::writeEpr(n, static_cast<uint16_t>(EpCtrKeep | t | (kind ? USB_EP_KIND : 0u) | n));
}

// --- Packet memory (RM0367 32.6.2, RM0008 23.5.3, RM0481 61.7) -------------
// usbfs::PacketMemorySize bytes, addressed by the USB peripheral with byte
// offsets. The buffer descriptor table is at offset 0, 8 bytes per endpoint
// number: ADDR_TX, COUNT_TX, ADDR_RX, COUNT_RX, 16 bits each (on the USB
// DRD block: the low and high halves of the 32-bit TXBD and RXBD). The
// access layer reads and writes these fields and copies the packets
// (usbfs::readBd(), usbfs::writeBd(), usbfs::copyToPma(),
// usbfs::copyFromPma()). Every buffer starts at a multiple of
// usbfs::BufferAlignment.

using usbfs::copyFromPma;
using usbfs::copyToPma;

constexpr uint16_t AddrTx = 0;
constexpr uint16_t CountTx = 2;
constexpr uint16_t AddrRx = 4;
constexpr uint16_t CountRx = 6;

uint16_t readBd(uint8_t n, uint16_t field) {
   return usbfs::readBd(static_cast<uint16_t>(n * 8u + field));
}

void writeBd(uint8_t n, uint16_t field, uint16_t value) {
   usbfs::writeBd(static_cast<uint16_t>(n * 8u + field), value);
}

constexpr uint16_t alignBuffer(uint16_t bytes) {
   return static_cast<uint16_t>((bytes + usbfs::BufferAlignment - 1u) & ~(usbfs::BufferAlignment - 1u));
}

constexpr uint16_t CountMask = 0x03FF;

constexpr uint16_t InterruptMask = USB_CNTR_CTRM | USB_CNTR_RESETM | USB_CNTR_SUSPM | USB_CNTR_WKUPM;

} // namespace

// Compile-time checks of the COUNTn_RX encoding (RM0367 32.6.2, RM0008
// 23.5.3).
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

   // Power-up sequence (RM0367 32.5.2, RM0008 23.4.2): clear PDWN with
   // FRES still set, wait tSTARTUP (1 us max, STM32L073 and STM32F103x8/B
   // datasheets), then release FRES and clear spurious interrupt flags.
   usbfs::beforePowerUp();
   usbfs::regs().CNTR = CntrReset;
   delayuS(10);
   usbfs::regs().CNTR = 0;
   usbfs::regs().ISTR = 0;
   usbfs::resetBtable();
   resetEndpoints();
   usbfs::regs().CNTR = InterruptMask;

   Interrupt::connectMemberHandler<UsbDevice, &UsbDevice::handleInterrupt>(UsbIrq, *this);
   NVIC_ClearPendingIRQ(UsbIrq);
   NVIC_EnableIRQ(UsbIrq);
}

void UsbDevice::end() {
   disconnect();
   NVIC_DisableIRQ(UsbIrq);
   usbfs::regs().CNTR = CntrReset;
   usbfs::regs().ISTR = 0;
   usbfs::regs().CNTR = CntrReset | USB_CNTR_PDWN;
   Interrupt::releaseHandler(UsbIrq);
   _callback = nullptr;
}

void UsbDevice::setPullupCallback(PullupCallback callback, void *context) {
   _pullupCallback = callback;
   _pullupContext = context;
}

void UsbDevice::connect() {
   _connected = true;
   usbfs::setInternalPullup(true);
   if(_pullupCallback != nullptr) {
      _pullupCallback(_pullupContext, true);
   }
}

void UsbDevice::disconnect() {
   _connected = false;
   usbfs::setInternalPullup(false);
   if(_pullupCallback != nullptr) {
      _pullupCallback(_pullupContext, false);
   }
}

bool UsbDevice::isConnected() const {
   return usbfs::HasInternalPullup ? usbfs::internalPullup() : _connected;
}

void UsbDevice::setAddress(uint8_t address) {
   usbfs::regs().DADDR = static_cast<uint16_t>(USB_DADDR_EF | (address & USB_DADDR_ADD));
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
   bytes = alignBuffer(bytes);
   if(bytes > PacketMemorySize - _pmaNext) {
      return 0;
   }
   uint16_t address = _pmaNext;
   _pmaNext = static_cast<uint16_t>(_pmaNext + bytes);
   return address;
}

bool UsbDevice::openEndpoint(uint8_t address, usb::EndpointType type, uint16_t maxPacketSize, bool doubleBuffered) {
   Interrupt::BlockingSection block(UsbIrq);
   uint8_t n = usb::endpointNumber(address);
   Direction *dir = direction(address);
   if(dir == nullptr || type == EndpointType::Isochronous || (type == EndpointType::Control && n != 0) ||
      maxPacketSize == 0 || maxPacketSize > 64) {
      return false;
   }
   bool in = usb::isInEndpoint(address);
   Endpoint &ep = _endpoints[n];
   const Direction &other = in ? ep.out : ep.in;
   if(other.open && (ep.type != type || doubleBuffered || other.doubleBuffered)) {
      return false;
   }
   if(doubleBuffered && (in || type != EndpointType::Bulk)) {
      return false;
   }

   // Reopening with the same size reuses the buffer. A different size would
   // need a new buffer and leak the old one until closeAllEndpoints().
   if(dir->open && (dir->maxPacketSize != maxPacketSize || dir->doubleBuffered != doubleBuffered)) {
      return false;
   }
   if(!dir->open) {
      uint16_t bytes = in ? maxPacketSize : rxBufferSize(maxPacketSize).pmaBytes;
      uint16_t pma = allocate(doubleBuffered ? static_cast<uint16_t>(2u * bytes) : bytes);
      if(pma == 0) {
         return false;
      }
      dir->pmaAddress = pma;
      dir->pmaAddress1 = doubleBuffered ? static_cast<uint16_t>(pma + bytes) : 0u;
      dir->maxPacketSize = maxPacketSize;
      dir->doubleBuffered = doubleBuffered;
   }
   dir->open = true;
   ep.type = type;

   if(!other.open) {
      setTypeAndAddress(n, type, doubleBuffered);
   }
   if(doubleBuffered) {
      writeBd(n, AddrTx, dir->pmaAddress);
      writeBd(n, CountTx, rxBufferSize(maxPacketSize).count);
      writeBd(n, AddrRx, dir->pmaAddress1);
      writeBd(n, CountRx, rxBufferSize(maxPacketSize).count);
      resetDoubleBuffer(n);
   } else if(in) {
      writeBd(n, AddrTx, dir->pmaAddress);
      writeBd(n, CountTx, 0);
      resetTxToggle(n);
      setTxStatus(n, USB_EP_TX_NAK);
   } else {
      writeBd(n, AddrRx, dir->pmaAddress);
      writeBd(n, CountRx, rxBufferSize(maxPacketSize).count);
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
         uint16_t last = static_cast<uint16_t>(d->pmaAddress + alignBuffer(bytes));
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
   usbfs::regs().DADDR = USB_DADDR_EF;
}

bool UsbDevice::startIn(uint8_t address, hw::span<const uint8_t> data) {
   Interrupt::BlockingSection block(UsbIrq);
   if(!usb::isInEndpoint(address)) {
      return false;
   }
   const Direction *dir = direction(address);
   uint8_t n = usb::endpointNumber(address);
   if(dir == nullptr || !dir->open || data.size() > dir->maxPacketSize ||
      (epr(n) & EpTxStat) == USB_EP_TX_VALID) {
      return false;
   }
   uint16_t size = static_cast<uint16_t>(data.size());
   copyToPma(dir->pmaAddress, data.data(), size);
   writeBd(n, CountTx, size);
   setTxStatus(n, USB_EP_TX_VALID);
   return true;
}

bool UsbDevice::armOut(uint8_t address) {
   Interrupt::BlockingSection block(UsbIrq);
   const Direction *dir = direction(address);
   if(usb::isInEndpoint(address) || dir == nullptr || !dir->open) {
      return false;
   }
   uint8_t n = usb::endpointNumber(address);
   if(dir->doubleBuffered) {
      Direction &out = _endpoints[n].out;
      out.reportedBuffer = NoBuffer;
      if(out.pendingBuffer != NoBuffer) {
         uint8_t buffer = out.pendingBuffer;
         out.pendingBuffer = NoBuffer;
         reportDoubleBuffered(n, buffer);
      }
      out.armed = true;
      // STAT stays VALID after the first packet (RM0367 32.5.3, RM0008
      // 23.4.3). It is NAK only before the first armOut(), after setNak(),
      // and after the first packet.
      if((epr(n) & EpRxStat) != USB_EP_RX_VALID) {
         setRxStatus(n, USB_EP_RX_VALID);
      }
      return true;
   }
   setRxStatus(n, USB_EP_RX_VALID);
   return true;
}

uint16_t UsbDevice::readOut(uint8_t address, hw::span<uint8_t> buffer) const {
   Interrupt::BlockingSection block(UsbIrq);
   const Direction *dir = direction(address);
   if(usb::isInEndpoint(address) || dir == nullptr || !dir->open) {
      return 0;
   }
   uint8_t n = usb::endpointNumber(address);
   uint16_t pma = dir->pmaAddress;
   uint16_t size = n == 0 ? _ep0OutSize : readBd(n, CountRx) & CountMask;
   if(dir->doubleBuffered) {
      if(dir->reportedBuffer == NoBuffer) {
         return 0;
      }
      if(dir->reportedBuffer == 0) {
         size = readBd(n, CountTx) & CountMask;
      } else {
         pma = dir->pmaAddress1;
      }
   }
   if(size > buffer.size()) {
      size = static_cast<uint16_t>(buffer.size());
   }
   if(n == 0) {
      for(uint16_t i = 0; i < size; ++i) {
         buffer[i] = _ep0Out[i];
      }
   } else {
      copyFromPma(pma, buffer.data(), size);
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
      } else if(dir->doubleBuffered) {
         // Drops the packets in the buffers, as a clear drops the transfer.
         resetDoubleBuffer(n);
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
      return (epr(n) & EpTxStat) == USB_EP_TX_STALL;
   }
   return (epr(n) & EpRxStat) == USB_EP_RX_STALL;
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
      _endpoints[n].out.armed = false;
      setRxStatus(n, USB_EP_RX_NAK);
   }
}

void UsbDevice::report(usb::EventType type, uint8_t endpoint, uint16_t size) {
   if(_callback != nullptr) {
      usb::Event event{ type, endpoint, size, {} };
      _callback(_context, event);
   }
}

void UsbDevice::resetDoubleBuffer(uint8_t n) {
   Direction &out = _endpoints[n].out;
   out.armed = false;
   out.reportedBuffer = NoBuffer;
   out.pendingBuffer = NoBuffer;
   resetDoubleBufferToggles(n);
   setRxStatus(n, USB_EP_RX_NAK);
}

void UsbDevice::receiveDoubleBuffered(uint8_t n, uint16_t eprValue) {
   Direction &out = _endpoints[n].out;
   // The hardware toggled DTOG_RX at the end of the packet, so the packet is
   // in the other buffer.
   uint8_t buffer = (eprValue & USB_EP_DTOG_RX) != 0 ? 0 : 1;
   if(out.reportedBuffer == NoBuffer) {
      reportDoubleBuffered(n, buffer);
   } else {
      // The software reads the other buffer. DTOG_RX equals SW_BUF now, so
      // the hardware NAKs until armOut() reports this packet.
      out.pendingBuffer = buffer;
   }
   // The first packet after DBL_BUF sets STAT to NAK (RM0367 32.5.3, RM0008
   // 23.4.3).
   if(out.armed && (epr(n) & EpRxStat) == USB_EP_RX_NAK) {
      setRxStatus(n, USB_EP_RX_VALID);
   }
}

void UsbDevice::reportDoubleBuffered(uint8_t n, uint8_t buffer) {
   Direction &out = _endpoints[n].out;
   out.reportedBuffer = buffer;
   // The software keeps this buffer until armOut(). The toggle gives the
   // other buffer to the hardware, so the next packet can come in now.
   toggleSwBuf(n);
   uint16_t size = static_cast<uint16_t>((buffer == 0 ? readBd(n, CountTx) : readBd(n, CountRx)) & CountMask);
   report(EventType::Out, n, size);
}

void UsbDevice::handleInterrupt() {
   // ISTR flags are rc_w0: write 0 to clear a flag, 1 leaves the others.
   uint16_t istr = usbfs::regs().ISTR;

   if(istr & USB_ISTR_RESET) {
      usbfs::regs().ISTR = static_cast<uint16_t>(~USB_ISTR_RESET);
      usbfs::regs().CNTR = static_cast<uint16_t>(usbfs::regs().CNTR & ~CntrSuspend);
      resetEndpoints();
      report(EventType::Reset, 0);
   }

   // One endpoint per iteration, highest priority first (ISTR.EP_ID).
   while((istr = usbfs::regs().ISTR) & USB_ISTR_CTR) {
      uint8_t n = istr & IstrEndpoint;
      uint16_t v = epr(n);
      if((v & (EpCtrRx | EpCtrTx)) == 0) {
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
      if(v & EpCtrTx) {
         clearCtrTx(n);
         report(EventType::InComplete, static_cast<uint8_t>(n | usb::EndpointDirectionIn));
      }
      if(v & EpCtrRx) {
         usbfs::waitForRxPacket();
         uint16_t size = readBd(n, CountRx) & CountMask;
         if(v & USB_EP_SETUP) {
            // Copy before CTR_RX is cleared: a control endpoint accepts the
            // next SETUP even while it answers NAK, which would overwrite it.
            usb::Event event{ EventType::Setup, n, 8, {} };
            copyFromPma(_endpoints[n].out.pmaAddress, event.setup.data(), 8);
            clearCtrRx(n);
            if(_callback != nullptr) {
               _callback(_context, event);
            }
         } else if(_endpoints[n].out.doubleBuffered) {
            // Clear CTR_RX before the SW_BUF toggle: the toggle lets the
            // next packet in, and its CTR_RX must stay set.
            clearCtrRx(n);
            receiveDoubleBuffered(n, v);
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
      // FSUSP stops further SUSP interrupts while suspended (RM0367 32.5.5,
      // RM0008 23.4.5).
      usbfs::regs().CNTR = static_cast<uint16_t>(usbfs::regs().CNTR | CntrSuspend);
      usbfs::regs().ISTR = static_cast<uint16_t>(~USB_ISTR_SUSP);
      report(EventType::Suspend, 0);
   }

   if(istr & USB_ISTR_WKUP) {
      usbfs::regs().CNTR = static_cast<uint16_t>(usbfs::regs().CNTR & ~(CntrSuspend | usbfs::CntrLowPowerMode));
      usbfs::regs().ISTR = static_cast<uint16_t>(~USB_ISTR_WKUP);
      report(EventType::Resume, 0);
   }

   // Masked sources can still be flagged; clear them so they stay quiet.
   usbfs::regs().ISTR = static_cast<uint16_t>(~(USB_ISTR_ERR | USB_ISTR_PMAOVR | USB_ISTR_SOF | USB_ISTR_ESOF));
}

} // namespace hw

#endif

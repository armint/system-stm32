#pragma once

#include "peripherals/UsbDeviceTypes.h"
#include "utils/Span.h"
#include "UsbFsFamily.h"
#include <cmsis_device.h>
#include <cstdint>

// The access layer defines SYSTEM_STM32_USBFS only on devices with the USB
// peripheral (STM32L0: L05x/L06x/L07x/L08x except L051/L071/L081; STM32F1:
// F102, F103; STM32H5: all). On the others this header is empty.
#if defined(SYSTEM_STM32_USBFS)

namespace hw {

// USB full-speed device peripheral: the USB block with USB_EPnR registers
// (STM32L0: RM0367 section 32; STM32F1: RM0008 section 23) and the USB DRD
// block in Device mode (STM32H5: RM0481 section 61). One driver for all
// these families. The family access layer
// (families/stm32<family>/UsbFsFamily.h, namespace hw::usbfs) has the
// differences:
//  - Packet memory: STM32L0 1024 bytes at a 16-bit stride. STM32F1 512
//    bytes; the CPU sees each 16-bit word at a 32-bit stride
//    (USB_PMAADDR + offset * 2). STM32H5 2048 bytes, 32-bit access only,
//    word-aligned buffers, 32-bit buffer descriptors. Buffer descriptors
//    and endpoint buffer addresses stay USB-local offsets.
//  - Endpoint registers: 16-bit USB_EPnR, or 32-bit USB_CHEPnR (STM32H5)
//    with the same low 16 bits.
//  - Interrupt: USB_IRQn (STM32L0), USB_LP_CAN1_RX0_IRQn (STM32F1, the
//    packet memory is shared with bxCAN, so do not use CAN at the same
//    time), USB_DRD_FS_IRQn (STM32H5).
//  - D+ pull-up: internal USB_BCDR.DPPU (STM32L0, STM32H5). STM32F1 has no
//    USB_BCDR: connect() and disconnect() call a board callback that drives
//    an external pull-up (see setPullupCallback()).
//  - Power-up: STM32H5 validates the VDDUSB supply in begin().
//  - Register names. LPM and BCD are not used.
//
// Packet level only: one call moves at most one packet of maxPacketSize
// bytes. Control transfers, multi-packet transfers and zero-length packets
// are the job of the layer above. The interrupt handler only decodes the
// hardware events, copies SETUP packets out of packet memory, and reports
// each event through the EventCallback.
//
// Flow control: an OUT endpoint answers NAK until armOut(), and the hardware
// sets it back to NAK after each received packet. An IN endpoint answers NAK
// until startIn(). readOut() can run in the interrupt callback or later in
// the main loop, until the next armOut() of that endpoint:
//  - Bulk and interrupt endpoints: the data stays in packet memory, because
//    the endpoint answers NAK until armOut().
//  - EP0 (the only control endpoint): a control endpoint accepts a new SETUP
//    while it answers NAK, and that SETUP overwrites the EP0 OUT buffer. So
//    the interrupt handler copies each EP0 OUT packet into a 64-byte buffer
//    in RAM, and readOut(0x00) reads from that copy.
//
// After a SETUP packet the hardware sets both directions of that control
// endpoint to NAK (RM0367 32.5.2, RM0008 23.4.2 "Control transfers"). The
// layer above then calls startIn(), armOut() or setStall() for the data
// and status stages.
//
// Requirements (see README "USB device driver"):
//  - 48 MHz USB clock, and Rcc::enableUSBClock() before begin().
//    STM32L0: Rcc::enableHSI48(), Rcc::setUSBClockSource(HSI48), CRS on
//    USB SOF. STM32F1: from the PLL (72 MHz / 1.5 or 48 MHz / 1), e.g.
//    with Rcc::setClock72MhzHSE() before Rcc::enableUSBClock(): USBPRE must
//    be valid before the USB clock is enabled and cannot change while it
//    is enabled (RM0008 7.3.2).
//  - Against packet memory overrun/underrun: STM32L0 APB clock >= 10 MHz
//    (RM0367 32.5.2, errata ES0292 2.16.1), STM32F1 APB1 clock > 8 MHz
//    (RM0008 23.4.2, "Structure and usage of packet buffers").
//  - PA11/PA12 need no alternate-function setup; the transceiver takes them
//    when the peripheral is powered up.
//  - STM32F1: a D+ pull-up of 1.5 kOhm to 3.3 V on the board, switched by a
//    GPIO through setPullupCallback(), or fixed.
//
// Concurrency: every public function may run in the event callback (USB
// interrupt) or in the main loop. openEndpoint(), closeAllEndpoints(),
// startIn(), armOut(), readOut(), setStall() and setNak() mask the USB
// interrupt (usbfs::Irq) while they run (Interrupt::BlockingSection), so a
// bus RESET in the interrupt cannot interleave with them. begin() and end()
// are for the main loop only.
//
// USB_EPnR STAT bits are toggle bits, so a status change is a read and an
// XOR write. If the hardware completes a transaction between the read and
// the write, the result is wrong. This can happen only on a VALID endpoint.
// setStall() and setNak() are race-free on an endpoint that is not VALID
// (NAK after Out/InComplete/Setup, or not armed yet). Do not use them to
// cancel a pending startIn() or armOut().
//
// Double-buffered bulk OUT (RM0367 32.5.3, RM0008 23.4.3): openEndpoint() with
// doubleBuffered true uses both buffer descriptors of the endpoint number.
// The hardware receives the next packet into the second buffer while the
// software still reads the first. The interface stays the same: the driver
// reports one Out event per armOut(), and readOut() reads that packet. A
// packet that arrives before armOut() waits in the second buffer, and
// armOut() reports it. The endpoint number then has no IN direction.
//
// Limits: 8 endpoint registers (endpoint numbers 0..7), PacketMemorySize
// bytes of packet memory, no isochronous endpoints, no double-buffered IN
// endpoints.
class UsbDevice {
   public:
   static constexpr uint8_t EndpointCount = 8;
   static constexpr uint16_t PacketMemorySize = usbfs::PacketMemorySize;
   static constexpr uint16_t Ep0MaxPacketSize = 64;

   // Drives an external D+ pull-up: enable true = pull-up on (attach).
   using PullupCallback = void (*)(void *context, bool enable);

   /**
    * @param endpointCount The highest endpoint number that the device uses,
    * plus 1 (1..EndpointCount). The buffer descriptor table has 8 bytes per
    * endpoint number, so a smaller count leaves more packet memory for the
    * endpoint buffers. openEndpoint() refuses higher endpoint numbers.
    */
   explicit UsbDevice(uint8_t endpointCount = EndpointCount);
   UsbDevice(const UsbDevice &) = delete;
   UsbDevice &operator=(const UsbDevice &) = delete;

   /**
    * @brief Powers up the transceiver, resets the peripheral, connects the
    * interrupt and enables it in the NVIC. The device stays disconnected
    * (no D+ pull-up) until connect(). The USB bus clock must be enabled.
    */
   void begin(usb::EventCallback callback, void *context);

   /**
    * @brief Disconnects, disables the interrupt and powers down the
    * transceiver.
    */
   void end();

   /**
    * @brief Sets the board function that switches an external D+ pull-up.
    * Call it before connect(). connect() and disconnect() call it after
    * they set the internal pull-up (STM32L0). The STM32F1 has no internal
    * pull-up: without a callback, connect() and disconnect() only change
    * isConnected(); use this for a board with a fixed pull-up.
    *
    * On STM32F1, isConnected() reports the state that
    * connect()/disconnect() requested, not the bus state. With a fixed
    * pull-up, the host sees the device from power-on, before begin(), while
    * isConnected() is false until connect().
    */
   void setPullupCallback(PullupCallback callback, void *context);

   // D+ pull-up on/off: the host sees the device attach/detach. STM32L0:
   // USB_BCDR.DPPU, and isConnected() reads it back. Then the
   // PullupCallback, if set.
   void connect();
   void disconnect();
   bool isConnected() const;

   /**
    * @brief Writes the device address (USB_DADDR). Call it after the status
    * stage of SET_ADDRESS, not before (USB 2.0 section 9.4.6).
    */
   void setAddress(uint8_t address);

   /**
    * @brief Opens one direction of an endpoint and allocates its buffer in
    * packet memory. The endpoint starts with data toggle DATA0 and answers
    * NAK. Both directions of one endpoint number share one register and must
    * have the same type.
    * @param doubleBuffered true: a double-buffered bulk OUT endpoint with two
    * buffers of maxPacketSize (see the class comment). The IN direction of
    * the number stays closed.
    * @return false for endpoint number >= the endpointCount of the
    * constructor, isochronous type,
    * control type on an endpoint other than 0, maxPacketSize 0 or > 64 (the full-speed limit for control, bulk and
    * interrupt endpoints), a type that does not match the other direction,
    * a direction that is already open with another maxPacketSize or another
    * doubleBuffered (call closeAllEndpoints() first), or not enough packet
    * memory. With doubleBuffered: also false for an IN or a non-bulk
    * endpoint, or when the IN direction of the number is open. An IN
    * direction cannot open on the number of a double-buffered endpoint.
    */
   [[nodiscard]] bool openEndpoint(uint8_t address, usb::EndpointType type, uint16_t maxPacketSize, bool doubleBuffered = false);

   /**
    * @brief Disables every endpoint except EP0 and frees their packet
    * memory. For SET_CONFIGURATION: close all, then open the new ones.
    */
   void closeAllEndpoints();

   /**
    * @brief Copies one packet into the IN buffer and makes the endpoint
    * VALID. data.size() 0 sends a zero-length packet.
    * @return false if the endpoint is not open, data is larger than
    * maxPacketSize, or the previous packet is not sent yet (STAT_TX VALID).
    */
   [[nodiscard]] bool startIn(uint8_t address, hw::span<const uint8_t> data);

   /**
    * @brief Makes an OUT endpoint VALID, so it accepts the next packet.
    * Double-buffered: frees the buffer of the reported packet. If the next
    * packet is already in the other buffer, it reports that packet (Out
    * event) before it returns.
    * @return false if the endpoint is not open.
    */
   bool armOut(uint8_t address);

   /**
    * @brief Copies the last received OUT packet (EP0: from the RAM copy, see
    * the class comment).
    * @return the number of bytes copied: min(received size, buffer.size()).
    */
   uint16_t readOut(uint8_t address, hw::span<uint8_t> buffer) const;

   /**
    * @brief Sets or clears STALL on one direction of an endpoint. Clearing
    * STALL sets NAK and resets the data toggle to DATA0 (CLEAR_FEATURE
    * ENDPOINT_HALT, USB 2.0 section 9.4.5). A STALL on EP0 ends with the
    * next SETUP packet. Race-free only if the endpoint is not VALID (see the
    * class comment). Double-buffered OUT: clearing STALL drops both
    * buffers. readOut() then returns 0, also for a packet that was
    * reported but not read yet.
    */
   void setStall(uint8_t address, bool stall);
   bool isStalled(uint8_t address) const;

   /**
    * @brief Sets NAK on one direction of an endpoint. Race-free only if the
    * endpoint is not VALID (see the class comment).
    */
   void setNak(uint8_t address);

   /**
    * @brief The interrupt handler. begin() connects it to usbfs::Irq
    * through Interrupt::connectMemberHandler; do not call it directly.
    */
   void handleInterrupt();

   // Buffer descriptor table helpers, constexpr so they can be checked at
   // compile time (see static_asserts in UsbDeviceCore.cpp).
   struct RxBufferSize {
      uint16_t count;    // USB_COUNTn_RX value (BLSIZE and NUM_BLOCK)
      uint16_t pmaBytes; // bytes allocated in packet memory
   };

   static constexpr RxBufferSize rxBufferSize(uint16_t maxPacketSize) {
      // BLSIZE = 0: 2-byte blocks, NUM_BLOCK 1..31 (2..62 bytes).
      // BLSIZE = 1: 32-byte blocks, NUM_BLOCK + 1 blocks (32..1024 bytes).
      if(maxPacketSize <= 62) {
         uint16_t blocks = (maxPacketSize + 1) / 2;
         if(blocks == 0) {
            blocks = 1;
         }
         return { static_cast<uint16_t>(blocks << 10), static_cast<uint16_t>(blocks * 2) };
      }
      uint16_t blocks = (maxPacketSize + 31) / 32;
      return { static_cast<uint16_t>(0x8000u | ((blocks - 1) << 10)), static_cast<uint16_t>(blocks * 32) };
   }

   private:
   static constexpr uint8_t NoBuffer = 0xFF;

   struct Direction {
      uint16_t pmaAddress = 0;
      uint16_t maxPacketSize = 0;
      bool open = false;
      // Double-buffered OUT only. pmaAddress is buffer 0 (the TX
      // descriptor), pmaAddress1 buffer 1 (the RX descriptor).
      bool doubleBuffered = false;
      bool armed = false;
      uint16_t pmaAddress1 = 0;
      // The buffer of the reported packet, until armOut()
      uint8_t reportedBuffer = NoBuffer;
      // The buffer of a received packet that waits for armOut()
      uint8_t pendingBuffer = NoBuffer;
   };

   struct Endpoint {
      usb::EndpointType type = usb::EndpointType::Control;
      Direction in;
      Direction out;
   };

   usb::EventCallback _callback = nullptr;
   void *_context = nullptr;
   PullupCallback _pullupCallback = nullptr;
   void *_pullupContext = nullptr;
   bool _connected = false;
   Endpoint _endpoints[EndpointCount] = {};
   uint8_t _endpointCount;
   uint16_t _btableSize;
   uint16_t _pmaNext;
   uint8_t _ep0Out[Ep0MaxPacketSize] = {};
   uint16_t _ep0OutSize = 0;

   void resetEndpoints();
   void report(usb::EventType type, uint8_t endpoint, uint16_t size = 0);
   void resetDoubleBuffer(uint8_t n);
   void receiveDoubleBuffered(uint8_t n, uint16_t epr);
   void reportDoubleBuffered(uint8_t n, uint8_t buffer);
   uint16_t allocate(uint16_t bytes);
   Direction *direction(uint8_t address);
   const Direction *direction(uint8_t address) const;
};

} // namespace hw

#endif

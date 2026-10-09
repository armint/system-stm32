#pragma once

#include "peripherals/UsbDeviceTypes.h"
#include "utils/Span.h"
#include <cmsis_device.h>
#include <cstdint>

// Only F1 devices with a USB peripheral (F102, F103) define USB_BASE; on
// the others this header is empty.
#if defined(USB_BASE)

namespace hw {

// STM32F1 USB full-speed device peripheral (RM0008 section 23).
//
// The same peripheral as on STM32L0 (families/stm32l0/UsbDevice), with the
// same interface and event model. The differences:
//  - 512 bytes of packet memory. The CPU sees each 16-bit word at a 32-bit
//    stride (USB_PMAADDR + offset * 2). Buffer descriptors and endpoint
//    buffer addresses stay USB-local offsets.
//  - No USB_BCDR, so no internal D+ pull-up. connect() and disconnect()
//    call a board callback that drives an external pull-up (see
//    setPullupCallback()).
//  - The interrupt is USB_LP_CAN1_RX0_IRQn. The packet memory is shared
//    with bxCAN, so do not use CAN at the same time.
//  - No LPM and no BCD.
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
// endpoint to NAK (RM0008 23.4.2 "Control transfers"). The layer above
// then calls startIn(), armOut() or setStall() for the data and status
// stages.
//
// Requirements (see README "USB device driver"):
//  - 48 MHz USB clock from the PLL (72 MHz / 1.5 or 48 MHz / 1), e.g. with
//    Rcc::setClock72MhzHSE(), and Rcc::enableUSBClock() before begin().
//  - Rcc::setClock72MhzHSE() before Rcc::enableUSBClock(): USBPRE must be
//    valid before the USB clock is enabled and cannot change while it is
//    enabled (RM0008 7.3.2).
//  - APB1 clock > 8 MHz, against packet memory overrun/underrun (RM0008
//    23.4.2, "Structure and usage of packet buffers").
//  - PA11/PA12 need no alternate-function setup; the transceiver takes them
//    when the peripheral is powered up.
//  - A D+ pull-up of 1.5 kOhm to 3.3 V on the board, switched by a GPIO
//    through setPullupCallback(), or fixed.
//
// Concurrency: every public function may run in the event callback (USB
// interrupt) or in the main loop. openEndpoint(), closeAllEndpoints(),
// startIn(), armOut(), readOut(), setStall() and setNak() mask
// USB_LP_CAN1_RX0_IRQn while they run (Interrupt::BlockingSection), so a
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
// Double-buffered bulk OUT (RM0008 23.4.3): openEndpoint() with
// doubleBuffered true uses both buffer descriptors of the endpoint number.
// The hardware receives the next packet into the second buffer while the
// software still reads the first. The interface stays the same: the driver
// reports one Out event per armOut(), and readOut() reads that packet. A
// packet that arrives before armOut() waits in the second buffer, and
// armOut() reports it. The endpoint number then has no IN direction.
//
// Limits: 8 endpoint registers (endpoint numbers 0..7), 512 bytes of packet
// memory, no isochronous endpoints, no double-buffered IN endpoints.
class UsbDevice {
   public:
   static constexpr uint8_t EndpointCount = 8;
   static constexpr uint16_t PacketMemorySize = 512;
   static constexpr uint16_t Ep0MaxPacketSize = 64;

   // Drives the external D+ pull-up: enable true = pull-up on (attach).
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
    * @brief Sets the board function that switches the D+ pull-up. Call it
    * before connect(). Without a callback, connect() and disconnect() only
    * change isConnected(); use this for a board with a fixed pull-up.
    *
    * isConnected() reports the state that connect()/disconnect() requested,
    * not the bus state. With a fixed pull-up, the host sees the device from
    * power-on, before begin(), while isConnected() is false until
    * connect().
    */
   void setPullupCallback(PullupCallback callback, void *context);

   // External D+ pull-up through the PullupCallback: the host sees the
   // device attach.
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
    * @brief The interrupt handler. begin() connects it to
    * USB_LP_CAN1_RX0_IRQn through
    * Interrupt::connectMemberHandler; do not call it directly.
    */
   void handleInterrupt();

   // Buffer descriptor table helpers, constexpr so they can be checked at
   // compile time (see static_asserts in UsbDevice.cpp).
   struct RxBufferSize {
      uint16_t count;    // USB_COUNTn_RX value (BLSIZE and NUM_BLOCK)
      uint16_t pmaBytes; // bytes allocated in packet memory
   };

   static constexpr RxBufferSize rxBufferSize(uint16_t maxPacketSize) {
      // BLSIZE = 0: 2-byte blocks, NUM_BLOCK 1..31 (2..62 bytes).
      // BLSIZE = 1: 32-byte blocks, NUM_BLOCK + 1 blocks (32..512 bytes).
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

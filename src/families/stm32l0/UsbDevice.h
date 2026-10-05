#pragma once

#include "peripherals/UsbDeviceTypes.h"
#include <cmsis_device.h>
#include <cstdint>
#include <span>

// Only L0 devices with a USB peripheral (L05x/L06x/L07x/L08x except
// L051/L071/L081) define USB_BASE; on the others this header is empty.
#if defined(USB_BASE)

namespace hw {

// STM32L0 USB full-speed device peripheral (RM0367 section 32).
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
// endpoint to NAK (RM0367 32.5.2 "Control transfers"). The layer above then
// calls startIn(), armOut() or setStall() for the data and status stages.
//
// Requirements (see README "USB device driver"):
//  - 48 MHz USB clock: Rcc::enableHSI48(), Rcc::setUSBClockSource(HSI48),
//    CRS on USB SOF. Rcc::enableUSBClock() before begin().
//  - APB clock >= 10 MHz (RM0367 32.5.2).
//  - PA11/PA12 need no alternate-function setup; the transceiver takes them
//    when the peripheral is powered up.
//
// Limits: 8 endpoint registers (endpoint numbers 0..7), 1024 bytes of packet
// memory, no isochronous and no double-buffered endpoints.
class UsbDevice {
   public:
   static constexpr uint8_t EndpointCount = 8;
   static constexpr uint16_t PacketMemorySize = 1024;
   static constexpr uint16_t Ep0MaxPacketSize = 64;

   UsbDevice() = default;
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

   // Internal D+ pull-up (USB_BCDR.DPPU): the host sees the device attach.
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
    * @return false for endpoint number >= EndpointCount, isochronous type,
    * control type on an endpoint other than 0, maxPacketSize 0 or > 64 (the full-speed limit for control, bulk and
    * interrupt endpoints), a type that does not match the other direction,
    * or not enough packet memory.
    */
   [[nodiscard]] bool openEndpoint(uint8_t address, usb::EndpointType type, uint16_t maxPacketSize);

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
   [[nodiscard]] bool startIn(uint8_t address, std::span<const uint8_t> data);

   /**
    * @brief Makes an OUT endpoint VALID, so it accepts the next packet.
    * @return false if the endpoint is not open.
    */
   bool armOut(uint8_t address);

   /**
    * @brief Copies the last received OUT packet (EP0: from the RAM copy, see
    * the class comment).
    * @return the number of bytes copied: min(received size, buffer.size()).
    */
   uint16_t readOut(uint8_t address, std::span<uint8_t> buffer) const;

   /**
    * @brief Sets or clears STALL on one direction of an endpoint. Clearing
    * STALL sets NAK and resets the data toggle to DATA0 (CLEAR_FEATURE
    * ENDPOINT_HALT, USB 2.0 section 9.4.5). A STALL on EP0 ends with the
    * next SETUP packet.
    */
   void setStall(uint8_t address, bool stall);
   bool isStalled(uint8_t address) const;

   /**
    * @brief Sets NAK on one direction of an endpoint, for example to take
    * back a startIn() or armOut() that the host has not served yet.
    */
   void setNak(uint8_t address);

   /**
    * @brief The interrupt handler. begin() connects it to USB_IRQn through
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
   struct Direction {
      uint16_t pmaAddress = 0;
      uint16_t maxPacketSize = 0;
      bool open = false;
   };

   struct Endpoint {
      usb::EndpointType type = usb::EndpointType::Control;
      Direction in;
      Direction out;
   };

   static constexpr uint16_t BtableSize = EndpointCount * 8;

   usb::EventCallback _callback = nullptr;
   void *_context = nullptr;
   Endpoint _endpoints[EndpointCount] = {};
   uint16_t _pmaNext = BtableSize;
   uint8_t _ep0Out[Ep0MaxPacketSize] = {};
   uint16_t _ep0OutSize = 0;

   void resetEndpoints();
   void report(usb::EventType type, uint8_t endpoint, uint16_t size = 0);
   uint16_t allocate(uint16_t bytes);
   Direction *direction(uint8_t address);
   const Direction *direction(uint8_t address) const;
};

} // namespace hw

#endif

#pragma once

#include <array>
#include <cstdint>

// Family-neutral types of the USB device drivers (hw::UsbDevice). Each family
// with a USB device peripheral has its own hw::UsbDevice in
// src/families/<family>/UsbDevice.h with the same public interface. Code
// above the driver (for example the libart-stm32 controller) uses only these
// types and that interface, so a new family needs no change above the driver.
namespace hw::usb {

// Bit 7 of an endpoint address is the direction (USB 2.0 section 9.6.6).
constexpr uint8_t EndpointDirectionIn = 0x80;
constexpr uint8_t EndpointNumberMask = 0x0F;

constexpr uint8_t endpointNumber(uint8_t address) {
   return address & EndpointNumberMask;
}

constexpr bool isInEndpoint(uint8_t address) {
   return (address & EndpointDirectionIn) != 0;
}

enum class EndpointType : uint8_t {
   Control,
   Isochronous,
   Bulk,
   Interrupt,
};

enum class EventType : uint8_t {
   Reset,      // USB bus reset. EP0 is open again, the address is 0.
   Setup,      // SETUP packet on a control endpoint; Event::setup holds it.
   Out,        // OUT packet received; Event::size bytes wait in packet memory.
   InComplete, // IN packet sent and acknowledged by the host.
   Suspend,    // No bus activity for 3 ms (host suspend or cable unplugged).
   Resume,     // Bus activity after Suspend.
};

struct Event {
   EventType type;
   uint8_t endpoint;             // Endpoint address with direction bit; 0 for bus events.
   uint16_t size;                // Out: received byte count. Setup: 8.
   std::array<uint8_t, 8> setup; // Setup: the 8 bytes of the SETUP packet.
};

// Called from the USB interrupt. Keep it short: copy the event into a queue.
using EventCallback = void (*)(void *context, const Event &event);

} // namespace hw::usb

#pragma once

#include "GpioPin.h"
#include <cstdint>

namespace hw {

/**
 * @brief GPIO pin with the port and pin fixed at compile time.
 *
 * For tight bit-bang loops (e.g. JTAG/SWD). Each access is one store to
 * BSRR or one load from IDR, with a constant address and mask, and no
 * read-modify-write. FastPin does not configure the pin; use pin() to get
 * a GpioPin for configure().
 *
 * Example: using Tck = hw::FastPin<GPIOA_BASE, 5>;
 *
 * @tparam PortBase the GPIO port base address, e.g. GPIOA_BASE.
 * @tparam Pin the pin number (0-15).
 */
template <uintptr_t PortBase, uint8_t Pin>
struct FastPin {
   static_assert(Pin < 16, "GPIO pin number must be 0-15");

   static constexpr uint32_t mask = 1UL << Pin;

   [[gnu::always_inline]] static inline GPIO_TypeDef *port() {
      return reinterpret_cast<GPIO_TypeDef *>(PortBase);
   }

   [[gnu::always_inline]] static inline void set() {
      port()->BSRR = mask;
   }

   // The upper half of BSRR does the same as BRR, and BSRR exists on
   // every supported family.
   [[gnu::always_inline]] static inline void clear() {
      port()->BSRR = mask << 16;
   }

   // Branch-free: when both the set and the reset bit are 1, set wins.
   [[gnu::always_inline]] static inline void write(bool state) {
      port()->BSRR = (mask << 16) | (static_cast<uint32_t>(state) << Pin);
   }

   [[gnu::always_inline]] [[nodiscard]] static inline bool read() {
      return (port()->IDR & mask) != 0;
   }

   static GpioPin pin() {
      return { port(), Pin };
   }
};

} // namespace hw

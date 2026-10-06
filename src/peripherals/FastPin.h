#pragma once

#include "GpioPin.h"
#include <cstdint>
#include <utility>

#if defined(GPIO_CRL_MODE0)
#include "GpioConfig.h" // STM32F1: families/stm32f1/GpioConfig.h
#endif

namespace hw {

/**
 * @brief GPIO pin with the port and pin fixed at compile time.
 *
 * For tight bit-bang loops (e.g. JTAG/SWD). Each access is one store to
 * BSRR or one load from IDR, with a constant address and mask, and no
 * read-modify-write. Use pin() to get a GpioPin for configure().
 *
 * STM32F1 only: setOutput(), setInput() and setConfig<>() switch the pin
 * configuration (e.g. the SWDIO direction at an SWD turnaround). Each one
 * is four stores to the bit-band aliases of the pin's CNF/MODE bits in
 * CRL/CRH, with constant addresses: no function call, no branch, no
 * read-modify-write, and safe against interrupts that change other pins
 * of the same register. See gpio_f1::writeOrder() for the intermediate
 * states.
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

#if defined(GPIO_CRL_MODE0)
   // Push-pull output, 50 MHz.
   static constexpr uint8_t OutputConfig =
   gpio_f1::pinConfig(GpioPin::Mode::Output, GpioPin::OutputType::PushPull, GpioPin::Pull::None, GpioPin::Speed::VeryHigh);
   // Floating input.
   static constexpr uint8_t InputConfig = gpio_f1::pinConfig(GpioPin::Mode::Input);

   /**
    * @brief Writes the 4-bit CNF/MODE field of this pin, one bit-band store
    * per bit. ODR (the output level or the pull direction) does not change.
    * @tparam Config a field value, e.g. gpio_f1::pinConfig(...).
    */
   template <uint8_t Config>
   [[gnu::always_inline]] static inline void setConfig() {
      static_assert(Config <= 0xF, "CNF/MODE field is 4 bits");
      static_assert(Config != 0b1100, "CNF 11 with MODE 00 is reserved (RM0008 9.2.1)");
      writeBits<Config>(std::make_index_sequence<4>{});
   }

   [[gnu::always_inline]] static inline void setOutput() {
      setConfig<OutputConfig>();
   }

   [[gnu::always_inline]] static inline void setInput() {
      setConfig<InputConfig>();
   }

   private:
   static constexpr uintptr_t configRegister = PortBase + gpio_f1::configRegisterOffset(Pin);

   [[gnu::always_inline]] static inline void storeWord(uintptr_t address, uint32_t value) {
      *reinterpret_cast<volatile uint32_t *>(address) = value;
   }

   template <uint8_t Config, std::size_t... I>
   [[gnu::always_inline]] static inline void writeBits(std::index_sequence<I...>) {
      constexpr auto order = gpio_f1::writeOrder(Config);
      (storeWord(gpio_f1::bitBandAlias(configRegister, gpio_f1::configShift(Pin) + order[I].bit), order[I].value), ...);
   }
#endif
};

} // namespace hw

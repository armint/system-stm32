
#pragma once

#include <cmsis_device.h>
#include <cstdint>

class GpioPin {
   public:
   // GPIO_MODER
   enum class Mode : uint8_t { Input = 0, Output = 1, Alternate = 2, Analog = 3 };

   // GPIO_OTYPER
   enum class OutputType : bool { PushPull = false, OpenDrain = true };

   // GPIO_OSPEEDR
   enum class Speed : uint8_t {
      Low = 0,     // ~400 kHz
      Medium = 1,  // ~2 MHz
      High = 2,    // ~10 MHz
      VeryHigh = 3 // ~40 MHz
   };

   // GPIO_PUPDR
   enum class Pull : uint8_t { None = 0, Up = 1, Down = 2 };

   GPIO_TypeDef * const port;
   const uint8_t pin;

   /**
    * @brief Constructor
    * @param port GPIO Port (e.g., GPIOA, GPIOB)
    * @param pin Pin number (0-15)
    */
   GpioPin(GPIO_TypeDef *port, uint8_t pin)
      : port(port)
      , pin(pin) {
   }

   // =============================================================
   // Configuration
   // =============================================================
   void setMode(Mode mode) const;
   void setOutputType(OutputType type) const;
   void setSpeed(Speed speed) const;
   void setPull(Pull pull) const;

   /**
    * @brief Sets the Alternate Function index (0-15).
    * @note Only relevant if Mode is set to Alternate.
    */
   void setAlternateFunction(uint8_t afIndex) const;

   // Helper to setup everything in one go
   void configure(Mode mode, OutputType type = OutputType::PushPull, Pull pull = Pull::None, Speed speed = Speed::Low) const;

   // =============================================================
   // Control
   // =============================================================
   // Atomic Set/Reset using BSRR register (No read-modify-write)
   void set() const {
      port->BSRR = (1UL << pin);
   }

   void clear() const {
      port->BSRR = (1UL << (pin + 16));
   }

   void write(bool state) const {
      if(state)
         set();
      else
         clear();
   }

   // Toggle using ODR (Read-Modify-Write)
   void toggle() const {
      port->ODR ^= (1UL << pin);
   }

   // =============================================================
   // Input
   // =============================================================
   [[nodiscard]] bool read() const {
      return (port->IDR & (1UL << pin)) != 0;
   }
};

// =============================================================================
// Zero-RAM, header-only GPIO pin for maximal RAM savings
// Usage example:
//   using Led = GpioPinStatic<GPIOA, 5>;
//   Led::set();
// This template stores no state; all operations are static and optimized away.
// May slightly increase flash due to template instantiations; enable LTO to
// allow the linker to merge identical code sequences across pins.
// =============================================================================
template<GPIO_TypeDef *Port, uint8_t Pin>
struct GpioPinStatic {
   // Keep enums identical to GpioPin to ease migration
   enum class Mode : uint8_t { Input = 0, Output = 1, Alternate = 2, Analog = 3 };
   enum class OutputType : bool { PushPull = false, OpenDrain = true };
   enum class Speed : uint8_t { Low = 0, Medium = 1, High = 2, VeryHigh = 3 };
   enum class Pull : uint8_t { None = 0, Up = 1, Down = 2 };

   // Configuration
   static void setMode(Mode mode) {
      const uint32_t pos = static_cast<uint32_t>(Pin) * 2u;
      const uint32_t mask = 3ul << pos;
      const uint32_t val = static_cast<uint32_t>(mode) << pos;
      uint32_t tmp = Port->MODER;
      tmp &= ~mask;
      tmp |= val;
      Port->MODER = tmp;
   }

   static void setOutputType(OutputType type) {
      const uint32_t pos = static_cast<uint32_t>(Pin);
      const uint32_t mask = 1ul << pos;
      const uint32_t val = static_cast<uint32_t>(type) << pos;
      uint32_t tmp = Port->OTYPER;
      tmp &= ~mask;
      tmp |= val;
      Port->OTYPER = tmp;
   }

   static void setSpeed(Speed speed) {
      const uint32_t pos = static_cast<uint32_t>(Pin) * 2u;
      const uint32_t mask = 3ul << pos;
      const uint32_t val = static_cast<uint32_t>(speed) << pos;
      uint32_t tmp = Port->OSPEEDR;
      tmp &= ~mask;
      tmp |= val;
      Port->OSPEEDR = tmp;
   }

   static void setPull(Pull pull) {
      const uint32_t pos = static_cast<uint32_t>(Pin) * 2u;
      const uint32_t mask = 3ul << pos;
      const uint32_t val = static_cast<uint32_t>(pull) << pos;
      uint32_t tmp = Port->PUPDR;
      tmp &= ~mask;
      tmp |= val;
      Port->PUPDR = tmp;
   }

   static void setAlternateFunction(uint8_t afIndex) {
      const uint32_t regIndex = static_cast<uint32_t>(Pin) >> 3; // 0 for 0-7, 1 for 8-15
      const uint32_t pos = (static_cast<uint32_t>(Pin) & 0x07u) * 4u; // 4 bits per AF
      const uint32_t mask = 0xFul << pos;
      const uint32_t val = (static_cast<uint32_t>(afIndex) & 0xFu) << pos;
      uint32_t tmp = Port->AFR[regIndex];
      tmp &= ~mask;
      tmp |= val;
      Port->AFR[regIndex] = tmp;
   }

   static void configure(Mode mode,
                                OutputType type = OutputType::PushPull,
                                Pull pull = Pull::None,
                                Speed speed = Speed::Low) {
      setMode(mode);
      setOutputType(type);
      setPull(pull);
      setSpeed(speed);
   }

   // Control
   static void set() { Port->BSRR = (1ul << Pin); }
   static void clear() { Port->BSRR = (1ul << (Pin + 16u)); }
   static void write(bool state) { state ? set() : clear(); }
   static void toggle() { Port->ODR ^= (1ul << Pin); }

   // Input
   [[nodiscard]] static bool read() { return (Port->IDR & (1ul << Pin)) != 0; }
};

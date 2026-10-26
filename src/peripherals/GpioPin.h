
#pragma once

#include <cmsis_device.h>
#include <cstdint>

namespace hw {

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

   // Alternate Function selection (AFR)
   enum class AlternateFunction : uint8_t {
      AF0 = 0,
      AF1 = 1,
      AF2 = 2,
      AF3 = 3,
      AF4 = 4,
      AF5 = 5,
      AF6 = 6,
      AF7 = 7,
      AF8 = 8,
      AF9 = 9,
      AF10 = 10,
      AF11 = 11,
      AF12 = 12,
      AF13 = 13,
      AF14 = 14,
      AF15 = 15
   };

   GPIO_TypeDef *const port;
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

   GpioPin(const GpioPin &) = default;
   GpioPin(GpioPin &&) = default;
   GpioPin &operator=(const GpioPin &) = delete;
   GpioPin &operator=(GpioPin &&) = delete;

   // =============================================================
   // Configuration
   // =============================================================
   void setMode(Mode mode) const;
   void setOutputType(OutputType type) const;
   void setSpeed(Speed speed) const;
   void setPull(Pull pull) const;

   /**
    * @brief Sets the Alternate Function.
    * @note Only relevant if Mode is set to Alternate.
    */
   void setAlternateFunction(AlternateFunction af) const;

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

   // Toggle: read ODR, then set or reset the pin through BSRR. ODR itself is
   // not written, so a change to another pin of the port in an interrupt
   // between the read and the write is not lost.
   void toggle() const {
      uint32_t mask = 1UL << pin;
      uint32_t odr = port->ODR;
      port->BSRR = ((odr & mask) << 16) | (~odr & mask);
   }

   // =============================================================
   // Input
   // =============================================================
   [[nodiscard]] bool read() const {
      return (port->IDR & (1UL << pin)) != 0;
   }
};

} // namespace hw

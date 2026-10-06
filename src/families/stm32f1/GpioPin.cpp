#include "peripherals/GpioPin.h"
#include "GpioConfig.h"
#include "utils/Interrupt.h"
#include <cassert>

// STM32F1 GpioPin implementation (RM0008 section 9.1). The CNF/MODE field
// encoding is in GpioConfig.h.
//
// The setters change only the part of the field that they own, so they
// depend on the current mode. Call setMode() first, as configure() does.
//
// Interrupt safety: CRL and CRH hold the fields of 8 pins each, so every
// change is a read-modify-write of a shared register. Each one runs in a
// short critical section (PRIMASK), so a change in an interrupt to another
// pin of the same register is not lost.

namespace hw {

namespace {

using namespace gpio_f1;

struct PinConfig {
   uint8_t cnf;
   uint8_t mode;
};

volatile uint32_t &configRegister(GPIO_TypeDef *port, uint8_t pin) {
   return pin < 8 ? port->CRL : port->CRH;
}

/**
 * Reads the field of one pin, gives it to @p change, and writes the result
 * back. The whole read-modify-write runs with interrupts disabled. @p change
 * returns false to leave the register unchanged.
 */
template <typename Change>
void updateConfig(GPIO_TypeDef *port, uint8_t pin, Change change) {
   volatile uint32_t &reg = configRegister(port, pin);
   uint32_t shift = configShift(pin);
   Interrupt::CriticalSection cs;
   uint32_t value = reg;
   uint32_t old = (value >> shift) & 0xFU;
   PinConfig config{ static_cast<uint8_t>(old >> 2), static_cast<uint8_t>(old & 0x3U) };
   if(!change(config)) {
      return;
   }
   value &= ~(0xFUL << shift);
   value |= static_cast<uint32_t>(field(config.cnf, config.mode)) << shift;
   reg = value;
}

// Selects pull-up or pull-down through ODR (BSRR/BRR: no read-modify-write).
// Only has an effect in input pull mode.
void setPullDirection(GPIO_TypeDef *port, uint8_t pin, GpioPin::Pull pull) {
   if(pull == GpioPin::Pull::Up) {
      port->BSRR = 1UL << pin;
   } else {
      port->BRR = 1UL << pin;
   }
}

} // namespace

void GpioPin::setMode(Mode mode) const {
   updateConfig(port, pin, [mode](PinConfig &config) {
      bool isOutput = config.mode != MODE_INPUT;
      switch(mode) {
      case Mode::Input:
         // Keep an existing pull; otherwise start floating.
         config.cnf = (!isOutput && config.cnf == CNF_INPUT_PULL) ? CNF_INPUT_PULL : CNF_INPUT_FLOATING;
         config.mode = MODE_INPUT;
         break;
      case Mode::Analog:
         config = { CNF_INPUT_ANALOG, MODE_INPUT };
         break;
      case Mode::Output:
      case Mode::Alternate:
         // Keep the speed and the open-drain bit of an existing output.
         config.cnf = isOutput ? (config.cnf & CNF_OUTPUT_OPEN_DRAIN) : 0;
         config.mode = isOutput ? config.mode : speedToMode(Speed::Low);
         if(mode == Mode::Alternate) {
            config.cnf |= CNF_OUTPUT_ALTERNATE;
         }
         break;
      }
      return true;
   });
}

void GpioPin::setOutputType(OutputType type) const {
   updateConfig(port, pin, [type](PinConfig &config) {
      if(config.mode == MODE_INPUT) {
         return false; // Inputs have no output type.
      }
      config.cnf = (config.cnf & CNF_OUTPUT_ALTERNATE) | (type == OutputType::OpenDrain ? CNF_OUTPUT_OPEN_DRAIN : 0);
      return true;
   });
}

void GpioPin::setSpeed(Speed speed) const {
   updateConfig(port, pin, [speed](PinConfig &config) {
      if(config.mode == MODE_INPUT) {
         return false; // Inputs have no speed.
      }
      config.mode = speedToMode(speed);
      return true;
   });
}

void GpioPin::setPull(Pull pull) const {
   updateConfig(port, pin, [this, pull](PinConfig &config) {
      bool isDigitalInput = config.mode == MODE_INPUT && config.cnf != CNF_INPUT_ANALOG;
      if(!isDigitalInput) {
         // F1 outputs and analog inputs have no pull resistors.
         assert(pull == Pull::None);
         return false;
      }
      if(pull == Pull::None) {
         config.cnf = CNF_INPUT_FLOATING;
      } else {
         setPullDirection(port, pin, pull);
         config.cnf = CNF_INPUT_PULL;
      }
      return true;
   });
}

void GpioPin::setAlternateFunction(AlternateFunction af) const {
   // F1 has no AF number per pin. The peripheral takes the pin when the
   // pin is an alternate-function output; remaps go through hw::Afio.
   // Alternate-function inputs (e.g. USART RX, SPI MISO) stay in an
   // input mode.
   (void)af;
   updateConfig(port, pin, [](PinConfig &config) {
      if(config.mode == MODE_INPUT) {
         return false;
      }
      config.cnf |= CNF_OUTPUT_ALTERNATE;
      return true;
   });
}

void GpioPin::configure(Mode mode, OutputType type, Pull pull, Speed speed) const {
   // F1 outputs and analog inputs have no pull resistors.
   assert(pull == Pull::None || mode == Mode::Input);
   if(mode == Mode::Input && pull != Pull::None) {
      // Set the pull direction before the pin becomes a pull input.
      setPullDirection(port, pin, pull);
   }
   // Compute the whole field and write it once, so the pin never passes
   // through an intermediate configuration.
   uint8_t value = pinConfig(mode, type, pull, speed);
   updateConfig(port, pin, [value](PinConfig &config) {
      config = { static_cast<uint8_t>(value >> 2), static_cast<uint8_t>(value & 0x3U) };
      return true;
   });
}

} // namespace hw

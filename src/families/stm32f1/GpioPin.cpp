#include "peripherals/GpioPin.h"
#include <cassert>

// STM32F1 GpioPin implementation (RM0008 section 9.1).
//
// F1 has no MODER/OTYPER/OSPEEDR/PUPDR/AFR. Each pin has one 4-bit field
// in CRL (pins 0-7) or CRH (pins 8-15): CNF[1:0] in the upper two bits and
// MODE[1:0] in the lower two bits.
//
//   MODE = 00            input; CNF: 00 analog, 01 floating, 10 pull-up/down
//   MODE = 01, 10, 11    output at 10, 2 or 50 MHz;
//                        CNF: 00 push-pull, 01 open-drain,
//                             10 alternate push-pull, 11 alternate open-drain
//
// In input pull mode, the ODR bit selects pull-up (1) or pull-down (0).
// Outputs have no internal pull resistors.
//
// The setters change only the part of the field that they own, so they
// depend on the current mode. Call setMode() first, as configure() does.

namespace hw {

namespace {

constexpr uint32_t MODE_INPUT = 0b00;
constexpr uint32_t MODE_10MHZ = 0b01;
constexpr uint32_t MODE_2MHZ = 0b10;
constexpr uint32_t MODE_50MHZ = 0b11;

constexpr uint32_t CNF_INPUT_ANALOG = 0b00;
constexpr uint32_t CNF_INPUT_FLOATING = 0b01;
constexpr uint32_t CNF_INPUT_PULL = 0b10;
constexpr uint32_t CNF_OUTPUT_OPEN_DRAIN = 0b01;
constexpr uint32_t CNF_OUTPUT_ALTERNATE = 0b10;

struct PinConfig {
   uint32_t cnf;
   uint32_t mode;
};

volatile uint32_t &configRegister(GPIO_TypeDef *port, uint8_t pin) {
   return pin < 8 ? port->CRL : port->CRH;
}

uint32_t configShift(uint8_t pin) {
   return (pin & 0x07U) * 4U;
}

PinConfig readConfig(GPIO_TypeDef *port, uint8_t pin) {
   uint32_t field = (configRegister(port, pin) >> configShift(pin)) & 0xFU;
   return { field >> 2, field & 0x3U };
}

void writeConfig(GPIO_TypeDef *port, uint8_t pin, PinConfig config) {
   volatile uint32_t &reg = configRegister(port, pin);
   uint32_t shift = configShift(pin);
   uint32_t temp = reg;
   temp &= ~(0xFUL << shift);
   temp |= ((config.cnf << 2) | config.mode) << shift;
   reg = temp;
}

// The GpioPin speeds name the L0/G0 rates (~400 kHz, ~2, ~10, ~40 MHz).
// Map each one to the nearest F1 rate that is not slower.
uint32_t speedToMode(GpioPin::Speed speed) {
   switch(speed) {
   case GpioPin::Speed::Low:
   case GpioPin::Speed::Medium:
      return MODE_2MHZ;
   case GpioPin::Speed::High:
      return MODE_10MHZ;
   case GpioPin::Speed::VeryHigh:
      return MODE_50MHZ;
   }
   return MODE_2MHZ;
}

// Selects pull-up or pull-down through ODR. Only has an effect in input
// pull mode.
void setPullDirection(GPIO_TypeDef *port, uint8_t pin, GpioPin::Pull pull) {
   if(pull == GpioPin::Pull::Up) {
      port->BSRR = 1UL << pin;
   } else {
      port->BRR = 1UL << pin;
   }
}

} // namespace

void GpioPin::setMode(Mode mode) const {
   PinConfig current = readConfig(port, pin);
   bool isOutput = current.mode != MODE_INPUT;
   PinConfig next{};

   switch(mode) {
   case Mode::Input:
      // Keep an existing pull; otherwise start floating.
      next.mode = MODE_INPUT;
      next.cnf = (!isOutput && current.cnf == CNF_INPUT_PULL) ? CNF_INPUT_PULL : CNF_INPUT_FLOATING;
      break;
   case Mode::Analog:
      next = { CNF_INPUT_ANALOG, MODE_INPUT };
      break;
   case Mode::Output:
   case Mode::Alternate:
      // Keep the speed and the open-drain bit of an existing output.
      next.mode = isOutput ? current.mode : speedToMode(Speed::Low);
      next.cnf = isOutput ? (current.cnf & CNF_OUTPUT_OPEN_DRAIN) : 0;
      if(mode == Mode::Alternate) {
         next.cnf |= CNF_OUTPUT_ALTERNATE;
      }
      break;
   }
   writeConfig(port, pin, next);
}

void GpioPin::setOutputType(OutputType type) const {
   PinConfig config = readConfig(port, pin);
   if(config.mode == MODE_INPUT) {
      return; // Inputs have no output type.
   }
   config.cnf = (config.cnf & CNF_OUTPUT_ALTERNATE) | (type == OutputType::OpenDrain ? CNF_OUTPUT_OPEN_DRAIN : 0);
   writeConfig(port, pin, config);
}

void GpioPin::setSpeed(Speed speed) const {
   PinConfig config = readConfig(port, pin);
   if(config.mode == MODE_INPUT) {
      return; // Inputs have no speed.
   }
   config.mode = speedToMode(speed);
   writeConfig(port, pin, config);
}

void GpioPin::setPull(Pull pull) const {
   PinConfig config = readConfig(port, pin);
   bool isDigitalInput = config.mode == MODE_INPUT && config.cnf != CNF_INPUT_ANALOG;
   if(!isDigitalInput) {
      // F1 outputs and analog inputs have no pull resistors.
      assert(pull == Pull::None);
      return;
   }
   if(pull == Pull::None) {
      config.cnf = CNF_INPUT_FLOATING;
   } else {
      setPullDirection(port, pin, pull);
      config.cnf = CNF_INPUT_PULL;
   }
   writeConfig(port, pin, config);
}

void GpioPin::setAlternateFunction(AlternateFunction af) const {
   // F1 has no AF number per pin. The peripheral takes the pin when the
   // pin is an alternate-function output; remaps go through hw::Afio.
   // Alternate-function inputs (e.g. USART RX, SPI MISO) stay in an
   // input mode.
   (void)af;
   PinConfig config = readConfig(port, pin);
   if(config.mode == MODE_INPUT) {
      return;
   }
   config.cnf |= CNF_OUTPUT_ALTERNATE;
   writeConfig(port, pin, config);
}

void GpioPin::configure(Mode mode, OutputType type, Pull pull, Speed speed) const {
   // Compute the whole field and write it once, so the pin never passes
   // through an intermediate configuration.
   PinConfig config{};
   switch(mode) {
   case Mode::Analog:
      assert(pull == Pull::None);
      config = { CNF_INPUT_ANALOG, MODE_INPUT };
      break;
   case Mode::Input:
      if(pull == Pull::None) {
         config = { CNF_INPUT_FLOATING, MODE_INPUT };
      } else {
         // Set the pull direction before the pin becomes a pull input.
         setPullDirection(port, pin, pull);
         config = { CNF_INPUT_PULL, MODE_INPUT };
      }
      break;
   case Mode::Output:
   case Mode::Alternate:
      assert(pull == Pull::None);
      config.mode = speedToMode(speed);
      config.cnf = type == OutputType::OpenDrain ? CNF_OUTPUT_OPEN_DRAIN : 0;
      if(mode == Mode::Alternate) {
         config.cnf |= CNF_OUTPUT_ALTERNATE;
      }
      break;
   }
   writeConfig(port, pin, config);
}

} // namespace hw

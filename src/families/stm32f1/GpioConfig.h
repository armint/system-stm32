#pragma once

#include "peripherals/GpioPin.h"
#include <cmsis_device.h>
#include <cstdint>

// STM32F1 pin configuration encoding (RM0008 9.2.1, Table 20), used by
// GpioPin (families/stm32f1/GpioPin.cpp).
//
// Each pin has one 4-bit field in CRL (pins 0-7) or CRH (pins 8-15):
// CNF[1:0] in the upper two bits and MODE[1:0] in the lower two bits.
//
//   MODE = 00            input; CNF: 00 analog, 01 floating, 10 pull-up/down
//   MODE = 01, 10, 11    output at 10, 2 or 50 MHz;
//                        CNF: 00 push-pull, 01 open-drain,
//                             10 alternate push-pull, 11 alternate open-drain
//
// In input pull mode, the ODR bit selects pull-up (1) or pull-down (0).
// Outputs have no internal pull resistors.

namespace hw::gpio_f1 {

constexpr uint8_t MODE_INPUT = 0b00;
constexpr uint8_t MODE_10MHZ = 0b01;
constexpr uint8_t MODE_2MHZ = 0b10;
constexpr uint8_t MODE_50MHZ = 0b11;

constexpr uint8_t CNF_INPUT_ANALOG = 0b00;
constexpr uint8_t CNF_INPUT_FLOATING = 0b01;
constexpr uint8_t CNF_INPUT_PULL = 0b10;
constexpr uint8_t CNF_OUTPUT_OPEN_DRAIN = 0b01;
constexpr uint8_t CNF_OUTPUT_ALTERNATE = 0b10;

// The GpioPin speeds name the L0/G0 rates (~400 kHz, ~2, ~10, ~40 MHz).
// Map each one to the nearest F1 rate that is not slower.
constexpr uint8_t speedToMode(GpioPin::Speed speed) {
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

constexpr uint8_t field(uint8_t cnf, uint8_t mode) {
   return static_cast<uint8_t>((cnf << 2) | mode);
}

/**
 * @brief The 4-bit CNF/MODE field for a GpioPin configuration.
 *
 * Pull::Up and Pull::Down give the input pull mode; the ODR bit selects
 * the direction and is not part of the field. Outputs and analog inputs
 * ignore @p pull (F1 has no pull resistors there).
 */
constexpr uint8_t pinConfig(GpioPin::Mode mode,
                            GpioPin::OutputType type = GpioPin::OutputType::PushPull,
                            GpioPin::Pull pull = GpioPin::Pull::None,
                            GpioPin::Speed speed = GpioPin::Speed::Low) {
   switch(mode) {
   case GpioPin::Mode::Analog:
      return field(CNF_INPUT_ANALOG, MODE_INPUT);
   case GpioPin::Mode::Input:
      return field(pull == GpioPin::Pull::None ? CNF_INPUT_FLOATING : CNF_INPUT_PULL, MODE_INPUT);
   case GpioPin::Mode::Output:
   case GpioPin::Mode::Alternate: {
      uint8_t cnf = type == GpioPin::OutputType::OpenDrain ? CNF_OUTPUT_OPEN_DRAIN : 0;
      if(mode == GpioPin::Mode::Alternate) {
         cnf |= CNF_OUTPUT_ALTERNATE;
      }
      return field(cnf, speedToMode(speed));
   }
   }
   return field(CNF_INPUT_FLOATING, MODE_INPUT);
}

// Offset of CRL or CRH from the port base address.
constexpr uint32_t configRegisterOffset(uint8_t pin) {
   return pin < 8 ? 0x00U : 0x04U;
}

constexpr uint32_t configShift(uint8_t pin) {
   return (pin & 0x07U) * 4U;
}

} // namespace hw::gpio_f1

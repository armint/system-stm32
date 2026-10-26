
#include "GpioPin.h"

namespace hw {

void GpioPin::setMode(Mode mode) const {
   uint32_t pos = pin * 2;
   uint32_t mask = 3UL << pos;
   uint32_t val = static_cast<uint32_t>(mode) << pos;

   uint32_t temp = port->MODER;
   temp &= ~mask;
   temp |= val;
   port->MODER = temp;
}

void GpioPin::setOutputType(OutputType type) const {
   uint32_t pos = pin;
   uint32_t mask = 1UL << pos;
   uint32_t val = static_cast<uint32_t>(type) << pos;

   uint32_t temp = port->OTYPER;
   temp &= ~mask;
   temp |= val;
   port->OTYPER = temp;
}

void GpioPin::setSpeed(Speed speed) const {
   uint32_t pos = pin * 2;
   uint32_t mask = 3UL << pos;
   uint32_t val = static_cast<uint32_t>(speed) << pos;

   uint32_t temp = port->OSPEEDR;
   temp &= ~mask;
   temp |= val;
   port->OSPEEDR = temp;
}

void GpioPin::setPull(Pull pull) const {
   uint32_t pos = pin * 2;
   uint32_t mask = 3UL << pos;
   uint32_t val = static_cast<uint32_t>(pull) << pos;

   uint32_t temp = port->PUPDR;
   temp &= ~mask;
   temp |= val;
   port->PUPDR = temp;
}

void GpioPin::setAlternateFunction(AlternateFunction af) const {
   // AFR[0] is for pins 0-7, AFR[1] is for pins 8-15
   uint32_t regIndex = pin >> 3;
   // 4 bits per pin
   uint32_t pos = (pin & 0x07) * 4;

   uint32_t mask = 0xFUL << pos;
   uint32_t val = (static_cast<uint32_t>(af) & 0xFUL) << pos;

   uint32_t temp = port->AFR[regIndex];
   temp &= ~mask;
   temp |= val;
   port->AFR[regIndex] = temp;
}

void GpioPin::configure(Mode mode, OutputType type, Pull pull, Speed speed) const {
   setMode(mode);
   setOutputType(type);
   setPull(pull);
   setSpeed(speed);
}

} // namespace hw
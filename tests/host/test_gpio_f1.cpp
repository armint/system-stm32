// Host tests for the STM32F1 GPIO code: GpioPin on CRL/CRH, the CNF/MODE
// encoding, and the bit-band write sequence of the FastPin direction switch.

#include "GpioConfig.h"
#include "peripherals/FastPin.h"
#include "peripherals/GpioPin.h"
#include "utils/Interrupt.h"

#include <cstdio>
#include <vector>

namespace {

int failures = 0;

#define CHECK(cond)                                                  \
   do {                                                              \
      if(!(cond)) {                                                  \
         std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
         ++failures;                                                 \
      }                                                              \
   } while(0)

int criticalSections = 0;
int criticalDepth = 0;

using hw::GpioPin;
using M = GpioPin::Mode;
using T = GpioPin::OutputType;
using P = GpioPin::Pull;
using S = GpioPin::Speed;
namespace f1 = hw::gpio_f1;

constexpr uint32_t ResetConfig = 0x44444444; // all pins floating input

uint32_t fieldOf(const GPIO_TypeDef &gpio, uint8_t pin) {
   uint32_t reg = pin < 8 ? gpio.CRL : gpio.CRH;
   return (reg >> ((pin & 7U) * 4U)) & 0xFU;
}

// Every pin except `pin` still has its reset field.
bool othersUnchanged(const GPIO_TypeDef &gpio, uint8_t pin) {
   for(uint8_t other = 0; other < 16; ++other) {
      if(other != pin && fieldOf(gpio, other) != 0x4U) {
         return false;
      }
   }
   return true;
}

void testPinConfig() {
   static_assert(f1::pinConfig(M::Analog) == 0b0000);
   static_assert(f1::pinConfig(M::Input) == 0b0100);
   static_assert(f1::pinConfig(M::Input, T::PushPull, P::Up) == 0b1000);
   static_assert(f1::pinConfig(M::Input, T::PushPull, P::Down) == 0b1000);
   static_assert(f1::pinConfig(M::Output) == 0b0010);
   static_assert(f1::pinConfig(M::Output, T::OpenDrain, P::None, S::VeryHigh) == 0b0111);
   static_assert(f1::pinConfig(M::Alternate, T::PushPull, P::None, S::High) == 0b1001);
   static_assert(f1::pinConfig(M::Alternate, T::OpenDrain, P::None, S::Medium) == 0b1110);
}

void testGpioPinConfigure() {
   GPIO_TypeDef gpio{ ResetConfig, ResetConfig, 0, 0, 0, 0, 0 };
   GpioPin low(&gpio, 3);
   GpioPin high(&gpio, 13);

   low.configure(M::Output);
   CHECK(fieldOf(gpio, 3) == 0b0010);
   CHECK(othersUnchanged(gpio, 3));
   low.configure(M::Output, T::OpenDrain, P::None, S::VeryHigh);
   CHECK(fieldOf(gpio, 3) == 0b0111);
   low.configure(M::Alternate, T::PushPull, P::None, S::High);
   CHECK(fieldOf(gpio, 3) == 0b1001);
   low.configure(M::Analog);
   CHECK(fieldOf(gpio, 3) == 0b0000);
   low.configure(M::Input);
   CHECK(fieldOf(gpio, 3) == 0b0100);

   high.configure(M::Input, T::PushPull, P::Up);
   CHECK(fieldOf(gpio, 13) == 0b1000);
   CHECK(gpio.BSRR == (1U << 13));
   high.configure(M::Input, T::PushPull, P::Down);
   CHECK(gpio.BRR == (1U << 13));
   high.configure(M::Input);
   CHECK(othersUnchanged(gpio, 99));
}

void testGpioPinSetters() {
   GPIO_TypeDef gpio{ ResetConfig, ResetConfig, 0, 0, 0, 0, 0 };
   GpioPin pin(&gpio, 13);

   pin.setMode(M::Output);
   CHECK(fieldOf(gpio, 13) == 0b0010);
   pin.setOutputType(T::OpenDrain);
   CHECK(fieldOf(gpio, 13) == 0b0110);
   pin.setSpeed(S::VeryHigh);
   CHECK(fieldOf(gpio, 13) == 0b0111);
   pin.setMode(M::Alternate);
   CHECK(fieldOf(gpio, 13) == 0b1111);
   pin.setMode(M::Output);
   CHECK(fieldOf(gpio, 13) == 0b0111);
   pin.setAlternateFunction(GpioPin::AlternateFunction::AF7);
   CHECK(fieldOf(gpio, 13) == 0b1111);
   pin.setOutputType(T::PushPull);
   CHECK(fieldOf(gpio, 13) == 0b1011);

   // Output-only settings have no effect on an input.
   pin.setMode(M::Input);
   CHECK(fieldOf(gpio, 13) == 0b0100);
   pin.setSpeed(S::High);
   pin.setOutputType(T::OpenDrain);
   pin.setAlternateFunction(GpioPin::AlternateFunction::AF1);
   CHECK(fieldOf(gpio, 13) == 0b0100);

   pin.setPull(P::Up);
   CHECK(fieldOf(gpio, 13) == 0b1000);
   pin.setMode(M::Input); // keeps the pull
   CHECK(fieldOf(gpio, 13) == 0b1000);
   pin.setPull(P::None);
   CHECK(fieldOf(gpio, 13) == 0b0100);
   pin.setMode(M::Analog);
   CHECK(fieldOf(gpio, 13) == 0b0000);
   pin.setMode(M::Input);
   CHECK(othersUnchanged(gpio, 99));
}

void testGpioPinIsAtomic() {
   GPIO_TypeDef gpio{ ResetConfig, ResetConfig, 0, 0, 0, 0, 0 };
   GpioPin pin(&gpio, 5);
   int before = criticalSections;
   pin.configure(M::Output);
   pin.setMode(M::Input);
   pin.setPull(P::Up);
   pin.setOutputType(T::OpenDrain); // no change: still one critical section
   CHECK(criticalSections - before == 4);
   CHECK(criticalDepth == 0);
}

void testToggle() {
   GPIO_TypeDef gpio{ ResetConfig, ResetConfig, 0, 0, 0, 0, 0 };
   GpioPin pin(&gpio, 9);
   gpio.ODR = 0;
   pin.toggle();
   CHECK(gpio.BSRR == (1U << 9)); // set, no reset bits
   gpio.ODR = (1U << 9) | (1U << 2);
   pin.toggle();
   CHECK(gpio.BSRR == (1U << (9 + 16)));       // reset only pin 9
   CHECK(gpio.ODR == ((1U << 9) | (1U << 2))); // ODR itself is not written
}

void testBitBandAlias() {
   // GPIOA_CRL bit 0, and GPIOB_CRL bit 16 (pin 4 MODE0).
   static_assert(f1::bitBandAlias(GPIOA_BASE, 0) == 0x42210000UL);
   static_assert(f1::bitBandAlias(GPIOB_BASE, 16) == 0x42218040UL);
   // GPIOB_CRH (pin 12) bit 16.
   static_assert(f1::bitBandAlias(GPIOB_BASE + f1::configRegisterOffset(12), f1::configShift(12)) == 0x422180C0UL);
}

// Applies the FastPin direction switch to a fake port: the same address
// calculation and write order as FastPin::setConfig<>(), with each alias
// address decoded back to its register and bit.
template <typename Visit>
void applySwitch(GPIO_TypeDef &gpio, uintptr_t portBase, uint8_t pin, uint8_t config, Visit visit) {
   auto order = f1::writeOrder(config);
   uintptr_t reg = portBase + f1::configRegisterOffset(pin);
   for(const auto &write : order) {
      uintptr_t alias = f1::bitBandAlias(reg, f1::configShift(pin) + write.bit);
      uintptr_t byteOffset = (alias - PERIPH_BB_BASE) / 32U;
      uint32_t bit = static_cast<uint32_t>((alias - PERIPH_BB_BASE) % 32U) / 4U;
      uintptr_t registerAddress = PERIPH_BASE + (byteOffset & ~uintptr_t{ 3 });
      bit += static_cast<uint32_t>(byteOffset & 3U) * 8U;
      volatile uint32_t &target = registerAddress == portBase ? gpio.CRL : gpio.CRH;
      CHECK(registerAddress == portBase || registerAddress == portBase + 4U);
      if(write.value) {
         target = target | (1U << bit);
      } else {
         target = target & ~(1U << bit);
      }
      visit(fieldOf(gpio, pin));
   }
}

void testDirectionSwitchTouchesOnlyItsPin() {
   using Pin = hw::FastPin<GPIOB_BASE, 4>;
   for(uint8_t pin = 0; pin < 16; ++pin) {
      for(uint8_t config : { Pin::OutputConfig, Pin::InputConfig, uint8_t{ 0b1011 }, uint8_t{ 0b0000 } }) {
         GPIO_TypeDef gpio{ ResetConfig, ResetConfig, 0, 0, 0, 0, 0 };
         applySwitch(gpio, GPIOB_BASE, pin, config, [](uint32_t) {});
         CHECK(fieldOf(gpio, pin) == config);
         CHECK(othersUnchanged(gpio, pin));
      }
   }
}

void testDirectionSwitchIntermediateStates() {
   using Pin = hw::FastPin<GPIOB_BASE, 4>;
   static_assert(Pin::OutputConfig == 0b0011);
   static_assert(Pin::InputConfig == 0b0100);

   // All inputs (CNF 00, 01, 10) and all outputs without CNF 11 (MODE 01,
   // 10, 11). CNF 11 is the reserved input state, or the alternate
   // open-drain output (see gpio_f1::writeOrder()).
   std::vector<uint8_t> inputs{ 0b0000, 0b0100, 0b1000 };
   std::vector<uint8_t> outputs;
   for(uint8_t cnf = 0; cnf < 3; ++cnf) {
      for(uint8_t mode = 1; mode < 4; ++mode) {
         outputs.push_back(f1::field(cnf, mode));
      }
   }

   auto run = [](uint8_t from, uint8_t to, uint8_t drivingCnf) {
      GPIO_TypeDef gpio{ ResetConfig, ResetConfig, 0, 0, 0, 0, 0 };
      gpio.CRL = (gpio.CRL & ~(0xFU << 16)) | (uint32_t{ from } << 16);
      applySwitch(gpio, GPIOB_BASE, 4, to, [drivingCnf](uint32_t state) {
         if((state & 0x3U) != 0) {
            // While the pin drives, it drives with one known CNF.
            CHECK((state >> 2) == drivingCnf);
         } else {
            CHECK((state >> 2) != 0b11U); // never the reserved input
         }
      });
      CHECK(fieldOf(gpio, 4) == to);
   };

   // Input to output: while the pin drives, CNF is already the target CNF.
   for(uint8_t from : inputs) {
      for(uint8_t to : outputs) {
         run(from, to, static_cast<uint8_t>(to >> 2));
      }
   }
   // Output to input: while the pin drives, CNF is still the old CNF.
   for(uint8_t from : outputs) {
      for(uint8_t to : inputs) {
         run(from, to, static_cast<uint8_t>(from >> 2));
      }
   }
}

} // namespace

// Host stand-in for utils/Interrupt.cpp: counts the critical sections.
namespace Interrupt {
CriticalSection::CriticalSection()
   : _state(0) {
   ++criticalSections;
   ++criticalDepth;
}

CriticalSection::~CriticalSection() {
   --criticalDepth;
}
} // namespace Interrupt

int main() {
   testPinConfig();
   testGpioPinConfigure();
   testGpioPinSetters();
   testGpioPinIsAtomic();
   testToggle();
   testBitBandAlias();
   testDirectionSwitchTouchesOnlyItsPin();
   testDirectionSwitchIntermediateStates();
   if(failures != 0) {
      std::printf("%d check(s) failed\n", failures);
      return 1;
   }
   std::printf("all GPIO F1 host tests passed\n");
   return 0;
}

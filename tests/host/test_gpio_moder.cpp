// Host tests for the GPIO code of the MODER families (L0, G0, WL, H5): the
// FastPin direction switch and the AFR mapping of GpioPin.

#include "peripherals/FastPin.h"
#include "peripherals/GpioPin.h"

#include <sys/mman.h>

#include <cstdio>

namespace {

int failures = 0;

#define CHECK(cond)                                                  \
   do {                                                              \
      if(!(cond)) {                                                  \
         std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
         ++failures;                                                 \
      }                                                              \
   } while(0)

using hw::GpioPin;
using M = GpioPin::Mode;

uint32_t primask = 0;
// MODER when interrupts go off and when PRIMASK is restored: the write to
// MODER must fall between the two.
int disableCalls = 0;
uint32_t moderAtDisable = 0;
uint32_t moderAtRestore = 0;

GPIO_TypeDef *gpio = nullptr;

// FastPin needs the port at a constant address: map one page there.
bool mapFakePort() {
   void *page = mmap(reinterpret_cast<void *>(GPIOA_BASE), 4096, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
   if(page != reinterpret_cast<void *>(GPIOA_BASE)) {
      std::printf("cannot map the fake port at 0x%lx\n", GPIOA_BASE);
      return false;
   }
   gpio = static_cast<GPIO_TypeDef *>(page);
   return true;
}

uint32_t fieldOf(uint8_t pin) {
   return (gpio->MODER >> (pin * 2U)) & 3U;
}

template <uint8_t Pin>
void checkDirectionSwitch() {
   using P = hw::FastPin<GPIOA_BASE, Pin>;
   constexpr uint32_t others = ~(3U << (Pin * 2U));
   for(uint32_t reset : { 0x00000000U, 0xFFFFFFFFU, 0xABFFFFFFU }) {
      gpio->MODER = reset;
      gpio->ODR = 0x1234;
      gpio->PUPDR = 0x5555;
      P::setOutput();
      CHECK(fieldOf(Pin) == 1U);
      CHECK((gpio->MODER & others) == (reset & others));
      P::setInput();
      CHECK(fieldOf(Pin) == 0U);
      CHECK((gpio->MODER & others) == (reset & others));
      P::template setMode<M::Alternate>();
      CHECK(fieldOf(Pin) == 2U);
      P::template setMode<M::Analog>();
      CHECK(fieldOf(Pin) == 3U);
      CHECK((gpio->MODER & others) == (reset & others));
      CHECK(gpio->ODR == 0x1234);
      CHECK(gpio->PUPDR == 0x5555);
   }
}

void testDirectionSwitch() {
   checkDirectionSwitch<0>();
   checkDirectionSwitch<7>();
   checkDirectionSwitch<8>();
   checkDirectionSwitch<15>();
}

// The switch writes MODER with interrupts off, and then restores PRIMASK:
// it does not enable interrupts that the caller has disabled.
void testDirectionSwitchIsCritical() {
   using P = hw::FastPin<GPIOA_BASE, 3>;
   for(uint32_t callerPrimask : { 0U, 1U }) {
      gpio->MODER = 0;
      primask = callerPrimask;
      disableCalls = 0;
      P::setOutput();
      CHECK(disableCalls == 1);
      CHECK(moderAtDisable == 0);
      CHECK(moderAtRestore == (1U << 6));
      CHECK(primask == callerPrimask);
      P::setInput();
      CHECK(disableCalls == 2);
      CHECK(moderAtDisable == (1U << 6));
      CHECK(moderAtRestore == 0);
      CHECK(primask == callerPrimask);
   }
   primask = 0;
}

void testAlternateFunction() {
   GPIO_TypeDef port{};
   port.AFR[0] = 0xFFFFFFFF;
   port.AFR[1] = 0xFFFFFFFF;
   GpioPin(&port, 2).setAlternateFunction(GpioPin::AlternateFunction::AF5);
   CHECK(port.AFR[0] == 0xFFFFF5FFU);
   GpioPin(&port, 13).setAlternateFunction(GpioPin::AlternateFunction::AF0);
   CHECK(port.AFR[1] == 0xFF0FFFFFU);
   GpioPin(&port, 7).setAlternateFunction(GpioPin::AlternateFunction::AF14);
   CHECK(port.AFR[0] == 0xEFFFF5FFU);
}

void testWriteUsesBsrr() {
   using P = hw::FastPin<GPIOA_BASE, 9>;
   P::write(true);
   CHECK(gpio->BSRR == ((1U << 25) | (1U << 9)));
   P::write(false);
   CHECK(gpio->BSRR == (1U << 25));
   gpio->IDR = 1U << 9;
   CHECK(P::read());
   gpio->IDR = ~(1U << 9);
   CHECK(!P::read());
}

} // namespace

uint32_t __get_PRIMASK() {
   return primask;
}

void __set_PRIMASK(uint32_t value) {
   moderAtRestore = gpio->MODER;
   primask = value;
}

void __disable_irq() {
   ++disableCalls;
   moderAtDisable = gpio->MODER;
   primask = 1;
}

int main() {
   if(!mapFakePort()) {
      return 1;
   }
   testDirectionSwitch();
   testDirectionSwitchIsCritical();
   testAlternateFunction();
   testWriteUsesBsrr();
   if(failures != 0) {
      std::printf("%d check(s) failed\n", failures);
      return 1;
   }
   std::printf("all GPIO MODER host tests passed\n");
   return 0;
}

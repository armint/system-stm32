// Minimal STM32H5 test image. CI links it to prove that the H5 part of the
// library builds, links and fits in memory: start-up, vector table, fault
// handlers, the interrupt table copy, the DWT delay, the RCC with the 250 MHz
// recipes, the ICACHE, HSI48 and the CRS.
//
// On a NUCLEO-H533RE it sets 250 MHz from the 24 MHz HSE crystal. Hold B1
// (PC13) during reset to use HSI instead. LD2 (PA5) then toggles every
// 500 SysTick interrupts of 1 ms: it blinks at 1 Hz only if SystemCoreClock
// is the real core clock. If the clock recipe fails, LD2 blinks fast (5 Hz)
// on the reset clock (HSI, 32 MHz).
#include <cmsis_device.h>
#include <delaybusywait.h>
#include <peripherals/Crs.h>
#include <peripherals/GpioPin.h>
#include <peripherals/Rcc.h>
#include <peripherals/Timer.h>
#include <utils/Interrupt.h>
#include <utils/Timekeeping.h>

namespace {
volatile uint32_t ticks;
volatile uint32_t sink;

void onTim2() {
   TIM2->SR = 0;
   ticks = ticks + 1;
}

// HSI48 trimmed by the CRS from USB SOF: the USB clock of the H5.
void exerciseUsbClock() {
   if(!Rcc::enableHSI48()) {
      return;
   }
   Rcc::setUSBClockSource(Rcc::USBClockSource::HSI48);
   Rcc::enableCRSClock();
   if(hw::Crs::configure({})) {
      hw::Crs::enable();
   }
}
} // namespace

int main() {
   Rcc::enableGPIOAClock();
   Rcc::enableGPIOCClock();

   using Pin = hw::GpioPin;
   Pin led(GPIOA, 5);
   led.configure(Pin::Mode::Output, Pin::OutputType::PushPull);
   Pin key(GPIOC, 13);
   key.configure(Pin::Mode::Input);

   const bool clockOk = key.read() ? Rcc::setClock250MhzHSI() : Rcc::setClock250MhzHSE();
   Rcc::enableICache();
   exerciseUsbClock();
   sink = Rcc::getAPBClock() + Rcc::getAPB2Clock() + Rcc::getAPB3Clock() + Rcc::getPLL1QClock();

   // Links Interrupt::connectHandler(): it copies the 149-entry vector table
   // to SRAM_BASE and moves VTOR there.
   Rcc::enableTIM2Clock();
   Interrupt::connectHandler(TIM2_IRQn, &onTim2);
   hw::Timer timer(TIM2);
   timer.enableCounter();

   delayCycles(100);
   delayuS(10);
   if(!clockOk) {
      for(;;) {
         led.write(!led.read());
         delaymS(100);
      }
   }
   SysTick_Config(SystemCoreClock / 1000);
   timestamp_t next = 500;
   for(;;) {
      if(getUptimeTicks() >= next) {
         next += 500;
         led.write(!led.read());
      }
   }
}

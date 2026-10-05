// Minimal STM32F1 test image. CI links it to prove that the F1 part of the
// library builds, links and fits in memory. It never runs on hardware.
// Extend it when more F1 drivers land (GPIO, USART, ADC, USB).
#include <cmsis_device.h>
#include <delaybusywait.h>
#include <peripherals/Timer.h>
#include <utils/Interrupt.h>

namespace {
volatile uint32_t ticks;

void onTim2() {
   TIM2->SR = 0;
   ticks = ticks + 1;
}
} // namespace

int main() {
   Interrupt::connectHandler(TIM2_IRQn, &onTim2);
   hw::Timer timer(TIM2);
   timer.enableCounter();
   for(;;) {
      __WFI();
   }
}

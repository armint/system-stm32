// Minimal STM32H5 test image. CI links it to prove that the H5 part of the
// library builds, links and fits in memory: start-up, vector table, fault
// handlers, the interrupt table copy and the DWT delay.
//
// On a NUCLEO-H533RE it blinks LD2 (PA5) at 1 Hz on the reset clock (HSI,
// 32 MHz). The RCC driver for the H5 comes with system-stm32#32, so the
// clock enable below writes the register directly.
#include <cmsis_device.h>
#include <delaybusywait.h>
#include <peripherals/GpioPin.h>
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
   RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
   (void)RCC->AHB2ENR; // Wait for the clock before the first GPIO access.

   using Pin = hw::GpioPin;
   Pin led(GPIOA, 5);
   led.configure(Pin::Mode::Output, Pin::OutputType::PushPull);

   // Links Interrupt::connectHandler(): it copies the 149-entry vector table
   // to SRAM_BASE and moves VTOR there.
   RCC->APB1LENR |= RCC_APB1LENR_TIM2EN;
   Interrupt::connectHandler(TIM2_IRQn, &onTim2);
   hw::Timer timer(TIM2);
   timer.enableCounter();

   delayCycles(100);
   delayuS(10);
   for(;;) {
      led.write(!led.read());
      delaymS(500);
   }
}

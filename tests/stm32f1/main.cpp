// Minimal STM32F1 test image. CI links it to prove that the F1 part of the
// library builds, links and fits in memory. It never runs on hardware.
// Extend it when more F1 drivers land (USART, ADC, USB).
#include <Afio.h>
#include <cmsis_device.h>
#include <delaybusywait.h>
#include <peripherals/FastPin.h>
#include <peripherals/GpioPin.h>
#include <peripherals/Timer.h>
#include <utils/Interrupt.h>

namespace {
volatile uint32_t ticks;
volatile bool sink;

using Tck = hw::FastPin<GPIOA_BASE, 5>;
using Tdo = hw::FastPin<GPIOA_BASE, 6>;

// Links the F1 GpioPin (CRL/CRH), Afio and FastPin code.
void exerciseGpio() {
   RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN;
   hw::Afio::setSwjConfig(hw::Afio::SwjConfig::SwdOnly);
   hw::Afio::remapSpi1(true);

   using Pin = hw::GpioPin;
   Pin led(GPIOC, 13);
   led.configure(Pin::Mode::Output, Pin::OutputType::OpenDrain);
   Pin key(GPIOB, 12);
   key.configure(Pin::Mode::Input, Pin::OutputType::PushPull, Pin::Pull::Up);
   Tck::pin().configure(Pin::Mode::Alternate, Pin::OutputType::PushPull, Pin::Pull::None, Pin::Speed::VeryHigh);
   Tdo::pin().configure(Pin::Mode::Input);

   Pin pin(GPIOB, 9);
   pin.setMode(Pin::Mode::Output);
   pin.setOutputType(Pin::OutputType::PushPull);
   pin.setSpeed(Pin::Speed::High);
   pin.setAlternateFunction(Pin::AlternateFunction::AF0);
   pin.setMode(Pin::Mode::Input);
   pin.setPull(Pin::Pull::Down);

   led.write(key.read());
   Tck::write(true);
   Tck::clear();
   Tck::set();
   sink = Tdo::read();
}

void onTim2() {
   TIM2->SR = 0;
   ticks = ticks + 1;
}
} // namespace

int main() {
   exerciseGpio();
   Interrupt::connectHandler(TIM2_IRQn, &onTim2);
   hw::Timer timer(TIM2);
   timer.enableCounter();
   // Link the DWT delay code too.
   delayuS(10);
   delaymS(1);
   delayCycles(100);
   for(;;) {
      __WFI();
   }
}

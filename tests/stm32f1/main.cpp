// Minimal STM32F1 test image. CI links it to prove that the F1 part of the
// library builds, links and fits in memory. It never runs on hardware.
// Extend it when more F1 drivers land (USART).
#include <Adc.h>
#include <Afio.h>
#include <UsbDevice.h>
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

// Links the F1 Adc code: clock setup, calibration, single conversion and the
// VDDA measurement from Vrefint.
void exerciseAdc() {
   Adc::enableClock();
   Adc::configureClock();
   Adc adc(ADC1);
   adc.enable();
   if(!adc.calibrate())
      return;
   adc.setSamplingTime(Adc::PB0, Adc::SamplingCycles::Cycles55_5);
   uint16_t raw = 0;
   uint32_t vdda = 0;
   if(adc.convert(Adc::PB0, raw) && adc.readSupplyMillivolts(vdda))
      sink = Adc::toMillivolts(raw, vdda) > 0;
   adc.disable();
}

// Links the F1 USB driver: a packet-level echo on bulk EP1, without
// enumeration (no descriptors, EP0 SETUP is stalled). The CDC echo with
// the libArT USB stack is in the oven controller (extra/boardtest).
hw::UsbDevice usb;

void onUsbEvent(void *, const hw::usb::Event &event) {
   using hw::usb::EventType;
   switch(event.type) {
   case EventType::Reset:
      (void)usb.openEndpoint(0x01, hw::usb::EndpointType::Bulk, 64);
      (void)usb.openEndpoint(0x81, hw::usb::EndpointType::Bulk, 64);
      usb.armOut(0x01);
      break;
   case EventType::Setup:
      usb.setStall(0x80, true);
      usb.setStall(0x00, true);
      break;
   case EventType::Out:
      if(event.endpoint == 0x01) {
         uint8_t packet[64];
         uint16_t size = usb.readOut(0x01, packet);
         (void)usb.startIn(0x81, std::span<const uint8_t>(packet, size));
      }
      break;
   case EventType::InComplete:
      if(event.endpoint == 0x81) {
         usb.armOut(0x01);
      }
      break;
   case EventType::Suspend:
   case EventType::Resume:
      break;
   }
}

void usbPullup(void *, bool enable) {
   // Versaloon Mini: PC13 switches the D+ pull-up.
   GPIOC->BSRR = enable ? (1UL << 13) : (1UL << (13 + 16));
}

void exerciseUsb() {
   RCC->APB1ENR |= RCC_APB1ENR_USBEN;
   usb.setPullupCallback(&usbPullup, nullptr);
   usb.begin(&onUsbEvent, nullptr);
   usb.connect();
   sink = usb.isConnected() && !usb.isStalled(0x81);
}

void onTim2() {
   TIM2->SR = 0;
   ticks = ticks + 1;
}
} // namespace

int main() {
   exerciseGpio();
   exerciseAdc();
   exerciseUsb();
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

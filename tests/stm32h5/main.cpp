// Minimal STM32H5 test image. CI links it to prove that the H5 part of the
// library builds, links and fits in memory: start-up, vector table, fault
// handlers, the interrupt table copy, the DWT delay, the RCC with the 250 MHz
// recipes, the ICACHE, HSI48, the CRS, GpioPin, FastPin, UID, DBG, IWDG, the
// UCPD dead-battery disable, the USART, the SPI v2, the ADC, the GPDMA and
// the USB device driver.
//
// After the clock setup it runs self-tests and prints the results on USART2
// (PA2/PA3, the ST-LINK virtual COM port, 115200 8N1):
// - USART1 loopback: a wire from PA9 (TX) to PA10 (RX).
// - SPI2 loopback, master, 8 bits: a wire from PB15 (MOSI) to PB14 (MISO).
//   SCK is on PB13.
// - ADC1: VDDA from Vrefint and the factory VREFINT_CAL, and PB0 (channel
//   9) in mV. Then PB0 again at each LD2 toggle.
// - GPDMA1 memory-to-memory: one transfer and a two-item linked list.
// A missing wire gives "FAIL" for that test only.
//
// USB (CN13, USB Type-C): a packet-level echo without enumeration. The host
// sees the device attach, but EP0 stalls every SETUP. Bulk OUT 0x02
// (double-buffered) echoes on bulk IN 0x81.
//
// On a NUCLEO-H533RE it sets 250 MHz from the 24 MHz HSE crystal. Hold B1
// (PC13) during reset to use HSI instead. LD2 (PA5) then toggles every
// 500 SysTick interrupts of 1 ms: it blinks at 1 Hz only if SystemCoreClock
// is the real core clock. If the clock recipe fails, LD2 blinks fast (5 Hz)
// on the reset clock (HSI, 32 MHz). The IWDG (about 2 s) resets the board if
// the main loop stops; it stops while a debugger halts the core.
#include <Adc.h>
#include <UsbDevice.h>
#include <cmsis_device.h>
#include <delaybusywait.h>
#include <peripherals/Crs.h>
#include <peripherals/Dma.h>
#include <peripherals/Dbg.h>
#include <peripherals/FastPin.h>
#include <peripherals/GpioPin.h>
#include <peripherals/Iwdg.h>
#include <peripherals/Pwr.h>
#include <peripherals/Rcc.h>
#include <peripherals/Spi.h>
#include <peripherals/Timer.h>
#include <peripherals/Uid.h>
#include <peripherals/Usart.h>
#include <utils/Interrupt.h>
#include <utils/Timekeeping.h>

namespace {
volatile uint32_t ticks;
volatile uint32_t sink;

using Led = hw::FastPin<GPIOA_BASE, 5>;

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

// Links the H5 USB driver: a packet-level echo, without enumeration (no
// descriptors, EP0 SETUP is stalled).
hw::UsbDevice usb;

void onUsbEvent(void *, const hw::usb::Event &event) {
   using hw::usb::EventType;
   switch(event.type) {
   case EventType::Reset:
      (void)usb.openEndpoint(0x02, hw::usb::EndpointType::Bulk, 64, true);
      (void)usb.openEndpoint(0x81, hw::usb::EndpointType::Bulk, 64);
      usb.armOut(0x02);
      break;
   case EventType::Setup:
      usb.setStall(0x80, true);
      usb.setStall(0x00, true);
      break;
   case EventType::Out:
      if(event.endpoint == 0x02) {
         uint8_t packet[64];
         uint16_t size = usb.readOut(0x02, packet);
         (void)usb.startIn(0x81, hw::span<const uint8_t>(packet, size));
      }
      break;
   case EventType::InComplete:
      if(event.endpoint == 0x81) {
         usb.armOut(0x02);
      }
      break;
   case EventType::Suspend:
   case EventType::Resume:
      break;
   }
}

// After exerciseUsbClock(): the USB kernel clock is HSI48. GPIOA clock on.
void exerciseUsb() {
   // PA11/PA12: USB_DM/USB_DP are alternate function AF10 on the H533.
   for(uint8_t pin : { 11, 12 }) {
      const hw::GpioPin p(GPIOA, pin);
      p.setAlternateFunction(hw::GpioPin::AlternateFunction::AF10);
      p.configure(hw::GpioPin::Mode::Alternate, hw::GpioPin::OutputType::PushPull, hw::GpioPin::Pull::None, hw::GpioPin::Speed::VeryHigh);
   }
   Rcc::enableUSBClock();
   usb.begin(&onUsbEvent, nullptr);
   usb.connect();
   sink = usb.isConnected() && !usb.isStalled(0x81);
}

using Pin = hw::GpioPin;

// RX pins get a pull-up: without the loopback wire they then idle high.
void alternate(GPIO_TypeDef *port, uint8_t pin, Pin::AlternateFunction af, Pin::Pull pull = Pin::Pull::None) {
   const Pin p(port, pin);
   p.setAlternateFunction(af);
   p.configure(Pin::Mode::Alternate, Pin::OutputType::PushPull, pull, Pin::Speed::High);
}

void startUsart(const hw::Usart &usart) {
   usart.configure(115200);
   usart.enableFifo();
   usart.enableTx();
   usart.enableRx();
   usart.enable();
}

// USART2: the ST-LINK virtual COM port of the NUCLEO-H533RE.
const hw::Usart vcp(USART2);

void print(const char *text) {
   for(; *text != '\0'; ++text) {
      while(!vcp.isTxRegisterEmpty()) {
      }
      vcp.setData(static_cast<uint8_t>(*text));
   }
}

void print(uint32_t value) {
   char digits[11];
   int n = 0;
   do {
      digits[n++] = static_cast<char>('0' + value % 10);
      value /= 10;
   } while(value != 0);
   char text[12];
   for(int i = 0; i < n; ++i) {
      text[i] = digits[n - 1 - i];
   }
   text[n] = '\0';
   print(text);
}

void printResult(const char *name, bool ok) {
   print(name);
   print(ok ? ": ok\r\n" : ": FAIL\r\n");
}

// USART1 with the FIFO on: PA9 (TX) to PA10 (RX), AF7.
bool usartLoopback() {
   Rcc::enableUSART1Clock();
   alternate(GPIOA, 9, Pin::AlternateFunction::AF7);
   alternate(GPIOA, 10, Pin::AlternateFunction::AF7, Pin::Pull::Up);
   const hw::Usart usart(USART1);
   startUsart(usart);
   while(usart.isRxRegisterNotEmpty()) {
      (void)usart.getData();
   }
   for(const uint8_t byte : { 0x55U, 0xA5U, 0x00U, 0xFFU }) {
      usart.setData(byte);
      uint32_t loops = 1000000;
      while(!usart.isRxRegisterNotEmpty()) {
         if(--loops == 0) {
            return false;
         }
      }
      if(usart.getData() != byte) {
         return false;
      }
   }
   return true;
}

// SPI2 master, 8 bits, 125 MHz / 64: PB13 SCK, PB14 MISO, PB15 MOSI, AF5.
bool spiLoopback() {
   Rcc::enableSPI2Clock();
   for(const uint8_t pin : { 13, 14, 15 }) {
      alternate(GPIOB, pin, Pin::AlternateFunction::AF5);
   }
   const hw::Spi spi(SPI2);
   spi.configure(hw::Spi::BaudRatePrescaler::Div64);
   spi.enable();
   bool ok = true;
   for(const uint8_t byte : { 0x5AU, 0x81U, 0x00U, 0xFFU }) {
      const auto rx = spi.transfer(byte);
      ok = ok && rx && *rx == byte;
   }
   ok = spi.waitUntilIdle() && ok;
   spi.disable();
   return ok;
}

const Adc adc(ADC1);

bool startAdc() {
   Pin(GPIOB, 0).configure(Pin::Mode::Analog);
   Adc::enableClock();
   if(Adc::configureClock() == 0 || !adc.powerUp() || !adc.calibrate() || !adc.enable()) {
      return false;
   }
   adc.setSamplingTime(Adc::PB0, Adc::SamplingCycles::Cycles247_5);
   return true;
}

uint32_t vddaMv = 3300;

void printPb0() {
   uint16_t raw = 0;
   if(!adc.convert(Adc::PB0, raw)) {
      printResult("pb0", false);
      return;
   }
   print("pb0: ");
   print(raw);
   print(" raw, ");
   print(Adc::toMillivolts(raw, vddaMv));
   print(" mV\r\n");
}

alignas(4) uint8_t dmaSource[64];
alignas(4) uint8_t dmaTarget[64];
// The items must share one 64 KiB region: align the array to its size.
alignas(64) Dma::LinkedListItem dmaList[2];

bool waitDone(const Dma::Channel &channel) {
   for(uint32_t i = 0; i < 100000; ++i) {
      if(channel.isErrorFlagSet()) {
         return false;
      }
      if(channel.isTransferCompleteFlagSet() && channel.isIdle()) {
         return true;
      }
   }
   return false;
}

bool targetMatches(size_t offset, size_t count) {
   for(size_t i = 0; i < count; ++i) {
      if(dmaTarget[offset + i] != dmaSource[offset + i]) {
         return false;
      }
   }
   return true;
}

// Memory-to-memory with one transfer, then with a two-item list. A transfer
// to a peripheral only changes Config::direction and Config::request.
bool exerciseDma() {
   Rcc::enableGPDMA1Clock();
   for(size_t i = 0; i < sizeof(dmaSource); ++i) {
      dmaSource[i] = static_cast<uint8_t>(i * 7 + 1);
   }
   const Dma::Channel channel(GPDMA1, 0);
   Dma::Config config;
   config.sourceIncrement = true;
   config.destinationIncrement = true;
   if(!channel.start(config, dmaSource, dmaTarget, 32) || !waitDone(channel) || !targetMatches(0, 32)) {
      return false;
   }

   config.sourceSize = Dma::DataSize::Size32Bits;
   config.destinationSize = Dma::DataSize::Size32Bits;
   config.completeEvent = Dma::CompleteEvent::Channel;
   dmaList[1] = Dma::LinkedListItem::make(config, dmaSource + 32, dmaTarget + 32, 16, nullptr);
   dmaList[0] = Dma::LinkedListItem::make(config, dmaSource + 48, dmaTarget + 48, 16, &dmaList[1]);
   for(auto &byte : dmaTarget) {
      byte = 0;
   }
   channel.clearAllFlags();
   return channel.startList(dmaList[0]) && waitDone(channel) && targetMatches(32, 32);
}

void selfTest() {
   Rcc::enableGPIOBClock();
   Rcc::enableUSART2Clock();
   alternate(GPIOA, 2, Pin::AlternateFunction::AF7);
   alternate(GPIOA, 3, Pin::AlternateFunction::AF7, Pin::Pull::Up);
   startUsart(vcp);
   print("\r\nsystem-stm32 STM32H5 test image\r\n");
   printResult("usart1 loopback (PA9-PA10)", usartLoopback());
   printResult("spi2 loopback (PB15-PB14)", spiLoopback());
   printResult("gpdma memory-to-memory", exerciseDma());
   const bool adcOk = startAdc();
   printResult("adc1 calibration", adcOk);
   if(!adcOk) {
      return;
   }
   uint32_t mv = 0;
   if(adc.readSupplyMillivolts(mv)) {
      vddaMv = mv;
      print("vdda: ");
      print(vddaMv);
      print(" mV\r\n");
   } else {
      printResult("vdda", false);
   }
   printPb0();
}
} // namespace

int main() {
   Rcc::enableGPIOAClock();
   Rcc::enableGPIOCClock();

   // PB13/PB14 (UCPD1 CC1/CC2) are SPI2 on the Versaloon pin map.
   hw::Pwr::disableUcpdDeadBattery();

   Led::pin().configure(Pin::Mode::Output, Pin::OutputType::PushPull);
   // Links the direction switch; LD2 ends as an output.
   Led::setInput();
   Led::setOutput();
   Pin key(GPIOC, 13);
   key.configure(Pin::Mode::Input);

   const bool clockOk = key.read() ? Rcc::setClock250MhzHSI() : Rcc::setClock250MhzHSE();
   Rcc::enableICache();
   exerciseUsbClock();
   exerciseUsb();
   sink = Rcc::getAPBClock() + Rcc::getAPB2Clock() + Rcc::getAPB3Clock() + Rcc::getPLL1QClock();
   sink = hw::Uid::hash();
   selfTest();

   // Enable first: the H5 updates PR and RLR only while LSI runs.
   hw::Dbg::freezeIwdgOnDebug();
   const hw::Iwdg iwdg(IWDG);
   iwdg.enable();
   iwdg.configure(hw::Iwdg::Prescaler::Div64, 1000); // 32 kHz / 64 / 1000: 2 s
   iwdg.reload();

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
         Led::write(!Led::read());
         iwdg.reload();
         delaymS(100);
      }
   }
   SysTick_Config(SystemCoreClock / 1000);
   timestamp_t next = 500;
   for(;;) {
      if(getUptimeTicks() >= next) {
         next += 500;
         Led::write(!Led::read());
         iwdg.reload();
         printPb0();
      }
   }
}

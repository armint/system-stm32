// Minimal STM32H5 test image. CI links it to prove that the H5 part of the
// library builds, links and fits in memory: start-up, vector table, fault
// handlers, the interrupt table copy, the DWT delay, the RCC with the 250 MHz
// recipes, the ICACHE, HSI48, the CRS, GpioPin, FastPin, UID, DBG, IWDG and
// the UCPD dead-battery disable and the GPDMA (memory-to-memory, single and
// linked-list).
//
// On a NUCLEO-H533RE it sets 250 MHz from the 24 MHz HSE crystal. Hold B1
// (PC13) during reset to use HSI instead. LD2 (PA5) then toggles every
// 500 SysTick interrupts of 1 ms: it blinks at 1 Hz only if SystemCoreClock
// is the real core clock. If the clock recipe fails, LD2 blinks fast (5 Hz)
// on the reset clock (HSI, 32 MHz). If a DMA transfer fails, LD2 blinks fast
// on the selected clock. The IWDG (about 2 s) resets the board if the main
// loop stops; it stops while a debugger halts the core.
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
#include <peripherals/Timer.h>
#include <peripherals/Uid.h>
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
} // namespace

int main() {
   Rcc::enableGPIOAClock();
   Rcc::enableGPIOCClock();

   // PB13/PB14 (UCPD1 CC1/CC2) are SPI2 on the Versaloon pin map.
   hw::Pwr::disableUcpdDeadBattery();

   using Pin = hw::GpioPin;
   Led::pin().configure(Pin::Mode::Output, Pin::OutputType::PushPull);
   // Links the direction switch; LD2 ends as an output.
   Led::setInput();
   Led::setOutput();
   Pin key(GPIOC, 13);
   key.configure(Pin::Mode::Input);

   bool clockOk = key.read() ? Rcc::setClock250MhzHSI() : Rcc::setClock250MhzHSE();
   Rcc::enableICache();
   clockOk = exerciseDma() && clockOk;
   exerciseUsbClock();
   sink = Rcc::getAPBClock() + Rcc::getAPB2Clock() + Rcc::getAPB3Clock() + Rcc::getPLL1QClock();
   sink = hw::Uid::hash();

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
      }
   }
}

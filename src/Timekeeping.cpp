#include <cmsis_device.h>
#include <utils/Interrupt.h>
#include <utils/Timekeeping.h>

static inline volatile timestamp_t& systemTick() {
   static volatile timestamp_t _systemTick = 0;
   return _systemTick;
}
extern "C" void __attribute__((section(".after_vectors"))) SysTick_Handler(void) {
   sysTickUpdateHook(++systemTick());
}

timestamp_t getUptimeTicks() {
   return systemTick();
}

void __attribute__((weak)) sysTickUpdateHook(timestamp_t sysTick) {
   (void)sysTick;
}

timestamp_t getUptimeMilliTicks() {
   Interrupt::CriticalSection ics;
   timestamp_t subTick = ((SysTick->LOAD + 1) - static_cast<timestamp_t>(SysTick->VAL)) * 1000 / (SysTick->LOAD + 1);
   timestamp_t wholeTicks = systemTick() * 1000;
   return wholeTicks + subTick;
}

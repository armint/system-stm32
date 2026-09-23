#pragma once
#include <cmsis_device.h>

namespace Exti {

enum class TriggerEdge : uint8_t {
   Disabled = 0,
   Rising = 1,
   Falling = 2,
   Both = 3

};

enum class Port : uint8_t {
#ifdef GPIOA
   A = 0,
#endif
#ifdef GPIOB
   B = 1,
#endif
#ifdef GPIOC
   C = 2,
#endif
#ifdef GPIOD
   D = 3,
#endif
#ifdef GPIOE
   E = 4,
#endif
#ifdef GPIOH
   H = 5,
#endif
   Undefined = 255,
};

#if defined(STM32WL)
enum Line : uint8_t {
   GPIO0 = 0,
   GPIO1,
   GPIO2,
   GPIO3,
   GPIO4,
   GPIO5,
   GPIO6,
   GPIO7,
   GPIO8,
   GPIO9,
   GPIO10,
   GPIO11,
   GPIO12,
   GPIO13,
   GPIO14,
   GPIO15,
   PVD = 16,
   RTCAlarm = 17,
   RTCSsrUnderflow = 18,
   RTCTamperAndTimestamp = 19,
   RTCTamper = 19,
   RTCWakeup = 20,
   Comp1Out = 21,
   Comp2Out = 22,
   I2C1Wakeup = 23,
   I2C2Wakeup = 24,
   I2C3Wakeup = 25,
   USART1Wakeup = 26,
   USART2Wakeup = 27,
   LPUART1Wakeup = 28,
   LPTimer1 = 29,
   LPTimer2 = 30,
   LPTimer3 = 31,
   // Per RM0453 and ST's own HAL (stm32wlxx_hal_subghz.c HAL_SUBGHZ_Init(),
   // which does LL_EXTI_EnableIT_32_63(LL_EXTI_LINE_44)): the sub-GHz
   // radio's 3 IRQs are ORed onto EXTI line 44 (a "Direct" line -- no
   // port routing or edge-trigger configuration applies, only the IMR2
   // mask), and RFBUSY is on line 45 (a "Configurable" line). Previously
   // misnumbered as 36/37.
   //
   // IMPORTANT: as a Direct line, RadioIRQ has no PR2 pending bit --
   // isInterruptPending(RadioIRQ) is always false and
   // clearInterruptPending(RadioIRQ) is a no-op on hardware. The radio IRQ
   // source can only be acknowledged by issuing the SX126x ClearIrqStatus
   // command over SPI; callers must not rely on this API to clear it.
   RadioIRQ = 44,
   RadioBusy = 45,
};
#elif defined(STM32G0)
enum Line : uint8_t {
   GPIO0 = 0,
   GPIO1,
   GPIO2,
   GPIO3,
   GPIO4,
   GPIO5,
   GPIO6,
   GPIO7,
   GPIO8,
   GPIO9,
   GPIO10,
   GPIO11,
   GPIO12,
   GPIO13,
   GPIO14,
   GPIO15,
   RTCAlarm = 17,
   RTCTamperAndWakeup = 19,
   Comp1Out = 21,
   Comp2Out = 22,
   I2C1Wakeup = 23,
   USART1Wakeup = 24,
   USART2Wakeup = 25,
   LPUART1Wakeup = 26,
   LPTimer1 = 27,
   LPTimer2 = 28,
};
#else
enum Line : uint8_t {
   GPIO0 = 0,
   GPIO1,
   GPIO2,
   GPIO3,
   GPIO4,
   GPIO5,
   GPIO6,
   GPIO7,
   GPIO8,
   GPIO9,
   GPIO10,
   GPIO11,
   GPIO12,
   GPIO13,
   GPIO14,
   GPIO15,
   PVD,
   RTCAlarm,
   USBWakeup,
   RTCTamper,
   RTCWakeup,
   Comp1Out,
   Comp2Out,
   I2C1Wakeup,
   I2C3Wakeup,
   USART1Wakeup,
   USART2Wakeup,
   RESERVED1,
   LPUART1Wakeup,
   LPTimer1,
};
#endif

#if defined(STM32WL)
constexpr IRQn_Type gpioIRQs[16] = {
   EXTI0_IRQn,     EXTI1_IRQn,     EXTI2_IRQn,     EXTI3_IRQn,     EXTI4_IRQn,
   EXTI9_5_IRQn,   EXTI9_5_IRQn,   EXTI9_5_IRQn,   EXTI9_5_IRQn,   EXTI9_5_IRQn,
   EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn,
};
#else
constexpr IRQn_Type gpioIRQs[16] = {
   EXTI0_1_IRQn,  EXTI0_1_IRQn,  EXTI2_3_IRQn,  EXTI2_3_IRQn,  EXTI4_15_IRQn, EXTI4_15_IRQn,
   EXTI4_15_IRQn, EXTI4_15_IRQn, EXTI4_15_IRQn, EXTI4_15_IRQn, EXTI4_15_IRQn, EXTI4_15_IRQn,
   EXTI4_15_IRQn, EXTI4_15_IRQn, EXTI4_15_IRQn, EXTI4_15_IRQn,
};
#endif

// Lines >= 16 (PVD, RTC, comparators, radio, ...) have no GPIO port routing --
// SYSCFG_EXTICR only maps GPIO lines 0-15 to a port. Calling this for a line
// >= 16 is a no-op (see definition).
void setPortForExtiLine(const uint8_t line, const Port port);

void setTriggerEdge(const uint32_t mask, const TriggerEdge trigger);
TriggerEdge getTriggerEdge(const uint32_t mask);

// STM32WL's extended-bank lines (32-63) have their own RTSR2/FTSR2 trigger
// registers, separate from the RTSR1/FTSR1 bank the mask-based functions
// above operate on. Only lines with an actual "Configurable" line in that
// bank define the corresponding bit (e.g. line 45/RadioBusy); "Direct" lines
// (e.g. line 44/RadioIRQ) have no trigger-edge bit at all -- setting one is a
// silent no-op on hardware, matching the no-op these functions perform on it.
#if defined(EXTI_IMR2_IM44)
void setTriggerEdgeMask2(const uint32_t mask, const TriggerEdge trigger);
TriggerEdge getTriggerEdgeMask2(const uint32_t mask);
#endif

inline void setTriggerEdgeForLine(const uint8_t line, const TriggerEdge trigger) {
#if defined(EXTI_IMR2_IM44)
   if(line >= 32) {
      setTriggerEdgeMask2(1UL << (line - 32), trigger);
      return;
   }
#endif
   setTriggerEdge(1UL << line, trigger);
}

inline TriggerEdge getTriggerEdgeForLine(const uint8_t line) {
#if defined(EXTI_IMR2_IM44)
   if(line >= 32) {
      return getTriggerEdgeMask2(1UL << (line - 32));
   }
#endif
   return getTriggerEdge(1UL << line);
}

void configureExtiInterrupt(const uint8_t line, const Port port, const TriggerEdge edge);

inline void enableInterruptMask(const uint32_t mask) {
#if defined(EXTI_IMR1_IM0)
   EXTI->IMR1 |= mask;
#else
   EXTI->IMR |= mask;
#endif
}

inline void disableInterruptMask(const uint32_t mask) {
#if defined(EXTI_IMR1_IM0)
   EXTI->IMR1 &= ~mask;
#else
   EXTI->IMR &= ~mask;
#endif
}

inline void enableEventMask(const uint32_t mask) {
#if defined(EXTI_IMR1_IM0)
   EXTI->EMR1 |= mask;
#else
   EXTI->EMR |= mask;
#endif
}

inline void disableEventMask(const uint32_t mask) {
#if defined(EXTI_IMR1_IM0)
   EXTI->EMR1 &= ~mask;
#else
   EXTI->EMR &= ~mask;
#endif
}

// STM32WL's radio-related lines (RadioIRQ = 44, RadioBusy = 45) live in the
// extended bank (IMR2/EMR2/PR2, lines 32-63), separate from the IMR1/EMR1/PR1
// bank the mask-based helpers above operate on. EXTI_IMR2_IM44 gates this on
// chips that actually have that bank and that line (STM32WL only, here).
#if defined(EXTI_IMR2_IM44)
inline void enableInterruptMask2(const uint32_t mask) {
   EXTI->IMR2 |= mask;
}

inline void disableInterruptMask2(const uint32_t mask) {
   EXTI->IMR2 &= ~mask;
}

inline void enableEventMask2(const uint32_t mask) {
   EXTI->EMR2 |= mask;
}

inline void disableEventMask2(const uint32_t mask) {
   EXTI->EMR2 &= ~mask;
}
#endif

inline void enableInterrupt(const uint8_t line) {
#if defined(EXTI_IMR2_IM44)
   if(line >= 32) {
      enableInterruptMask2(1UL << (line - 32));
      return;
   }
#endif
   enableInterruptMask(1UL << line);
}

inline void disableInterrupt(const uint8_t line) {
#if defined(EXTI_IMR2_IM44)
   if(line >= 32) {
      disableInterruptMask2(1UL << (line - 32));
      return;
   }
#endif
   disableInterruptMask(1UL << line);
}

inline void enableEvent(const uint8_t line) {
#if defined(EXTI_IMR2_IM44)
   if(line >= 32) {
      enableEventMask2(1UL << (line - 32));
      return;
   }
#endif
   enableEventMask(1UL << line);
}

inline void disableEvent(const uint8_t line) {
#if defined(EXTI_IMR2_IM44)
   if(line >= 32) {
      disableEventMask2(1UL << (line - 32));
      return;
   }
#endif
   disableEventMask(1UL << line);
}


inline bool isInterruptMasked(const uint8_t line) {
#if defined(EXTI_IMR2_IM44)
   if(line >= 32) {
      return !(EXTI->IMR2 & (1UL << (line - 32)));
   }
#endif
#if defined(EXTI_IMR1_IM0)
   return !(EXTI->IMR1 & (1UL << line));
#else
   return !(EXTI->IMR & (1UL << line));
#endif
}

// Note: "Direct" extended-bank lines (e.g. RadioIRQ = 44) have no PR2 bit at
// all -- for those, isInterruptPending() always returns false and
// clearInterruptPending() is a hardware no-op. Only "Configurable" lines
// (e.g. RadioBusy = 45) have a real pending bit here.
inline bool isInterruptPending(const uint8_t line) {
#if defined(EXTI_IMR2_IM44)
   if(line >= 32) {
      return EXTI->PR2 & (1UL << (line - 32));
   }
#endif
#if defined(EXTI_RPR1_RPIF0) || defined(EXTI_RPR1_RPIF)
   return (EXTI->RPR1 | EXTI->FPR1) & (1UL << line);
#elif defined(EXTI_PR1_PIF0) || defined(EXTI_PR1_PIF)
   return EXTI->PR1 & (1UL << line);
#else
   return EXTI->PR & (1UL << line);
#endif
}

inline void clearInterruptPending(const uint8_t line) {
#if defined(EXTI_IMR2_IM44)
   if(line >= 32) {
      EXTI->PR2 = (1UL << (line - 32));
      return;
   }
#endif
#if defined(EXTI_RPR1_RPIF0) || defined(EXTI_RPR1_RPIF)
   EXTI->RPR1 = (1UL << line);
   EXTI->FPR1 = (1UL << line);
#elif defined(EXTI_PR1_PIF0) || defined(EXTI_PR1_PIF)
   EXTI->PR1 = (1UL << line);
#else
   EXTI->PR = (1UL << line);
#endif
}

inline void clearInterrruptPending(const uint8_t line) {
   clearInterruptPending(line);
}

constexpr Port getPortFromGPIO(GPIO_TypeDef *const gpio) {
   Port p;
   switch(reinterpret_cast<uint32_t>(gpio)) {
#ifdef GPIOA
   case GPIOA_BASE:
      p = Port::A;
      break;
#endif
#ifdef GPIOB
   case GPIOB_BASE:
      p = Port::B;
      break;
#endif
#ifdef GPIOC
   case GPIOC_BASE:
      p = Port::C;
      break;
#endif
#ifdef GPIOD
   case GPIOD_BASE:
      p = Port::D;
      break;
#endif
#ifdef GPIOE
   case GPIOE_BASE:
      p = Port::E;
      break;
#endif
#ifdef GPIOH
   case GPIOH_BASE:
      p = Port::H;
      break;
#endif
   default:
      p = Port::Undefined;
   }
   return p;
}

constexpr IRQn_Type getExtiIRQnForGPIOLine(uint8_t line) {
   if(line > 15) {
      return static_cast<IRQn_Type>(-15); // undefined IRQn
   }
   return gpioIRQs[line];
}

} // namespace Exti


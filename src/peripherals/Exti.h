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

enum Line : uint8_t {
   GPIOLine0 = 0,
   GPIOLine1,
   GPIOLine2,
   GPIOLine3,
   GPIOLine4,
   GPIOLine5,
   GPIOLine6,
   GPIOLine7,
   GPIOLine8,
   GPIOLine9,
   GPIOLine10,
   GPIOLine11,
   GPIOLine12,
   GPIOLine13,
   GPIOLine14,
   GPIOLine15,
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

constexpr IRQn_Type gpioIRQs[16] = {
   EXTI0_1_IRQn,  EXTI0_1_IRQn,  EXTI2_3_IRQn,  EXTI2_3_IRQn,  EXTI4_15_IRQn, EXTI4_15_IRQn,
   EXTI4_15_IRQn, EXTI4_15_IRQn, EXTI4_15_IRQn, EXTI4_15_IRQn, EXTI4_15_IRQn, EXTI4_15_IRQn,
   EXTI4_15_IRQn, EXTI4_15_IRQn, EXTI4_15_IRQn, EXTI4_15_IRQn,
};

void setPortForExtiLine(const uint8_t line, const Port port);

void setTriggerEdge(const uint32_t mask, const TriggerEdge trigger);
TriggerEdge getTriggerEdge(const uint32_t mask);

inline void setTriggerEdgeForLine(const uint8_t line, const TriggerEdge trigger) {
   setTriggerEdge(1UL << line, trigger);
}

inline TriggerEdge getTriggerEdgeForLine(const uint8_t line) {
   return getTriggerEdge(1UL << line);
}

void configureExtiInterrupt(const uint8_t line, const Port port, const TriggerEdge edge);

inline void enableInterruptMask(const uint32_t mask) {
   EXTI->IMR |= mask;
}

inline void disableInterruptMask(const uint32_t mask) {
   EXTI->IMR &= ~mask;
}

inline void enableEventMask(const uint32_t mask) {
   EXTI->EMR |= mask;
}

inline void disableEventMask(const uint32_t mask) {
   EXTI->EMR &= ~mask;
}

inline void enableInterrupt(const uint8_t line) {
   enableInterruptMask(1 << line);
}

inline void disableInterrupt(const uint8_t line) {
   disableInterruptMask(1 << line);
}

inline void enableEvent(const uint8_t line) {
   enableEventMask(1 << line);
}

inline void disableEvent(const uint8_t line) {
   disableEventMask(1 << line);
}


inline bool isInterruptMasked(const uint8_t line) {
   return !(EXTI->IMR & (1 << line));
}

inline bool isInterruptPending(const uint8_t line) {
   return EXTI->PR & (1 << line);
}

inline void clearInterrruptPending(const uint8_t line) {
   EXTI->PR = (1 << line);
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

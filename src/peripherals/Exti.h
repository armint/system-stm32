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

extern const IRQn_Type gpioIRQs[16];

void setPortForExtiLine(uint8_t line, Port port);

void setTriggerEdge(uint32_t mask, TriggerEdge trigger);
TriggerEdge getTriggerEdge(uint32_t mask);

inline void setTriggerEdgeForLine(uint8_t line, TriggerEdge trigger) {
   setTriggerEdge(1UL << line, trigger);
}

inline TriggerEdge getTriggerEdgeForLine(uint8_t line) {
   return getTriggerEdge(1UL << line);
}

void configureExtiInterrupt(uint8_t line, Port port, TriggerEdge edge);

inline void enableInterruptMask(uint32_t mask) {
   EXTI->IMR |= mask;
}

inline void disableInterruptMask(uint32_t mask) {
   EXTI->IMR &= ~mask;
}

inline void enableEventMask(uint32_t mask) {
   EXTI->EMR |= mask;
}

inline void disableEventMask(uint32_t mask) {
   EXTI->EMR &= ~mask;
}

inline void enableInterrupt(uint8_t line) {
   enableInterruptMask(1 << line);
}

inline void disableInterrupt(uint8_t line) {
   disableInterruptMask(1 << line);
}

inline void enableEvent(uint8_t line) {
   enableEventMask(1 << line);
}

inline void disableEvent(uint8_t line) {
   disableEventMask(1 << line);
}


inline bool isInterruptMasked(uint8_t line) {
   return !(EXTI->IMR & (1 << line));
}

inline bool isInterruptPending(uint8_t line) {
   return EXTI->PR & (1 << line);
}

inline void clearInterrruptPending(uint8_t line) {
   EXTI->PR = (1 << line);
}

Port getPortFromGPIO(GPIO_TypeDef *gpio);

IRQn_Type getExtiIRQnForGPIOLine(uint8_t line);
} // namespace Exti

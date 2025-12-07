/*
 * Exti.cpp
 *
 *  Created on: Jun 23, 2016
 *      Author: armin
 */
#include "Exti.h"

namespace Exti {
void setTriggerEdge(uint32_t mask, TriggerEdge trigger) {
   switch(trigger) {
   case TriggerEdge::Disabled:
      EXTI->RTSR &= ~mask;
      EXTI->FTSR &= ~mask;
      break;
   case TriggerEdge::Both:
      EXTI->RTSR |= mask;
      EXTI->FTSR |= mask;
      break;
   case TriggerEdge::Rising:
      EXTI->RTSR |= mask;
      EXTI->FTSR &= ~mask;
      break;
   case TriggerEdge::Falling:
      EXTI->RTSR &= ~mask;
      EXTI->FTSR |= mask;
      break;
   }
}

TriggerEdge getTriggerEdge(uint32_t mask) {
   uint8_t edge = 0;
   if(EXTI->RTSR & mask) {
      edge |= static_cast<uint8_t>(TriggerEdge::Rising);
   }
   if(EXTI->FTSR & mask) {
      edge |= static_cast<uint8_t>(TriggerEdge::Falling);
   }
   return static_cast<TriggerEdge>(edge);
}

void configureExtiInterrupt(uint8_t line, Port port, TriggerEdge edge) {
   setPortForExtiLine(line, port);
   uint32_t mask = 1 << line;
   setTriggerEdge(mask, edge);
}

Port getPortFromGPIO(GPIO_TypeDef *gpio) {
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

constexpr IRQn_Type gpioIRQs[16] = {
   EXTI0_1_IRQn,  EXTI0_1_IRQn,  EXTI2_3_IRQn,  EXTI2_3_IRQn,  EXTI4_15_IRQn, EXTI4_15_IRQn,
   EXTI4_15_IRQn, EXTI4_15_IRQn, EXTI4_15_IRQn, EXTI4_15_IRQn, EXTI4_15_IRQn, EXTI4_15_IRQn,
   EXTI4_15_IRQn, EXTI4_15_IRQn, EXTI4_15_IRQn, EXTI4_15_IRQn,
};
IRQn_Type getExtiIRQnForGPIOLine(uint8_t line) {
   if(line > 15) {
      return static_cast<IRQn_Type>(-15); // undefined IRQn
   }
   return gpioIRQs[line];
}

void setPortForExtiLine(uint8_t line, Port port) {
   uint8_t regnum = (line & 0xC) >> 2;
   uint8_t index = (line & 0x3) << 2;
   SYSCFG->EXTICR[regnum] = (SYSCFG->EXTICR[regnum] & ~(0x3 << index)) | (static_cast<uint16_t>(port) << index);
}

} // namespace Exti

/*
 * Exti.cpp
 *
 *  Created on: Jun 23, 2016
 *      Author: armin
 */
#include "Exti.h"

namespace Exti {
void setTriggerEdge(const uint32_t mask, const TriggerEdge trigger) {
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

TriggerEdge getTriggerEdge(const uint32_t mask) {
   uint8_t edge = 0;
   if(EXTI->RTSR & mask) {
      edge |= static_cast<uint8_t>(TriggerEdge::Rising);
   }
   if(EXTI->FTSR & mask) {
      edge |= static_cast<uint8_t>(TriggerEdge::Falling);
   }
   return static_cast<TriggerEdge>(edge);
}

void configureExtiInterrupt(const uint8_t line, const Port port, const TriggerEdge edge) {
   setPortForExtiLine(line, port);
   uint32_t mask = 1 << line;
   setTriggerEdge(mask, edge);
}


void setPortForExtiLine(const uint8_t line, const Port port) {
   uint8_t regnum = (line & 0xC) >> 2;
   uint8_t index = (line & 0x3) << 2;
   SYSCFG->EXTICR[regnum] = (SYSCFG->EXTICR[regnum] & ~(0x3 << index)) | (static_cast<uint16_t>(port) << index);
}

} // namespace Exti

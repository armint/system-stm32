/*
 * Exti.cpp
 *
 *  Created on: Jun 23, 2016
 *      Author: armin
 */
#include "Exti.h"

namespace Exti {
void setTriggerEdge(const uint32_t mask, const TriggerEdge trigger) {
#if defined(EXTI_IMR1_IM0)
   switch(trigger) {
   case TriggerEdge::Disabled:
      EXTI->RTSR1 &= ~mask;
      EXTI->FTSR1 &= ~mask;
      break;
   case TriggerEdge::Both:
      EXTI->RTSR1 |= mask;
      EXTI->FTSR1 |= mask;
      break;
   case TriggerEdge::Rising:
      EXTI->RTSR1 |= mask;
      EXTI->FTSR1 &= ~mask;
      break;
   case TriggerEdge::Falling:
      EXTI->RTSR1 &= ~mask;
      EXTI->FTSR1 |= mask;
      break;
   }
#else
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
#endif
}

TriggerEdge getTriggerEdge(const uint32_t mask) {
   uint8_t edge = 0;
#if defined(EXTI_IMR1_IM0)
   if(EXTI->RTSR1 & mask) {
      edge |= static_cast<uint8_t>(TriggerEdge::Rising);
   }
   if(EXTI->FTSR1 & mask) {
      edge |= static_cast<uint8_t>(TriggerEdge::Falling);
   }
#else
   if(EXTI->RTSR & mask) {
      edge |= static_cast<uint8_t>(TriggerEdge::Rising);
   }
   if(EXTI->FTSR & mask) {
      edge |= static_cast<uint8_t>(TriggerEdge::Falling);
   }
#endif
   return static_cast<TriggerEdge>(edge);
}

void configureExtiInterrupt(const uint8_t line, const Port port, const TriggerEdge edge) {
   setPortForExtiLine(line, port);
   uint32_t mask = 1 << line;
   setTriggerEdge(mask, edge);
}


void setPortForExtiLine(const uint8_t line, const Port port) {
#if defined(EXTI_EXTICR1_EXTI0) || defined(EXTI_EXTICR1_EXTI0_Pos)
   uint8_t regnum = (line & 0xC) >> 2;
   uint8_t index = (line & 0x3) << 3; // 8 bits per line in G0/C0
   EXTI->EXTICR[regnum] = (EXTI->EXTICR[regnum] & ~(0xFF << index)) | (static_cast<uint16_t>(port) << index);
#elif defined(SYSCFG_EXTICR1_EXTI0) || defined(SYSCFG_EXTICR1_EXTI0_Pos) || defined(SYSCFG_EXTICR1_EXTI0_PA)
   uint8_t regnum = (line & 0xC) >> 2;
   uint8_t index = (line & 0x3) << 2; // 4 bits per line in L0/WL
   SYSCFG->EXTICR[regnum] = (SYSCFG->EXTICR[regnum] & ~(0xF << index)) | (static_cast<uint16_t>(port) << index);
#endif
}

} // namespace Exti

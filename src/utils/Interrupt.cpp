
#include "Interrupt.h"
#include <cassert>
#include <utility>

#define VTOR_ADDRESS SRAM_BASE

extern uintptr_t __vectors_start;

namespace Interrupt {
static inline void prepareVectorTable(void) {
   auto *p = reinterpret_cast<uint32_t *>(VTOR_ADDRESS);
   uint32_t *end = p + VECTOR_TABLE_SIZE;
   uint32_t *from = reinterpret_cast<uint32_t *>(&__vectors_start);
   while(p < end) {
      *p++ = *from++;
   }
   CriticalSection ics;
   __DSB();
   SCB->VTOR = VTOR_ADDRESS;
}

static inline AdvancedHandler_t& getAdvancedHandler(unsigned int index) {
   static AdvancedHandler_t _advancedHandlers[VECTOR_TABLE_SIZE - 16]; // We only don't support object handlers for system interrupts
   return _advancedHandlers[index];
}


inline void handlerWithArgument() {
   getAdvancedHandler(__get_IPSR() - 16)();
}

void connectHandler(IRQn_Type IRQn, AdvancedHandler_t handler) {
   assert(static_cast<int>(IRQn) >= 0);
   getAdvancedHandler(static_cast<int>(IRQn)) = std::move(handler);
   connectHandler(IRQn, &handlerWithArgument);
}


void connectHandler(IRQn_Type IRQn, Handler_t handler) {
   auto *vTable = reinterpret_cast<Handler_t *>(VTOR_ADDRESS);
   if(SCB->VTOR != VTOR_ADDRESS) {
      prepareVectorTable();
   }
   vTable[IRQn + 16] = handler;
   __DSB();
}

void releaseHandler(IRQn_Type IRQn) {
   auto *vTableOrig = reinterpret_cast<Handler_t *>(&__vectors_start);
   connectHandler(IRQn, vTableOrig[IRQn + 16]);
}

AdvancedHandler_t getObjectHandler(IRQn_Type irq) {
   int i = static_cast<int>(irq);
   if(i < 0 || i >= VECTOR_TABLE_SIZE) {
      return nullptr;
   }
   return getAdvancedHandler(i);
}

Handler_t getHandler(IRQn_Type irq) {
   auto *handlers = reinterpret_cast<Handler_t *>(SCB->VTOR);
   int index = static_cast<int>(irq) + 16;
   return (index < 0 || index > VECTOR_TABLE_SIZE) ? nullptr : handlers[index];
}

BlockingSection::BlockingSection(IRQn_Type IRQn) {
   if(NVIC_GetEnableIRQ(IRQn)) {
      irqn = static_cast<int8_t>(IRQn);
      NVIC_DisableIRQ(IRQn);
   } else {
      irqn = INT8_MIN;
   }
}

BlockingSection::~BlockingSection() {
   if(irqn != INT8_MIN) {
      NVIC_EnableIRQ(static_cast<IRQn_Type>(irqn));
   }
}

CriticalSection::CriticalSection() {
   // Read the current PRIMASK, to be later restored
   _state = __get_PRIMASK();
   // Disable all interrupts
   __disable_irq();
}

CriticalSection::~CriticalSection() {
   // Restore PRIMASK to the saved value
   __set_PRIMASK(_state);
}
}; // namespace Interrupt

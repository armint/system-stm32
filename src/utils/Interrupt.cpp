
#include "Interrupt.h"
#include <cassert>
#include <cstddef>
#include <utility>

extern uintptr_t __vectors_start;

namespace Interrupt {
// VTOR needs the table aligned to the next power of two of its size, at least
// 128 bytes (TBLOFF starts at bit 7).
static constexpr size_t vectorTableAlignment(size_t size) {
   size_t alignment = 128;
   while(alignment < size) {
      alignment *= 2;
   }
   return alignment;
}

// The RAM copy of the vector table. Its own .bss.* section lets the linker
// place it with no linker script change, and --gc-sections removes it when
// nothing calls connectHandler(). The alignment can cost padding in .bss.
alignas(vectorTableAlignment(VECTOR_TABLE_SIZE * sizeof(Handler_t))) __attribute__((section(".bss.ramVectorTable")))
   static Handler_t ramVectorTable[VECTOR_TABLE_SIZE];

static inline void prepareVectorTable(void) {
   auto *from = reinterpret_cast<Handler_t *>(&__vectors_start);
   for(auto &vector : ramVectorTable) {
      vector = *from++;
   }
   CriticalSection ics;
   __DSB();
   SCB->VTOR = reinterpret_cast<uint32_t>(ramVectorTable);
}

struct AdvancedHandler {
   AdvancedHandler_t handler;
   void *argument;
};

static inline AdvancedHandler &getAdvancedHandler(unsigned int index) {
   static AdvancedHandler _advancedHandlers[VECTOR_TABLE_SIZE - 16]; // We only don't support object handlers for system interrupts
   return _advancedHandlers[index];
}


inline void handlerWithArgument() {
   auto &advanced_handler = getAdvancedHandler(__get_IPSR() - 16);
   advanced_handler.handler(advanced_handler.argument);
}

void connectHandler(IRQn_Type IRQn, AdvancedHandler_t handler, void *arg) {
   assert(static_cast<int>(IRQn) >= 0);
   auto &advanced_handler = getAdvancedHandler(static_cast<int>(IRQn));
   advanced_handler.handler = handler;
   advanced_handler.argument = arg;
   connectHandler(IRQn, &handlerWithArgument);
}


void connectHandler(IRQn_Type IRQn, Handler_t handler) {
   if(SCB->VTOR != reinterpret_cast<uint32_t>(ramVectorTable)) {
      prepareVectorTable();
   }
   ramVectorTable[IRQn + 16] = handler;
   __DSB();
}

void releaseHandler(IRQn_Type IRQn) {
   auto *vTableOrig = reinterpret_cast<Handler_t *>(&__vectors_start);
   connectHandler(IRQn, vTableOrig[IRQn + 16]);
}

AdvancedHandler getObjectHandler(IRQn_Type irq) {
   int i = static_cast<int>(irq);
   if(i < 0 || i >= VECTOR_TABLE_SIZE - 16) {
      return {};
   }
   return getAdvancedHandler(i);
}

Handler_t getHandler(IRQn_Type irq) {
   auto *handlers = reinterpret_cast<Handler_t *>(SCB->VTOR);
   int index = static_cast<int>(irq) + 16;
   return (index < 0 || index >= VECTOR_TABLE_SIZE) ? nullptr : handlers[index];
}

BlockingSection::BlockingSection(IRQn_Type IRQn) {
   if(NVIC_GetEnableIRQ(IRQn)) {
      irqn = static_cast<int16_t>(IRQn);
      NVIC_DisableIRQ(IRQn);
   } else {
      irqn = INT16_MIN;
   }
}

BlockingSection::~BlockingSection() {
   if(irqn != INT16_MIN) {
      NVIC_EnableIRQ(static_cast<IRQn_Type>(irqn));
   }
}

CriticalSection::CriticalSection()
   // Read the current PRIMASK, to be later restored
   : _state(__get_PRIMASK()) {
   // Disable all interrupts
   __disable_irq();
}

CriticalSection::~CriticalSection() {
   // Restore PRIMASK to the saved value
   __set_PRIMASK(_state);
}
}; // namespace Interrupt

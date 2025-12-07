#pragma once

#include <cmsis_device.h>
#include <functional>
#include <utility>

#if defined(STM32L0)
// STM32L0 processors have 32 interrupt vectors + 16 cortex-m vectors
#define VECTOR_TABLE_SIZE 48
#endif

namespace Interrupt {

typedef void (*Handler_t)(void);

using AdvancedHandler_t = std::function<void()>;


void handlerWithArgument();

void connectHandler(IRQn_Type IRQn, Handler_t handler);

void connectHandler(IRQn_Type IRQn, AdvancedHandler_t handler);

void releaseHandler(IRQn_Type IRQn);

AdvancedHandler_t getAdvancedHandler(IRQn_Type irq);

Handler_t getHandler(IRQn_Type irq);

class BlockingSection {
   int8_t irqn;

   public:
   explicit BlockingSection(IRQn_Type IRQn);

   virtual ~BlockingSection();
};

class CriticalSection {
   uint32_t _state;

   public:
   CriticalSection();
   ~CriticalSection();
};
} // namespace Interrupt

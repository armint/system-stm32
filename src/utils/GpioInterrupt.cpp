#include "../peripherals/Exti.h"
#include <algorithm>
#include <utility>
#include <utils/GpioInterrupt.h>

namespace GpioInterrupt {

struct LineConnection {
   Interrupt::AdvancedHandler_t handler;
   Exti::Port port;
};

template <uint8_t gpioline, IRQn_Type irq>
void GpioHandler();

template <uint8_t first, uint8_t last, IRQn_Type irq>
void GpioHandler();

#if defined(STM32L0)
constexpr Interrupt::Handler_t gpioHandlers[16] = {
   GpioHandler<0, 1, EXTI0_1_IRQn>,   GpioHandler<0, 1, EXTI0_1_IRQn>,   GpioHandler<2, 3, EXTI2_3_IRQn>,
   GpioHandler<2, 3, EXTI2_3_IRQn>,   GpioHandler<4, 15, EXTI4_15_IRQn>, GpioHandler<4, 15, EXTI4_15_IRQn>,
   GpioHandler<4, 15, EXTI4_15_IRQn>, GpioHandler<4, 15, EXTI4_15_IRQn>, GpioHandler<4, 15, EXTI4_15_IRQn>,
   GpioHandler<4, 15, EXTI4_15_IRQn>, GpioHandler<4, 15, EXTI4_15_IRQn>, GpioHandler<4, 15, EXTI4_15_IRQn>,
   GpioHandler<4, 15, EXTI4_15_IRQn>, GpioHandler<4, 15, EXTI4_15_IRQn>, GpioHandler<4, 15, EXTI4_15_IRQn>,
   GpioHandler<4, 15, EXTI4_15_IRQn>,
};
#elif defined(STM32L4)
constexpr InterruptHandler_t gpioHandlers[16] = {
   GpioHandler<0, EXTI0_IRQn>,          GpioHandler<1, EXTI1_IRQn>,          GpioHandler<2, EXTI2_IRQn>,
   GpioHandler<3, EXTI3_IRQn>,          GpioHandler<4, EXTI4_IRQn>,          GpioHandler<5, 9, EXTI9_5_IRQn>,
   GpioHandler<5, 9, EXTI9_5_IRQn>,     GpioHandler<5, 9, EXTI9_5_IRQn>,     GpioHandler<5, 9, EXTI9_5_IRQn>,
   GpioHandler<5, 9, EXTI9_5_IRQn>,     GpioHandler<10, 15, EXTI15_10_IRQn>, GpioHandler<10, 15, EXTI15_10_IRQn>,
   GpioHandler<10, 15, EXTI15_10_IRQn>, GpioHandler<10, 15, EXTI15_10_IRQn>, GpioHandler<10, 15, EXTI15_10_IRQn>,
   GpioHandler<10, 15, EXTI15_10_IRQn>,
};
#else
#error No GPIO interrupts defined for this architecture
#endif

inline LineConnection& getLineConnection(uint8_t index) {
   static LineConnection lineConnections[16] = {
      { .handler = nullptr, .port = Exti::Port::Undefined }, { .handler = nullptr, .port = Exti::Port::Undefined },
      { .handler = nullptr, .port = Exti::Port::Undefined }, { .handler = nullptr, .port = Exti::Port::Undefined },
      { .handler = nullptr, .port = Exti::Port::Undefined }, { .handler = nullptr, .port = Exti::Port::Undefined },
      { .handler = nullptr, .port = Exti::Port::Undefined }, { .handler = nullptr, .port = Exti::Port::Undefined },
      { .handler = nullptr, .port = Exti::Port::Undefined }, { .handler = nullptr, .port = Exti::Port::Undefined },
      { .handler = nullptr, .port = Exti::Port::Undefined }, { .handler = nullptr, .port = Exti::Port::Undefined },
      { .handler = nullptr, .port = Exti::Port::Undefined }, { .handler = nullptr, .port = Exti::Port::Undefined },
      { .handler = nullptr, .port = Exti::Port::Undefined }, { .handler = nullptr, .port = Exti::Port::Undefined },
   };
   return lineConnections[index];


}
template <uint8_t gpioline, IRQn_Type irq>
void GpioHandler() {
   if(Exti::isInterruptPending(gpioline) && !Exti::isInterruptMasked(gpioline)) {
      if(getLineConnection(gpioline).handler != nullptr) {
#if defined(TRACE_GPIO_INTERRUPT)
         trace_printf("I: %d\r\n", i);
#endif
         getLineConnection(gpioline).handler();
      }
   }
   Exti::clearInterrruptPending(gpioline);
   NVIC_ClearPendingIRQ(irq);
}

template <uint8_t first, uint8_t last, IRQn_Type irq>
void GpioHandler() {
   for(int i = first; i <= last; i++) {
      if(Exti::isInterruptPending(i) && !Exti::isInterruptMasked(i) && getLineConnection(i).handler != nullptr) {
         getLineConnection(i).handler();
      }
      Exti::clearInterrruptPending(i);
   }
   NVIC_ClearPendingIRQ(irq);
}


bool connectHandler(Interrupt::AdvancedHandler_t handler, GPIO_TypeDef *port, uint8_t pin, Exti::TriggerEdge edge, bool enableInterruptLine) {
   if(pin > 15) {
      return false;
   }
   Exti::Port extiPort = Exti::getPortFromGPIO(port);
   if(getLineConnection(pin).handler != nullptr && getLineConnection(pin).port != extiPort) {
      return false;
   }
   LineConnection &connection = getLineConnection(pin);
   IRQn_Type irq = Exti::getExtiIRQnForGPIOLine(pin);
   connection.port = extiPort;
   connection.handler = std::move(handler);
   NVIC_DisableIRQ(irq);
   Exti::setPortForExtiLine(pin, Exti::getPortFromGPIO(port));
   Exti::setTriggerEdgeForLine(pin, edge);
   if(enableInterruptLine) {
      Exti::enableInterrupt(pin);
   }
   NVIC_ClearPendingIRQ(irq);
   Interrupt::connectHandler(irq, gpioHandlers[pin]);
   NVIC_EnableIRQ(irq);
   return true;
}

bool disconnectHandler(GPIO_TypeDef *port, uint8_t pin) {
   if(pin > 15 || Exti::getPortFromGPIO(port) != getLineConnection(pin).port) {
      return false;
   }
   IRQn_Type irq = Exti::getExtiIRQnForGPIOLine(pin);
   uint8_t connectCounter = 0;
   for(uint8_t i = 0; i < 16; i++) {
      if(Exti::gpioIRQs[i] == irq && getLineConnection(i).handler != nullptr && i != pin) {
         connectCounter++;
      }
   }
   NVIC_DisableIRQ(irq);
   getLineConnection(pin) = { .handler = nullptr, .port = Exti::Port::Undefined };
   Exti::disableInterrupt(pin);
   Exti::clearInterrruptPending(pin);
   if(connectCounter > 0) {
      // reenable IRQ
      NVIC_EnableIRQ(irq);
   } else {
      NVIC_ClearPendingIRQ(irq);
   }
   return true;
}
} // namespace GpioInterrupt

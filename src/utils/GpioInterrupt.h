#pragma once

#include "../peripherals/Exti.h"
#include "Interrupt.h"
#include <cmsis_device.h>

namespace GpioInterrupt {

bool connectHandler(Interrupt::AdvancedHandler_t handler,
                    void *handler_arg,
                    GPIO_TypeDef *port,
                    uint8_t pin,
                    Exti::TriggerEdge edge,
                    bool enableInterruptLine = true);

template <typename T, void (T::*Method)()>
static bool connectHandler(T &instance, GPIO_TypeDef *port, uint8_t pin, Exti::TriggerEdge edge, bool enableInterruptLine = true) {
   return connectHandler(&Interrupt::_memberThunk<T, Method>, static_cast<void *>(&instance), port, pin, edge, enableInterruptLine);
}
bool disconnectHandler(GPIO_TypeDef *port, uint8_t pin);

}; // namespace GpioInterrupt

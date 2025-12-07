#pragma once

#include "../peripherals/Exti.h"
#include "Interrupt.h"
#include <cmsis_device.h>

namespace GpioInterrupt {

bool connectHandler(Interrupt::AdvancedHandler_t handler, GPIO_TypeDef *port, uint8_t pin, Exti::TriggerEdge edge, bool enableInterruptLine = true);

bool disconnectHandler(GPIO_TypeDef *port, uint8_t pin);

}; // namespace GpioInterrupt

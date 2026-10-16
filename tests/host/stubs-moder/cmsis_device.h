#pragma once

// Host stand-in for a CMSIS device header with the MODER GPIO (L0, G0, WL,
// H5): only what the host-tested sources use. Register layout from
// stm32h533xx.h. GPIOA_BASE is the address where the test maps a fake port.

#include <cstdint>

struct GPIO_TypeDef {
   volatile uint32_t MODER;
   volatile uint32_t OTYPER;
   volatile uint32_t OSPEEDR;
   volatile uint32_t PUPDR;
   volatile uint32_t IDR;
   volatile uint32_t ODR;
   volatile uint32_t BSRR;
   volatile uint32_t LCKR;
   volatile uint32_t AFR[2];
   volatile uint32_t BRR;
};

#define GPIOA_BASE 0x42020000UL

// PRIMASK: defined by the test, so it can check the critical sections.
uint32_t __get_PRIMASK();
void __set_PRIMASK(uint32_t primask);
void __disable_irq();

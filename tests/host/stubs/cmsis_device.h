#pragma once

// Host stand-in for the STM32F1 CMSIS device header: only what the
// host-tested sources use. Values from stm32f103xb.h.

#include <cstdint>

#define STM32F1

struct GPIO_TypeDef {
   volatile uint32_t CRL;
   volatile uint32_t CRH;
   volatile uint32_t IDR;
   volatile uint32_t ODR;
   volatile uint32_t BSRR;
   volatile uint32_t BRR;
   volatile uint32_t LCKR;
};

#define PERIPH_BASE 0x40000000UL
#define PERIPH_BB_BASE 0x42000000UL
#define GPIOA_BASE (PERIPH_BASE + 0x00010800UL)
#define GPIOB_BASE (PERIPH_BASE + 0x00010C00UL)
#define GPIO_CRL_MODE0 0x00000003UL

enum IRQn_Type { TIM2_IRQn = 28 };

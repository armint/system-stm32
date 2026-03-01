//
// This file is part of the µOS++ III distribution.
// Copyright (c) 2014 Liviu Ionescu.
//

// ----------------------------------------------------------------------------

#include "cmsis-plus/cortexm/exception-handlers.h"

// ----------------------------------------------------------------------------
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

void __attribute__((weak)) Default_Handler(void);

// Forward declaration of the specific IRQ handlers. These are aliased
// to the Default_Handler, which is a 'forever' loop. When the application
// defines a handler (with the same name), this will automatically take
// precedence over these weak definitions

void __attribute__((weak, alias("Default_Handler"))) WWDG_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) PVD_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) RTC_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) FLASH_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) RCC_CRS_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) RCC_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) EXTI0_1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) EXTI2_3_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) EXTI4_15_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TSC_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) DMA1_Channel1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) DMA1_Channel2_3_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) DMA1_Channel4_5_6_7_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) ADC1_COMP_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) LPTIM1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) USART4_5_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM2_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM3_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM6_DAC_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM7_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM21_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) I2C3_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM22_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) I2C1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) I2C2_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) SPI1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) SPI2_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) USART1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) USART2_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) RNG_LPUART1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) LPUART1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) LCD_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) USB_IRQHandler(void);

// ----------------------------------------------------------------------------

extern unsigned int _estack;

typedef void (*const pHandler)(void);

// ----------------------------------------------------------------------------

// The vector table.
// This relies on the linker script to place at correct location in memory.

__attribute__((section(".isr_vector"), used)) pHandler g_pfnVectors[] = {
   // Core Level - CM0
   (pHandler)&_estack, // The initial stack pointer
   Reset_Handler,      // The reset handler

   NMI_Handler,       // The NMI handler
   HardFault_Handler, // The hard fault handler
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
   MemManage_Handler,  // The MPU fault handler
   BusFault_Handler,   // The bus fault handler
   UsageFault_Handler, // The usage fault handler
#else
   0, 0, 0, // Reserved
#endif
   0,           // Reserved
   0,           // Reserved
   0,           // Reserved
   0,           // Reserved
   SVC_Handler, // SVCall handler
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
   DebugMon_Handler, // Debug monitor handler
#else
   0,       // Reserved
#endif
   0,               // Reserved
   PendSV_Handler,  // The PendSV handler
   SysTick_Handler, // The SysTick handler

// ----------------------------------------------------------------------
#if defined(STM32L031xx)

   WWDG_IRQHandler,                //
   PVD_IRQHandler,                 //
   RTC_IRQHandler,                 //
   FLASH_IRQHandler,               //
   RCC_IRQHandler,                 //
   EXTI0_1_IRQHandler,             //
   EXTI2_3_IRQHandler,             //
   EXTI4_15_IRQHandler,            //
   0,                              //
   DMA1_Channel1_IRQHandler,       //
   DMA1_Channel2_3_IRQHandler,     //
   DMA1_Channel4_5_6_7_IRQHandler, //
   ADC1_COMP_IRQHandler,           //
   LPTIM1_IRQHandler,              //
   0,                              //
   TIM2_IRQHandler,                //
   0,                              //
   0,                              //
   0,                              //
   0,                              //
   TIM21_IRQHandler,               //
   0,                              //
   TIM22_IRQHandler,               //
   I2C1_IRQHandler,                //
   0,                              //
   SPI1_IRQHandler,                //
   0,                              //
   0,                              //
   USART2_IRQHandler,              //
   LPUART1_IRQHandler,             //
   0,                              //
   0,                              //


#elif defined(STM32L053xx)


   WWDG_IRQHandler,                //
   PVD_IRQHandler,                 //
   RTC_IRQHandler,                 //
   FLASH_IRQHandler,               //
   RCC_CRS_IRQHandler,             //
   EXTI0_1_IRQHandler,             //
   EXTI2_3_IRQHandler,             //
   EXTI4_15_IRQHandler,            //
   TSC_IRQHandler,                 //
   DMA1_Channel1_IRQHandler,       //
   DMA1_Channel2_3_IRQHandler,     //
   DMA1_Channel4_5_6_7_IRQHandler, //
   ADC1_COMP_IRQHandler,           //
   LPTIM1_IRQHandler,              //
   0,                              //
   TIM2_IRQHandler,                //
   0,                              //
   TIM6_DAC_IRQHandler,            //
   0,                              //
   0,                              //
   TIM21_IRQHandler,               //
   0,                              //
   TIM22_IRQHandler,               //
   I2C1_IRQHandler,                //
   I2C2_IRQHandler,                //
   SPI1_IRQHandler,                //
   SPI2_IRQHandler,                //
   USART1_IRQHandler,              //
   USART2_IRQHandler,              //
   RNG_LPUART1_IRQHandler,         //
   LCD_IRQHandler,                 //
   USB_IRQHandler,                 //

#elif defined(STM32L073xx)

   WWDG_IRQHandler,                /* Window WatchDog              */
   PVD_IRQHandler,                 /* PVD through EXTI Line detection */
   RTC_IRQHandler,                 /* RTC through the EXTI line     */
   FLASH_IRQHandler,               /* FLASH                        */
   RCC_CRS_IRQHandler,             /* RCC and CRS                  */
   EXTI0_1_IRQHandler,             /* EXTI Line 0 and 1            */
   EXTI2_3_IRQHandler,             /* EXTI Line 2 and 3            */
   EXTI4_15_IRQHandler,            /* EXTI Line 4 to 15            */
   TSC_IRQHandler,                 /* TSC                          */
   DMA1_Channel1_IRQHandler,       /* DMA1 Channel 1               */
   DMA1_Channel2_3_IRQHandler,     /* DMA1 Channel 2 and Channel 3 */
   DMA1_Channel4_5_6_7_IRQHandler, /* DMA1 Channel 4, Channel 5, Channel 6 and Channel 7*/
   ADC1_COMP_IRQHandler,           /* ADC1, COMP1 and COMP2        */
   LPTIM1_IRQHandler,              /* LPTIM1                       */
   USART4_5_IRQHandler,            /* USART4 and USART 5           */
   TIM2_IRQHandler,                /* TIM2                         */
   TIM3_IRQHandler,                /* TIM3                         */
   TIM6_DAC_IRQHandler,            /* TIM6 and DAC                 */
   TIM7_IRQHandler,                /* TIM7                         */
   0,                              /* Reserved                     */
   TIM21_IRQHandler,               /* TIM21                        */
   I2C3_IRQHandler,                /* I2C3                         */
   TIM22_IRQHandler,               /* TIM22                        */
   I2C1_IRQHandler,                /* I2C1                         */
   I2C2_IRQHandler,                /* I2C2                         */
   SPI1_IRQHandler,                /* SPI1                         */
   SPI2_IRQHandler,                /* SPI2                         */
   USART1_IRQHandler,              /* USART1                       */
   USART2_IRQHandler,              /* USART2                       */
   RNG_LPUART1_IRQHandler,         /* RNG and LPUART1              */
   LCD_IRQHandler,                 /* LCD                          */
   USB_IRQHandler,                 /* USB                          */

#elif defined(STM32L071xx)

   WWDG_IRQHandler,                /* Window WatchDog              */
   PVD_IRQHandler,                 /* PVD through EXTI Line detection */
   RTC_IRQHandler,                 /* RTC through the EXTI line     */
   FLASH_IRQHandler,               /* FLASH                        */
   RCC_CRS_IRQHandler,             /* RCC and CRS                  */
   EXTI0_1_IRQHandler,             /* EXTI Line 0 and 1            */
   EXTI2_3_IRQHandler,             /* EXTI Line 2 and 3            */
   EXTI4_15_IRQHandler,            /* EXTI Line 4 to 15            */
   0, DMA1_Channel1_IRQHandler,    /* DMA1 Channel 1               */
   DMA1_Channel2_3_IRQHandler,     /* DMA1 Channel 2 and Channel 3 */
   DMA1_Channel4_5_6_7_IRQHandler, /* DMA1 Channel 4, Channel 5, Channel 6 and Channel 7*/
   ADC1_COMP_IRQHandler,           /* ADC1, COMP1 and COMP2        */
   LPTIM1_IRQHandler,              /* LPTIM1                       */
   USART4_5_IRQHandler,            /* USART4 and USART 5           */
   TIM2_IRQHandler,                /* TIM2                         */
   TIM3_IRQHandler,                /* TIM3                         */
   TIM6_DAC_IRQHandler,            /* TIM6 and DAC                 */
   TIM7_IRQHandler,                /* TIM7                         */
   0,                              /* Reserved                     */
   TIM21_IRQHandler,               /* TIM21                        */
   I2C3_IRQHandler,                /* I2C3                         */
   TIM22_IRQHandler,               /* TIM22                        */
   I2C1_IRQHandler,                /* I2C1                         */
   I2C2_IRQHandler,                /* I2C2                         */
   SPI1_IRQHandler,                /* SPI1                         */
   SPI2_IRQHandler,                /* SPI2                         */
   USART1_IRQHandler,              /* USART1                       */
   USART2_IRQHandler,              /* USART2                       */
   RNG_LPUART1_IRQHandler,         /* RNG and LPUART1              */
   0, 0,

#else
#error "missing vectors"
#endif

   // Is it for stm32l as well. @0x108. This is for boot in RAM mode for STM32F0xx devices.
   (pHandler)0xF108F85F

};

// ----------------------------------------------------------------------------

// Processor ends up here if an unexpected interrupt occurs or a specific
// handler is not present in the application code.

void __attribute__((section(".after_vectors"))) Default_Handler(void) {
#if defined(DEBUG)
   __DEBUG_BKPT();
#endif
   while(1) {
   }
}
#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------

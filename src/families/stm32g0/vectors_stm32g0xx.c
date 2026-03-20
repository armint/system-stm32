// STM32G0 series interrupt vector table.
// Modeled after the STM32L0 version. Weak aliases allow application code
// to override individual handlers.

#include "cmsis-plus/cortexm/exception-handlers.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

void __attribute__((weak)) Default_Handler(void);

// Forward declarations — weak aliases to Default_Handler.
void __attribute__((weak, alias("Default_Handler"))) WWDG_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) PVD_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) PVD_VDDIO2_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) RTC_TAMP_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) FLASH_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) RCC_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) RCC_CRS_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) EXTI0_1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) EXTI2_3_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) EXTI4_15_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) UCPD1_2_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) USB_UCPD1_2_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) DMA1_Channel1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) DMA1_Channel2_3_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) DMA1_Ch4_7_DMAMUX1_OVR_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) DMA1_Ch4_7_DMA2_Ch1_5_DMAMUX1_OVR_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) ADC1_COMP_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM1_BRK_UP_TRG_COM_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM1_CC_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM2_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM3_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM3_TIM4_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM6_DAC_LPTIM1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM7_LPTIM2_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM14_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM15_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM16_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM16_FDCAN_IT0_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM17_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM17_FDCAN_IT1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) I2C1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) I2C2_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) I2C2_3_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) SPI1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) SPI2_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) SPI2_3_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) USART1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) USART2_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) USART2_LPUART2_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) USART3_4_LPUART1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) USART3_4_5_6_LPUART1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) CEC_IRQHandler(void);

extern unsigned int _estack;

typedef void (*const pHandler)(void);

// The vector table.
// Placed in .isr_vector by the linker script at the start of FLASH.

__attribute__((section(".isr_vector"), used)) pHandler g_pfnVectors[] = {
   // Cortex-M0+ core exceptions
   (pHandler)&_estack, // Initial stack pointer
   Reset_Handler,      // Reset handler

   NMI_Handler,       // -14  NMI
   HardFault_Handler, // -13  Hard fault
   0,                 // Reserved
   0,                 // Reserved
   0,                 // Reserved
   0,                 // Reserved
   0,                 // Reserved
   0,                 // Reserved
   0,                 // Reserved
   SVC_Handler,       // -5   SVCall
   0,                 // Reserved
   0,                 // Reserved
   PendSV_Handler,    // -2   PendSV
   SysTick_Handler,   // -1   SysTick

   // STM32G0 device-specific interrupts (IRQ 0–30)

#if defined(STM32G0B1xx) || defined(STM32G0C1xx)

   WWDG_IRQHandler,                              //  0  Window Watchdog
   PVD_VDDIO2_IRQHandler,                        //  1  PVD + VDDIO2 monitor
   RTC_TAMP_IRQHandler,                           //  2  RTC + Tamper
   FLASH_IRQHandler,                              //  3  Flash
   RCC_CRS_IRQHandler,                            //  4  RCC + CRS
   EXTI0_1_IRQHandler,                            //  5  EXTI 0-1
   EXTI2_3_IRQHandler,                            //  6  EXTI 2-3
   EXTI4_15_IRQHandler,                           //  7  EXTI 4-15
   USB_UCPD1_2_IRQHandler,                        //  8  USB + UCPD1/2
   DMA1_Channel1_IRQHandler,                      //  9  DMA1 Ch1
   DMA1_Channel2_3_IRQHandler,                    // 10  DMA1 Ch2-3
   DMA1_Ch4_7_DMA2_Ch1_5_DMAMUX1_OVR_IRQHandler, // 11  DMA1 Ch4-7, DMA2 Ch1-5, DMAMUX
   ADC1_COMP_IRQHandler,                          // 12  ADC1 + COMP
   TIM1_BRK_UP_TRG_COM_IRQHandler,               // 13  TIM1 break/update/trigger/commutation
   TIM1_CC_IRQHandler,                            // 14  TIM1 capture compare
   TIM2_IRQHandler,                               // 15  TIM2
   TIM3_TIM4_IRQHandler,                          // 16  TIM3 + TIM4
   TIM6_DAC_LPTIM1_IRQHandler,                    // 17  TIM6 + DAC + LPTIM1
   TIM7_LPTIM2_IRQHandler,                        // 18  TIM7 + LPTIM2
   TIM14_IRQHandler,                              // 19  TIM14
   TIM15_IRQHandler,                              // 20  TIM15
   TIM16_FDCAN_IT0_IRQHandler,                    // 21  TIM16 + FDCAN IT0
   TIM17_FDCAN_IT1_IRQHandler,                    // 22  TIM17 + FDCAN IT1
   I2C1_IRQHandler,                               // 23  I2C1
   I2C2_3_IRQHandler,                             // 24  I2C2 + I2C3
   SPI1_IRQHandler,                               // 25  SPI1
   SPI2_3_IRQHandler,                             // 26  SPI2 + SPI3
   USART1_IRQHandler,                             // 27  USART1
   USART2_LPUART2_IRQHandler,                     // 28  USART2 + LPUART2
   USART3_4_5_6_LPUART1_IRQHandler,              // 29  USART3-6 + LPUART1
   CEC_IRQHandler,                                // 30  CEC

#elif defined(STM32G071xx) || defined(STM32G081xx) || defined(STM32G070xx)

   WWDG_IRQHandler,                    //  0  Window Watchdog
   PVD_IRQHandler,                     //  1  PVD
   RTC_TAMP_IRQHandler,                //  2  RTC + Tamper
   FLASH_IRQHandler,                   //  3  Flash
   RCC_IRQHandler,                     //  4  RCC
   EXTI0_1_IRQHandler,                //  5  EXTI 0-1
   EXTI2_3_IRQHandler,                //  6  EXTI 2-3
   EXTI4_15_IRQHandler,               //  7  EXTI 4-15
   UCPD1_2_IRQHandler,                //  8  UCPD1/2
   DMA1_Channel1_IRQHandler,          //  9  DMA1 Ch1
   DMA1_Channel2_3_IRQHandler,        // 10  DMA1 Ch2-3
   DMA1_Ch4_7_DMAMUX1_OVR_IRQHandler, // 11  DMA1 Ch4-7 + DMAMUX
   ADC1_COMP_IRQHandler,              // 12  ADC1 + COMP
   TIM1_BRK_UP_TRG_COM_IRQHandler,   // 13  TIM1 break/update/trigger/commutation
   TIM1_CC_IRQHandler,                // 14  TIM1 capture compare
   TIM2_IRQHandler,                   // 15  TIM2
   TIM3_IRQHandler,                   // 16  TIM3
   TIM6_DAC_LPTIM1_IRQHandler,       // 17  TIM6 + DAC + LPTIM1
   TIM7_LPTIM2_IRQHandler,           // 18  TIM7 + LPTIM2
   TIM14_IRQHandler,                  // 19  TIM14
   TIM15_IRQHandler,                  // 20  TIM15
   TIM16_IRQHandler,                  // 21  TIM16
   TIM17_IRQHandler,                  // 22  TIM17
   I2C1_IRQHandler,                   // 23  I2C1
   I2C2_IRQHandler,                   // 24  I2C2
   SPI1_IRQHandler,                   // 25  SPI1
   SPI2_IRQHandler,                   // 26  SPI2
   USART1_IRQHandler,                 // 27  USART1
   USART2_IRQHandler,                 // 28  USART2
   USART3_4_LPUART1_IRQHandler,      // 29  USART3-4 + LPUART1
   CEC_IRQHandler,                    // 30  CEC

#else
#error "Unsupported STM32G0 device — add vector table entries"
#endif
};

void __attribute__((section(".after_vectors"))) Default_Handler(void) {
#if defined(DEBUG)
   __DEBUG_BKPT();
#endif
   while (1) {
   }
}

#pragma GCC diagnostic pop

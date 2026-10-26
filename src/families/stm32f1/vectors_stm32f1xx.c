// STM32F1 series interrupt vector table (Cortex-M3).
// Medium-density devices (STM32F103x8/xB): 43 device IRQs, positions from
// RM0008 Table 63 and ST's startup_stm32f103xb.s. Weak aliases allow
// application code to override individual handlers.

#include "cmsis-plus/cortexm/exception-handlers.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

void __attribute__((weak)) Default_Handler(void);

// Forward declarations — weak aliases to Default_Handler.
void __attribute__((weak, alias("Default_Handler"))) WWDG_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) PVD_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TAMPER_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) RTC_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) FLASH_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) RCC_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) EXTI0_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) EXTI1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) EXTI2_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) EXTI3_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) EXTI4_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) DMA1_Channel1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) DMA1_Channel2_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) DMA1_Channel3_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) DMA1_Channel4_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) DMA1_Channel5_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) DMA1_Channel6_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) DMA1_Channel7_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) ADC1_2_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) USB_HP_CAN1_TX_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) USB_LP_CAN1_RX0_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) CAN1_RX1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) CAN1_SCE_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) EXTI9_5_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM1_BRK_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM1_UP_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM1_TRG_COM_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM1_CC_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM2_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM3_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM4_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) I2C1_EV_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) I2C1_ER_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) I2C2_EV_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) I2C2_ER_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) SPI1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) SPI2_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) USART1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) USART2_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) USART3_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) EXTI15_10_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) RTC_Alarm_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) USBWakeUp_IRQHandler(void);

extern unsigned int _estack;

typedef void (*const pHandler)(void);

// The vector table.
// Placed in .isr_vector by the linker script at the start of FLASH.

__attribute__((section(".isr_vector"), used)) pHandler g_pfnVectors[] = {
   // Cortex-M3 core exceptions
   (pHandler)&_estack, // Initial stack pointer
   Reset_Handler,      // Reset handler

   NMI_Handler,        // -14  NMI
   HardFault_Handler,  // -13  Hard fault
   MemManage_Handler,  // -12  MPU fault
   BusFault_Handler,   // -11  Bus fault
   UsageFault_Handler, // -10  Usage fault
   0,                  // -9   Reserved
   0,                  // -8   Reserved
   0,                  // -7   Reserved
   0,                  // -6   Reserved
   SVC_Handler,        // -5   SVCall
   DebugMon_Handler,   // -4   Debug monitor
   0,                  // -3   Reserved
   PendSV_Handler,     // -2   PendSV
   SysTick_Handler,    // -1   SysTick

// STM32F1 medium-density device-specific interrupts (IRQ 0–42)

#if defined(STM32F103xB)

   WWDG_IRQHandler,            //  0  Window Watchdog
   PVD_IRQHandler,             //  1  PVD through EXTI line 16
   TAMPER_IRQHandler,          //  2  Tamper
   RTC_IRQHandler,             //  3  RTC global
   FLASH_IRQHandler,           //  4  Flash
   RCC_IRQHandler,             //  5  RCC
   EXTI0_IRQHandler,           //  6  EXTI line 0
   EXTI1_IRQHandler,           //  7  EXTI line 1
   EXTI2_IRQHandler,           //  8  EXTI line 2
   EXTI3_IRQHandler,           //  9  EXTI line 3
   EXTI4_IRQHandler,           // 10  EXTI line 4
   DMA1_Channel1_IRQHandler,   // 11  DMA1 channel 1
   DMA1_Channel2_IRQHandler,   // 12  DMA1 channel 2
   DMA1_Channel3_IRQHandler,   // 13  DMA1 channel 3
   DMA1_Channel4_IRQHandler,   // 14  DMA1 channel 4
   DMA1_Channel5_IRQHandler,   // 15  DMA1 channel 5
   DMA1_Channel6_IRQHandler,   // 16  DMA1 channel 6
   DMA1_Channel7_IRQHandler,   // 17  DMA1 channel 7
   ADC1_2_IRQHandler,          // 18  ADC1 + ADC2
   USB_HP_CAN1_TX_IRQHandler,  // 19  USB high priority + CAN TX
   USB_LP_CAN1_RX0_IRQHandler, // 20  USB low priority + CAN RX0
   CAN1_RX1_IRQHandler,        // 21  CAN RX1
   CAN1_SCE_IRQHandler,        // 22  CAN SCE
   EXTI9_5_IRQHandler,         // 23  EXTI lines 5-9
   TIM1_BRK_IRQHandler,        // 24  TIM1 break
   TIM1_UP_IRQHandler,         // 25  TIM1 update
   TIM1_TRG_COM_IRQHandler,    // 26  TIM1 trigger + commutation
   TIM1_CC_IRQHandler,         // 27  TIM1 capture compare
   TIM2_IRQHandler,            // 28  TIM2
   TIM3_IRQHandler,            // 29  TIM3
   TIM4_IRQHandler,            // 30  TIM4
   I2C1_EV_IRQHandler,         // 31  I2C1 event
   I2C1_ER_IRQHandler,         // 32  I2C1 error
   I2C2_EV_IRQHandler,         // 33  I2C2 event
   I2C2_ER_IRQHandler,         // 34  I2C2 error
   SPI1_IRQHandler,            // 35  SPI1
   SPI2_IRQHandler,            // 36  SPI2
   USART1_IRQHandler,          // 37  USART1
   USART2_IRQHandler,          // 38  USART2
   USART3_IRQHandler,          // 39  USART3
   EXTI15_10_IRQHandler,       // 40  EXTI lines 10-15
   RTC_Alarm_IRQHandler,       // 41  RTC alarm through EXTI line 17
   USBWakeUp_IRQHandler,       // 42  USB wakeup from suspend through EXTI line 18

#else
#error "Unsupported STM32F1 device — add vector table entries"
#endif
};

void __attribute__((section(".after_vectors"))) Default_Handler(void) {
#if defined(DEBUG)
   __DEBUG_BKPT();
#endif
   while(1) {
   }
}

#pragma GCC diagnostic pop

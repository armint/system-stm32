// STM32WL series interrupt vector table (Cortex-M4 application core).
// Weak aliases allow application code to override individual handlers.
// IRQ positions from RM0461 Table 66 (STM32WLE5xx).

#include "cmsis-plus/cortexm/exception-handlers.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

void __attribute__((weak)) Default_Handler(void);

// Forward declarations — weak aliases to Default_Handler.
void __attribute__((weak, alias("Default_Handler"))) WWDG_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) PVD_PVM_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TAMP_STAMP_LSECSS_SSRU_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) RTC_WKUP_IRQHandler(void);
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
void __attribute__((weak, alias("Default_Handler"))) ADC_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) DAC_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) C2SEV_PWR_C2H_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) COMP1_COMP2_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) EXTI9_5_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM1_BRK_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM1_UP_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM1_TRG_COM_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM1_CC_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM2_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM16_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM17_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) I2C1_EV_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) I2C1_ER_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) I2C2_EV_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) I2C2_ER_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) SPI1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) SPI2_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) USART1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) USART2_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) LPUART1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) LPTIM1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) LPTIM2_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) EXTI15_10_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) RTC_Alarm_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) LPTIM3_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) SUBGHZSPI_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) IPCC_C1_RX_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) IPCC_C1_TX_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) HSEM_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) I2C3_EV_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) I2C3_ER_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) Radio_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) AES_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) RNG_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) PKA_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) DMA2_Channel1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) DMA2_Channel2_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) DMA2_Channel3_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) DMA2_Channel4_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) DMA2_Channel5_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) DMA2_Channel6_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) DMA2_Channel7_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) DMAMUX1_OVR_IRQHandler(void);

extern unsigned int _estack;

typedef void (*const pHandler)(void);

// The vector table.
// Placed in .isr_vector by the linker script at the start of FLASH.

__attribute__((section(".isr_vector"), used)) pHandler g_pfnVectors[] = {
    // Cortex-M4 core exceptions
    (pHandler)&_estack,    // Initial stack pointer
    Reset_Handler,         // Reset

    NMI_Handler,           // -14  NMI
    HardFault_Handler,     // -13  Hard fault
    MemManage_Handler,     // -12  MPU fault
    BusFault_Handler,      // -11  Bus fault
    UsageFault_Handler,    // -10  Usage fault
    0,                     // -9   Reserved
    0,                     // -8   Reserved
    0,                     // -7   Reserved
    0,                     // -6   Reserved
    SVC_Handler,           // -5   SVCall
    DebugMon_Handler,      // -4   Debug monitor
    0,                     // -3   Reserved
    PendSV_Handler,        // -2   PendSV
    SysTick_Handler,       // -1   SysTick

    // STM32WLE5xx device-specific interrupts (IRQ 0–61, RM0461 Table 66)
    WWDG_IRQHandler,                   //  0  Window Watchdog
    PVD_PVM_IRQHandler,                //  1  PVD/PVM monitor
    TAMP_STAMP_LSECSS_SSRU_IRQHandler, //  2  RTC tamper, timestamp, LSECSS, SSRU
    RTC_WKUP_IRQHandler,               //  3  RTC wakeup
    FLASH_IRQHandler,                  //  4  Flash
    RCC_IRQHandler,                    //  5  RCC
    EXTI0_IRQHandler,                  //  6  EXTI line 0
    EXTI1_IRQHandler,                  //  7  EXTI line 1
    EXTI2_IRQHandler,                  //  8  EXTI line 2
    EXTI3_IRQHandler,                  //  9  EXTI line 3
    EXTI4_IRQHandler,                  // 10  EXTI line 4
    DMA1_Channel1_IRQHandler,          // 11  DMA1 channel 1
    DMA1_Channel2_IRQHandler,          // 12  DMA1 channel 2
    DMA1_Channel3_IRQHandler,          // 13  DMA1 channel 3
    DMA1_Channel4_IRQHandler,          // 14  DMA1 channel 4
    DMA1_Channel5_IRQHandler,          // 15  DMA1 channel 5
    DMA1_Channel6_IRQHandler,          // 16  DMA1 channel 6
    DMA1_Channel7_IRQHandler,          // 17  DMA1 channel 7
    ADC_IRQHandler,                    // 18  ADC
    DAC_IRQHandler,                    // 19  DAC
    C2SEV_PWR_C2H_IRQHandler,          // 20  CPU2 SEV / PWR CPU2 wakeup
    COMP1_COMP2_IRQHandler,            // 21  COMP1 + COMP2
    EXTI9_5_IRQHandler,                // 22  EXTI lines 9-5
    TIM1_BRK_IRQHandler,               // 23  TIM1 break
    TIM1_UP_IRQHandler,                // 24  TIM1 update
    TIM1_TRG_COM_IRQHandler,           // 25  TIM1 trigger/commutation
    TIM1_CC_IRQHandler,                // 26  TIM1 capture compare
    TIM2_IRQHandler,                   // 27  TIM2
    TIM16_IRQHandler,                  // 28  TIM16
    TIM17_IRQHandler,                  // 29  TIM17
    I2C1_EV_IRQHandler,                // 30  I2C1 event
    I2C1_ER_IRQHandler,                // 31  I2C1 error
    I2C2_EV_IRQHandler,                // 32  I2C2 event
    I2C2_ER_IRQHandler,                // 33  I2C2 error
    SPI1_IRQHandler,                   // 34  SPI1
    SPI2_IRQHandler,                   // 35  SPI2
    USART1_IRQHandler,                 // 36  USART1
    USART2_IRQHandler,                 // 37  USART2
    LPUART1_IRQHandler,                // 38  LPUART1
    LPTIM1_IRQHandler,                 // 39  LPTIM1
    LPTIM2_IRQHandler,                 // 40  LPTIM2
    EXTI15_10_IRQHandler,              // 41  EXTI lines 15-10
    RTC_Alarm_IRQHandler,              // 42  RTC alarm (A and B)
    LPTIM3_IRQHandler,                 // 43  LPTIM3
    SUBGHZSPI_IRQHandler,              // 44  Sub-GHz radio SPI
    IPCC_C1_RX_IRQHandler,             // 45  IPCC CPU1 RX occupied
    IPCC_C1_TX_IRQHandler,             // 46  IPCC CPU1 TX free
    HSEM_IRQHandler,                   // 47  Hardware semaphore
    I2C3_EV_IRQHandler,                // 48  I2C3 event
    I2C3_ER_IRQHandler,                // 49  I2C3 error
    Radio_IRQHandler,                  // 50  Sub-GHz radio IRQ / busy
    AES_IRQHandler,                    // 51  AES
    RNG_IRQHandler,                    // 52  True RNG
    PKA_IRQHandler,                    // 53  PKA
    DMA2_Channel1_IRQHandler,          // 54  DMA2 channel 1
    DMA2_Channel2_IRQHandler,          // 55  DMA2 channel 2
    DMA2_Channel3_IRQHandler,          // 56  DMA2 channel 3
    DMA2_Channel4_IRQHandler,          // 57  DMA2 channel 4
    DMA2_Channel5_IRQHandler,          // 58  DMA2 channel 5
    DMA2_Channel6_IRQHandler,          // 59  DMA2 channel 6
    DMA2_Channel7_IRQHandler,          // 60  DMA2 channel 7
    DMAMUX1_OVR_IRQHandler,            // 61  DMAMUX1 overrun
};

void __attribute__((section(".after_vectors"))) Default_Handler(void) {
#if defined(DEBUG)
    __DEBUG_BKPT();
#endif
    while (1) {
    }
}

#pragma GCC diagnostic pop

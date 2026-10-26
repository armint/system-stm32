// STM32H5 series interrupt vector table (Cortex-M33, TrustZone off).
// Weak aliases allow application code to override individual handlers.
// IRQ positions from the STM32H533xx CMSIS startup file (cmsis-device-h5
// v1.7.0, RM0481). The STM32H523xx has the same IRQ positions, only without
// the crypto IRQs (SAES, OTFDEC1, AES, PKA), so this table also fits it.
// Other H5 parts (H503, H56x, H57x) have a different table.

#include "cmsis-plus/cortexm/exception-handlers.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

void __attribute__((weak)) Default_Handler(void);

// Only used with TrustZone on. It stays a weak alias, as in the ST table.
void __attribute__((weak, alias("Default_Handler"))) SecureFault_Handler(void);

// Forward declarations — weak aliases to Default_Handler.
void __attribute__((weak, alias("Default_Handler"))) WWDG_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) PVD_AVD_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) RTC_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) RTC_S_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TAMP_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) RAMCFG_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) FLASH_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) FLASH_S_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) GTZC_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) RCC_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) RCC_S_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) EXTI0_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) EXTI1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) EXTI2_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) EXTI3_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) EXTI4_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) EXTI5_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) EXTI6_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) EXTI7_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) EXTI8_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) EXTI9_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) EXTI10_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) EXTI11_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) EXTI12_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) EXTI13_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) EXTI14_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) EXTI15_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) GPDMA1_Channel0_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) GPDMA1_Channel1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) GPDMA1_Channel2_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) GPDMA1_Channel3_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) GPDMA1_Channel4_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) GPDMA1_Channel5_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) GPDMA1_Channel6_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) GPDMA1_Channel7_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) IWDG_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) SAES_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) ADC1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) DAC1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) FDCAN1_IT0_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) FDCAN1_IT1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM1_BRK_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM1_UP_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM1_TRG_COM_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM1_CC_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM2_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM3_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM4_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM5_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM6_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM7_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) I2C1_EV_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) I2C1_ER_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) I2C2_EV_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) I2C2_ER_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) SPI1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) SPI2_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) SPI3_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) USART1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) USART2_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) USART3_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) UART4_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) UART5_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) LPUART1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) LPTIM1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM8_BRK_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM8_UP_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM8_TRG_COM_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM8_CC_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) ADC2_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) LPTIM2_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM15_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) USB_DRD_FS_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) CRS_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) UCPD1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) FMC_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) OCTOSPI1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) SDMMC1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) I2C3_EV_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) I2C3_ER_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) SPI4_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) USART6_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) GPDMA2_Channel0_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) GPDMA2_Channel1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) GPDMA2_Channel2_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) GPDMA2_Channel3_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) GPDMA2_Channel4_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) GPDMA2_Channel5_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) GPDMA2_Channel6_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) GPDMA2_Channel7_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) FPU_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) ICACHE_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) DCACHE1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) DCMI_PSSI_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) FDCAN2_IT0_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) FDCAN2_IT1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) DTS_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) RNG_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) OTFDEC1_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) AES_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) HASH_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) PKA_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) CEC_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) TIM12_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) I3C1_EV_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) I3C1_ER_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) I3C2_EV_IRQHandler(void);
void __attribute__((weak, alias("Default_Handler"))) I3C2_ER_IRQHandler(void);

extern unsigned int _estack;

typedef void (*const pHandler)(void);

// The vector table.
// Placed in .isr_vector by the linker script at the start of FLASH.

__attribute__((section(".isr_vector"), used)) pHandler g_pfnVectors[] = {
   // Cortex-M33 core exceptions
   (pHandler)&_estack, // Initial stack pointer
   Reset_Handler,      // Reset

   NMI_Handler,         // -14  NMI
   HardFault_Handler,   // -13  Hard fault
   MemManage_Handler,   // -12  MPU fault
   BusFault_Handler,    // -11  Bus fault
   UsageFault_Handler,  // -10  Usage fault
   SecureFault_Handler, // -9   Secure fault
   0,                   // -8   Reserved
   0,                   // -7   Reserved
   0,                   // -6   Reserved
   SVC_Handler,         // -5   SVCall
   DebugMon_Handler,    // -4   Debug monitor
   0,                   // -3   Reserved
   PendSV_Handler,      // -2   PendSV
   SysTick_Handler,     // -1   SysTick

// Device interrupts

#if defined(STM32H533xx) || defined(STM32H523xx)

   WWDG_IRQHandler,            // 0   WWDG
   PVD_AVD_IRQHandler,         // 1   PVD_AVD
   RTC_IRQHandler,             // 2   RTC
   RTC_S_IRQHandler,           // 3   RTC_S
   TAMP_IRQHandler,            // 4   TAMP
   RAMCFG_IRQHandler,          // 5   RAMCFG
   FLASH_IRQHandler,           // 6   FLASH
   FLASH_S_IRQHandler,         // 7   FLASH_S
   GTZC_IRQHandler,            // 8   GTZC
   RCC_IRQHandler,             // 9   RCC
   RCC_S_IRQHandler,           // 10  RCC_S
   EXTI0_IRQHandler,           // 11  EXTI0
   EXTI1_IRQHandler,           // 12  EXTI1
   EXTI2_IRQHandler,           // 13  EXTI2
   EXTI3_IRQHandler,           // 14  EXTI3
   EXTI4_IRQHandler,           // 15  EXTI4
   EXTI5_IRQHandler,           // 16  EXTI5
   EXTI6_IRQHandler,           // 17  EXTI6
   EXTI7_IRQHandler,           // 18  EXTI7
   EXTI8_IRQHandler,           // 19  EXTI8
   EXTI9_IRQHandler,           // 20  EXTI9
   EXTI10_IRQHandler,          // 21  EXTI10
   EXTI11_IRQHandler,          // 22  EXTI11
   EXTI12_IRQHandler,          // 23  EXTI12
   EXTI13_IRQHandler,          // 24  EXTI13
   EXTI14_IRQHandler,          // 25  EXTI14
   EXTI15_IRQHandler,          // 26  EXTI15
   GPDMA1_Channel0_IRQHandler, // 27  GPDMA1_Channel0
   GPDMA1_Channel1_IRQHandler, // 28  GPDMA1_Channel1
   GPDMA1_Channel2_IRQHandler, // 29  GPDMA1_Channel2
   GPDMA1_Channel3_IRQHandler, // 30  GPDMA1_Channel3
   GPDMA1_Channel4_IRQHandler, // 31  GPDMA1_Channel4
   GPDMA1_Channel5_IRQHandler, // 32  GPDMA1_Channel5
   GPDMA1_Channel6_IRQHandler, // 33  GPDMA1_Channel6
   GPDMA1_Channel7_IRQHandler, // 34  GPDMA1_Channel7
   IWDG_IRQHandler,            // 35  IWDG
   SAES_IRQHandler,            // 36  SAES
   ADC1_IRQHandler,            // 37  ADC1
   DAC1_IRQHandler,            // 38  DAC1
   FDCAN1_IT0_IRQHandler,      // 39  FDCAN1_IT0
   FDCAN1_IT1_IRQHandler,      // 40  FDCAN1_IT1
   TIM1_BRK_IRQHandler,        // 41  TIM1_BRK
   TIM1_UP_IRQHandler,         // 42  TIM1_UP
   TIM1_TRG_COM_IRQHandler,    // 43  TIM1_TRG_COM
   TIM1_CC_IRQHandler,         // 44  TIM1_CC
   TIM2_IRQHandler,            // 45  TIM2
   TIM3_IRQHandler,            // 46  TIM3
   TIM4_IRQHandler,            // 47  TIM4
   TIM5_IRQHandler,            // 48  TIM5
   TIM6_IRQHandler,            // 49  TIM6
   TIM7_IRQHandler,            // 50  TIM7
   I2C1_EV_IRQHandler,         // 51  I2C1_EV
   I2C1_ER_IRQHandler,         // 52  I2C1_ER
   I2C2_EV_IRQHandler,         // 53  I2C2_EV
   I2C2_ER_IRQHandler,         // 54  I2C2_ER
   SPI1_IRQHandler,            // 55  SPI1
   SPI2_IRQHandler,            // 56  SPI2
   SPI3_IRQHandler,            // 57  SPI3
   USART1_IRQHandler,          // 58  USART1
   USART2_IRQHandler,          // 59  USART2
   USART3_IRQHandler,          // 60  USART3
   UART4_IRQHandler,           // 61  UART4
   UART5_IRQHandler,           // 62  UART5
   LPUART1_IRQHandler,         // 63  LPUART1
   LPTIM1_IRQHandler,          // 64  LPTIM1
   TIM8_BRK_IRQHandler,        // 65  TIM8_BRK
   TIM8_UP_IRQHandler,         // 66  TIM8_UP
   TIM8_TRG_COM_IRQHandler,    // 67  TIM8_TRG_COM
   TIM8_CC_IRQHandler,         // 68  TIM8_CC
   ADC2_IRQHandler,            // 69  ADC2
   LPTIM2_IRQHandler,          // 70  LPTIM2
   TIM15_IRQHandler,           // 71  TIM15
   0,                          // 72  Reserved
   0,                          // 73  Reserved
   USB_DRD_FS_IRQHandler,      // 74  USB_DRD_FS
   CRS_IRQHandler,             // 75  CRS
   UCPD1_IRQHandler,           // 76  UCPD1
   FMC_IRQHandler,             // 77  FMC
   OCTOSPI1_IRQHandler,        // 78  OCTOSPI1
   SDMMC1_IRQHandler,          // 79  SDMMC1
   I2C3_EV_IRQHandler,         // 80  I2C3_EV
   I2C3_ER_IRQHandler,         // 81  I2C3_ER
   SPI4_IRQHandler,            // 82  SPI4
   0,                          // 83  Reserved
   0,                          // 84  Reserved
   USART6_IRQHandler,          // 85  USART6
   0,                          // 86  Reserved
   0,                          // 87  Reserved
   0,                          // 88  Reserved
   0,                          // 89  Reserved
   GPDMA2_Channel0_IRQHandler, // 90  GPDMA2_Channel0
   GPDMA2_Channel1_IRQHandler, // 91  GPDMA2_Channel1
   GPDMA2_Channel2_IRQHandler, // 92  GPDMA2_Channel2
   GPDMA2_Channel3_IRQHandler, // 93  GPDMA2_Channel3
   GPDMA2_Channel4_IRQHandler, // 94  GPDMA2_Channel4
   GPDMA2_Channel5_IRQHandler, // 95  GPDMA2_Channel5
   GPDMA2_Channel6_IRQHandler, // 96  GPDMA2_Channel6
   GPDMA2_Channel7_IRQHandler, // 97  GPDMA2_Channel7
   0,                          // 98  Reserved
   0,                          // 99  Reserved
   0,                          // 100 Reserved
   0,                          // 101 Reserved
   0,                          // 102 Reserved
   FPU_IRQHandler,             // 103 FPU
   ICACHE_IRQHandler,          // 104 ICACHE
   DCACHE1_IRQHandler,         // 105 DCACHE1
   0,                          // 106 Reserved
   0,                          // 107 Reserved
   DCMI_PSSI_IRQHandler,       // 108 DCMI_PSSI
   FDCAN2_IT0_IRQHandler,      // 109 FDCAN2_IT0
   FDCAN2_IT1_IRQHandler,      // 110 FDCAN2_IT1
   0,                          // 111 Reserved
   0,                          // 112 Reserved
   DTS_IRQHandler,             // 113 DTS
   RNG_IRQHandler,             // 114 RNG
   OTFDEC1_IRQHandler,         // 115 OTFDEC1
   AES_IRQHandler,             // 116 AES
   HASH_IRQHandler,            // 117 HASH
   PKA_IRQHandler,             // 118 PKA
   CEC_IRQHandler,             // 119 CEC
   TIM12_IRQHandler,           // 120 TIM12
   0,                          // 121 Reserved
   0,                          // 122 Reserved
   I3C1_EV_IRQHandler,         // 123 I3C1_EV
   I3C1_ER_IRQHandler,         // 124 I3C1_ER
   0,                          // 125 Reserved
   0,                          // 126 Reserved
   0,                          // 127 Reserved
   0,                          // 128 Reserved
   0,                          // 129 Reserved
   0,                          // 130 Reserved
   I3C2_EV_IRQHandler,         // 131 I3C2_EV
   I3C2_ER_IRQHandler,         // 132 I3C2_ER

#else
#error "Unsupported STM32H5 device — add vector table entries"
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

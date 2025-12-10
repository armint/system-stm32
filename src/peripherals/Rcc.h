#pragma once

#include <cmsis_device.h>

// This header provides convenience inline functions to enable/disable
// RCC clocks (and their Sleep-mode clocks) for peripherals that exist on
// the selected STM32 device. Each function is only compiled if the
// corresponding RCC bit macro is defined by the device header.

namespace Rcc {

// Generic helpers to define enable/disable functions for a given RCC bit
#define RCC_DEFINE_EN_DIS(REG, BIT, Name)                                                                              \
   inline void enable##Name##Clock() {                                                                                 \
      RCC->REG |= (BIT);                                                                                               \
   }                                                                                                                   \
   inline void disable##Name##Clock() {                                                                                \
      RCC->REG &= ~(BIT);                                                                                              \
   }

#define RCC_DEFINE_SLEEP_EN_DIS(REG, BIT, Name)                                                                        \
   inline void enable##Name##ClockInSleepMode() {                                                                      \
      RCC->REG |= (BIT);                                                                                               \
   }                                                                                                                   \
   inline void disable##Name##ClockInSleepMode() {                                                                     \
      RCC->REG &= ~(BIT);                                                                                              \
   }

// New helper to enable/disable both Run and Sleep clocks with a single pair
// of functions per peripheral, as discussed. When both the run and sleep
// bits are available, prefer this over the separate helpers above.
#define RCC_DEFINE_EN_BOTH(REG_EN, BIT_EN, REG_SMEN, BIT_SMEN, Name)                                                    \
   inline void enable##Name##Clock() {                                                                                  \
      RCC->REG_EN |= (BIT_EN);                                                                                          \
      RCC->REG_SMEN |= (BIT_SMEN);                                                                                      \
   }                                                                                                                    \
   inline void disable##Name##Clock() {                                                                                 \
      RCC->REG_EN &= ~(BIT_EN);                                                                                         \
      RCC->REG_SMEN &= ~(BIT_SMEN);                                                                                    \
   }

// Backward-compat: keep the Sleep-Mode-named helpers but route them to the
// unified functions when both bits exist. This preserves existing call sites
// while ensuring both bits are toggled together.
#define RCC_DEFINE_SLEEP_ALIAS(Name)                                                                                    \
   inline void enable##Name##ClockInSleepMode() { enable##Name##Clock(); }                                             \
   inline void disable##Name##ClockInSleepMode() { disable##Name##Clock(); }

// -----------------
// GPIO (multiple series compatibility: L0 uses IOPENR/IOPSMENR, others use AHBxENR/AHBSMENR)
// -----------------
#if defined(RCC_IOPENR_IOPAEN) && defined(RCC_IOPSMENR_IOPASMEN)
RCC_DEFINE_EN_BOTH(IOPENR, RCC_IOPENR_IOPAEN, IOPSMENR, RCC_IOPSMENR_IOPASMEN, GPIOA)
RCC_DEFINE_SLEEP_ALIAS(GPIOA)
#elif defined(RCC_IOPENR_IOPAEN)
RCC_DEFINE_EN_DIS(IOPENR, RCC_IOPENR_IOPAEN, GPIOA)
#endif
#if defined(RCC_IOPENR_IOPBEN) && defined(RCC_IOPSMENR_IOPBSMEN)
RCC_DEFINE_EN_BOTH(IOPENR, RCC_IOPENR_IOPBEN, IOPSMENR, RCC_IOPSMENR_IOPBSMEN, GPIOB)
RCC_DEFINE_SLEEP_ALIAS(GPIOB)
#elif defined(RCC_IOPENR_IOPBEN)
RCC_DEFINE_EN_DIS(IOPENR, RCC_IOPENR_IOPBEN, GPIOB)
#endif
#if defined(RCC_IOPENR_IOPCEN) && defined(RCC_IOPSMENR_IOPCSMEN)
RCC_DEFINE_EN_BOTH(IOPENR, RCC_IOPENR_IOPCEN, IOPSMENR, RCC_IOPSMENR_IOPCSMEN, GPIOC)
RCC_DEFINE_SLEEP_ALIAS(GPIOC)
#elif defined(RCC_IOPENR_IOPCEN)
RCC_DEFINE_EN_DIS(IOPENR, RCC_IOPENR_IOPCEN, GPIOC)
#endif
#if defined(RCC_IOPENR_IOPDEN) && defined(RCC_IOPSMENR_IOPDSMEN)
RCC_DEFINE_EN_BOTH(IOPENR, RCC_IOPENR_IOPDEN, IOPSMENR, RCC_IOPSMENR_IOPDSMEN, GPIOD)
RCC_DEFINE_SLEEP_ALIAS(GPIOD)
#elif defined(RCC_IOPENR_IOPDEN)
RCC_DEFINE_EN_DIS(IOPENR, RCC_IOPENR_IOPDEN, GPIOD)
#endif
#if defined(RCC_IOPENR_IOPEEN) && defined(RCC_IOPSMENR_IOPESMEN)
RCC_DEFINE_EN_BOTH(IOPENR, RCC_IOPENR_IOPEEN, IOPSMENR, RCC_IOPSMENR_IOPESMEN, GPIOE)
RCC_DEFINE_SLEEP_ALIAS(GPIOE)
#elif defined(RCC_IOPENR_IOPEEN)
RCC_DEFINE_EN_DIS(IOPENR, RCC_IOPENR_IOPEEN, GPIOE)
#endif
#if defined(RCC_IOPENR_IOPHEN) && defined(RCC_IOPSMENR_IOPHSMEN)
RCC_DEFINE_EN_BOTH(IOPENR, RCC_IOPENR_IOPHEN, IOPSMENR, RCC_IOPSMENR_IOPHSMEN, GPIOH)
RCC_DEFINE_SLEEP_ALIAS(GPIOH)
#elif defined(RCC_IOPENR_IOPHEN)
RCC_DEFINE_EN_DIS(IOPENR, RCC_IOPENR_IOPHEN, GPIOH)
#endif

// Alternative GPIO for other series (AHB1ENR/AHB2ENR etc.)
#if defined(RCC_AHB1ENR_GPIOAEN)
RCC_DEFINE_EN_DIS(AHB1ENR, RCC_AHB1ENR_GPIOAEN, GPIOA)
#endif
#if defined(RCC_AHB1ENR_GPIOBEN)
RCC_DEFINE_EN_DIS(AHB1ENR, RCC_AHB1ENR_GPIOBEN, GPIOB)
#endif
#if defined(RCC_AHB1ENR_GPIOCEN)
RCC_DEFINE_EN_DIS(AHB1ENR, RCC_AHB1ENR_GPIOCEN, GPIOC)
#endif
#if defined(RCC_AHB1ENR_GPIODEN)
RCC_DEFINE_EN_DIS(AHB1ENR, RCC_AHB1ENR_GPIODEN, GPIOD)
#endif
#if defined(RCC_AHB1ENR_GPIOEEN)
RCC_DEFINE_EN_DIS(AHB1ENR, RCC_AHB1ENR_GPIOEEN, GPIOE)
#endif
#if defined(RCC_AHB1ENR_GPIOFEN)
RCC_DEFINE_EN_DIS(AHB1ENR, RCC_AHB1ENR_GPIOFEN, GPIOF)
#endif
#if defined(RCC_AHB1ENR_GPIOGEN)
RCC_DEFINE_EN_DIS(AHB1ENR, RCC_AHB1ENR_GPIOGEN, GPIOG)
#endif
#if defined(RCC_AHB1ENR_GPIOHEN)
RCC_DEFINE_EN_DIS(AHB1ENR, RCC_AHB1ENR_GPIOHEN, GPIOH)
#endif

// DMA
#if defined(RCC_AHBENR_DMA1EN)
RCC_DEFINE_EN_DIS(AHBENR, RCC_AHBENR_DMA1EN, DMA1)
#endif
#if defined(RCC_AHB1ENR_DMA1EN)
RCC_DEFINE_EN_DIS(AHB1ENR, RCC_AHB1ENR_DMA1EN, DMA1)
#endif
#if defined(RCC_AHB1ENR_DMA2EN)
RCC_DEFINE_EN_DIS(AHB1ENR, RCC_AHB1ENR_DMA2EN, DMA2)
#endif

// SYSCFG
#if defined(RCC_APB2ENR_SYSCFGEN) && defined(RCC_APB2SMENR_SYSCFGSMEN)
RCC_DEFINE_EN_BOTH(APB2ENR, RCC_APB2ENR_SYSCFGEN, APB2SMENR, RCC_APB2SMENR_SYSCFGSMEN, SYSCFG)
RCC_DEFINE_SLEEP_ALIAS(SYSCFG)
#elif defined(RCC_APB2ENR_SYSCFGEN)
RCC_DEFINE_EN_DIS(APB2ENR, RCC_APB2ENR_SYSCFGEN, SYSCFG)
#elif defined(RCC_APB2SMENR_SYSCFGSMEN)
RCC_DEFINE_SLEEP_EN_DIS(APB2SMENR, RCC_APB2SMENR_SYSCFGSMEN, SYSCFG)
#endif

// PWR
#if defined(RCC_APB1ENR_PWREN) && defined(RCC_APB1SMENR_PWRSMEN)
RCC_DEFINE_EN_BOTH(APB1ENR, RCC_APB1ENR_PWREN, APB1SMENR, RCC_APB1SMENR_PWRSMEN, PWR)
RCC_DEFINE_SLEEP_ALIAS(PWR)
#elif defined(RCC_APB1ENR_PWREN)
RCC_DEFINE_EN_DIS(APB1ENR, RCC_APB1ENR_PWREN, PWR)
#elif defined(RCC_APB1SMENR_PWRSMEN)
RCC_DEFINE_SLEEP_EN_DIS(APB1SMENR, RCC_APB1SMENR_PWRSMEN, PWR)
#endif

// USART/UART
#if defined(RCC_APB2ENR_USART1EN) && defined(RCC_APB2SMENR_USART1SMEN)
RCC_DEFINE_EN_BOTH(APB2ENR, RCC_APB2ENR_USART1EN, APB2SMENR, RCC_APB2SMENR_USART1SMEN, USART1)
RCC_DEFINE_SLEEP_ALIAS(USART1)
#elif defined(RCC_APB2ENR_USART1EN)
RCC_DEFINE_EN_DIS(APB2ENR, RCC_APB2ENR_USART1EN, USART1)
#endif
#if defined(RCC_APB1ENR_USART2EN)
RCC_DEFINE_EN_DIS(APB1ENR, RCC_APB1ENR_USART2EN, USART2)
#endif
#if defined(RCC_APB1ENR_USART3EN)
RCC_DEFINE_EN_DIS(APB1ENR, RCC_APB1ENR_USART3EN, USART3)
#endif
#if defined(RCC_APB1ENR_USART4EN)
RCC_DEFINE_EN_DIS(APB1ENR, RCC_APB1ENR_USART4EN, USART4)
#endif
#if defined(RCC_APB1ENR_USART5EN)
RCC_DEFINE_EN_DIS(APB1ENR, RCC_APB1ENR_USART5EN, USART5)
#endif
#if defined(RCC_APB1ENR_UART4EN)
RCC_DEFINE_EN_DIS(APB1ENR, RCC_APB1ENR_UART4EN, UART4)
#endif
#if defined(RCC_APB1ENR_UART5EN)
RCC_DEFINE_EN_DIS(APB1ENR, RCC_APB1ENR_UART5EN, UART5)
#endif
#if defined(RCC_APB1ENR_LPUART1EN)
// Handled below together with SMEN bit (if available)
#endif

#if defined(RCC_APB1SMENR_USART2SMEN)
RCC_DEFINE_SLEEP_EN_DIS(APB1SMENR, RCC_APB1SMENR_USART2SMEN, USART2)
#endif
#if defined(RCC_APB1SMENR_USART3SMEN)
RCC_DEFINE_SLEEP_EN_DIS(APB1SMENR, RCC_APB1SMENR_USART3SMEN, USART3)
#endif
#if defined(RCC_APB1SMENR_USART4SMEN)
RCC_DEFINE_SLEEP_EN_DIS(APB1SMENR, RCC_APB1SMENR_USART4SMEN, USART4)
#endif
#if defined(RCC_APB1SMENR_USART5SMEN)
RCC_DEFINE_SLEEP_EN_DIS(APB1SMENR, RCC_APB1SMENR_USART5SMEN, USART5)
#endif
#if defined(RCC_APB1SMENR_LPUART1SMEN) && defined(RCC_APB1ENR_LPUART1EN)
RCC_DEFINE_EN_BOTH(APB1ENR, RCC_APB1ENR_LPUART1EN, APB1SMENR, RCC_APB1SMENR_LPUART1SMEN, LPUART1)
RCC_DEFINE_SLEEP_ALIAS(LPUART1)
#elif defined(RCC_APB1ENR_LPUART1EN)
RCC_DEFINE_EN_DIS(APB1ENR, RCC_APB1ENR_LPUART1EN, LPUART1)
#elif defined(RCC_APB1SMENR_LPUART1SMEN)
RCC_DEFINE_SLEEP_EN_DIS(APB1SMENR, RCC_APB1SMENR_LPUART1SMEN, LPUART1)
#endif

// I2C
#if defined(RCC_APB1ENR_I2C1EN) && defined(RCC_APB1SMENR_I2C1SMEN)
RCC_DEFINE_EN_BOTH(APB1ENR, RCC_APB1ENR_I2C1EN, APB1SMENR, RCC_APB1SMENR_I2C1SMEN, I2C1)
RCC_DEFINE_SLEEP_ALIAS(I2C1)
#elif defined(RCC_APB1ENR_I2C1EN)
RCC_DEFINE_EN_DIS(APB1ENR, RCC_APB1ENR_I2C1EN, I2C1)
#endif
#if defined(RCC_APB1ENR_I2C2EN) && defined(RCC_APB1SMENR_I2C2SMEN)
RCC_DEFINE_EN_BOTH(APB1ENR, RCC_APB1ENR_I2C2EN, APB1SMENR, RCC_APB1SMENR_I2C2SMEN, I2C2)
RCC_DEFINE_SLEEP_ALIAS(I2C2)
#elif defined(RCC_APB1ENR_I2C2EN)
RCC_DEFINE_EN_DIS(APB1ENR, RCC_APB1ENR_I2C2EN, I2C2)
#endif
#if defined(RCC_APB1ENR_I2C3EN) && defined(RCC_APB1SMENR_I2C3SMEN)
RCC_DEFINE_EN_BOTH(APB1ENR, RCC_APB1ENR_I2C3EN, APB1SMENR, RCC_APB1SMENR_I2C3SMEN, I2C3)
RCC_DEFINE_SLEEP_ALIAS(I2C3)
#elif defined(RCC_APB1ENR_I2C3EN)
RCC_DEFINE_EN_DIS(APB1ENR, RCC_APB1ENR_I2C3EN, I2C3)
#endif

// SPI
#if defined(RCC_APB2ENR_SPI1EN) && defined(RCC_APB2SMENR_SPI1SMEN)
RCC_DEFINE_EN_BOTH(APB2ENR, RCC_APB2ENR_SPI1EN, APB2SMENR, RCC_APB2SMENR_SPI1SMEN, SPI1)
RCC_DEFINE_SLEEP_ALIAS(SPI1)
#elif defined(RCC_APB2ENR_SPI1EN)
RCC_DEFINE_EN_DIS(APB2ENR, RCC_APB2ENR_SPI1EN, SPI1)
#endif
#if defined(RCC_APB1ENR_SPI2EN) && defined(RCC_APB1SMENR_SPI2SMEN)
RCC_DEFINE_EN_BOTH(APB1ENR, RCC_APB1ENR_SPI2EN, APB1SMENR, RCC_APB1SMENR_SPI2SMEN, SPI2)
RCC_DEFINE_SLEEP_ALIAS(SPI2)
#elif defined(RCC_APB1ENR_SPI2EN)
RCC_DEFINE_EN_DIS(APB1ENR, RCC_APB1ENR_SPI2EN, SPI2)
#endif

// ADC
#if defined(RCC_APB2ENR_ADC1EN) && defined(RCC_APB2SMENR_ADC1SMEN)
RCC_DEFINE_EN_BOTH(APB2ENR, RCC_APB2ENR_ADC1EN, APB2SMENR, RCC_APB2SMENR_ADC1SMEN, ADC1)
RCC_DEFINE_SLEEP_ALIAS(ADC1)
#elif defined(RCC_APB2ENR_ADC1EN)
RCC_DEFINE_EN_DIS(APB2ENR, RCC_APB2ENR_ADC1EN, ADC1)
#elif defined(RCC_APB2SMENR_ADC1SMEN)
RCC_DEFINE_SLEEP_EN_DIS(APB2SMENR, RCC_APB2SMENR_ADC1SMEN, ADC1)
#endif

// Timers (selection varies by series)
#if defined(RCC_APB1ENR_TIM2EN) && defined(RCC_APB1SMENR_TIM2SMEN)
RCC_DEFINE_EN_BOTH(APB1ENR, RCC_APB1ENR_TIM2EN, APB1SMENR, RCC_APB1SMENR_TIM2SMEN, TIM2)
RCC_DEFINE_SLEEP_ALIAS(TIM2)
#elif defined(RCC_APB1ENR_TIM2EN)
RCC_DEFINE_EN_DIS(APB1ENR, RCC_APB1ENR_TIM2EN, TIM2)
#endif
#if defined(RCC_APB1ENR_TIM3EN) && defined(RCC_APB1SMENR_TIM3SMEN)
RCC_DEFINE_EN_BOTH(APB1ENR, RCC_APB1ENR_TIM3EN, APB1SMENR, RCC_APB1SMENR_TIM3SMEN, TIM3)
RCC_DEFINE_SLEEP_ALIAS(TIM3)
#elif defined(RCC_APB1ENR_TIM3EN)
RCC_DEFINE_EN_DIS(APB1ENR, RCC_APB1ENR_TIM3EN, TIM3)
#endif
#if defined(RCC_APB1ENR_TIM6EN)
RCC_DEFINE_EN_DIS(APB1ENR, RCC_APB1ENR_TIM6EN, TIM6)
#endif
#if defined(RCC_APB1ENR_TIM7EN)
RCC_DEFINE_EN_DIS(APB1ENR, RCC_APB1ENR_TIM7EN, TIM7)
#endif
#if defined(RCC_APB2ENR_TIM21EN) && defined(RCC_APB2SMENR_TIM21SMEN)
RCC_DEFINE_EN_BOTH(APB2ENR, RCC_APB2ENR_TIM21EN, APB2SMENR, RCC_APB2SMENR_TIM21SMEN, TIM21)
RCC_DEFINE_SLEEP_ALIAS(TIM21)
#elif defined(RCC_APB2ENR_TIM21EN)
RCC_DEFINE_EN_DIS(APB2ENR, RCC_APB2ENR_TIM21EN, TIM21)
#endif
#if defined(RCC_APB2ENR_TIM22EN) && defined(RCC_APB2SMENR_TIM22SMEN)
RCC_DEFINE_EN_BOTH(APB2ENR, RCC_APB2ENR_TIM22EN, APB2SMENR, RCC_APB2SMENR_TIM22SMEN, TIM22)
RCC_DEFINE_SLEEP_ALIAS(TIM22)
#elif defined(RCC_APB2ENR_TIM22EN)
RCC_DEFINE_EN_DIS(APB2ENR, RCC_APB2ENR_TIM22EN, TIM22)
#endif

#if defined(RCC_APB1SMENR_TIM2SMEN) && !defined(RCC_APB1ENR_TIM2EN)
RCC_DEFINE_SLEEP_EN_DIS(APB1SMENR, RCC_APB1SMENR_TIM2SMEN, TIM2)
#endif
#if defined(RCC_APB1SMENR_TIM3SMEN) && !defined(RCC_APB1ENR_TIM3EN)
RCC_DEFINE_SLEEP_EN_DIS(APB1SMENR, RCC_APB1SMENR_TIM3SMEN, TIM3)
#endif
#if defined(RCC_APB1SMENR_TIM6SMEN) && !defined(RCC_APB1ENR_TIM6EN)
RCC_DEFINE_SLEEP_EN_DIS(APB1SMENR, RCC_APB1SMENR_TIM6SMEN, TIM6)
#endif
#if defined(RCC_APB1SMENR_TIM7SMEN) && !defined(RCC_APB1ENR_TIM7EN)
RCC_DEFINE_SLEEP_EN_DIS(APB1SMENR, RCC_APB1SMENR_TIM7SMEN, TIM7)
#endif
#if defined(RCC_APB2SMENR_TIM21SMEN) && !defined(RCC_APB2ENR_TIM21EN)
RCC_DEFINE_SLEEP_EN_DIS(APB2SMENR, RCC_APB2SMENR_TIM21SMEN, TIM21)
#endif
#if defined(RCC_APB2SMENR_TIM22SMEN) && !defined(RCC_APB2ENR_TIM22EN)
RCC_DEFINE_SLEEP_EN_DIS(APB2SMENR, RCC_APB2SMENR_TIM22SMEN, TIM22)
#endif

// LPTIM
#if defined(RCC_APB1ENR_LPTIM1EN) && defined(RCC_APB1SMENR_LPTIM1SMEN)
RCC_DEFINE_EN_BOTH(APB1ENR, RCC_APB1ENR_LPTIM1EN, APB1SMENR, RCC_APB1SMENR_LPTIM1SMEN, LPTIM1)
RCC_DEFINE_SLEEP_ALIAS(LPTIM1)
#elif defined(RCC_APB1ENR_LPTIM1EN)
RCC_DEFINE_EN_DIS(APB1ENR, RCC_APB1ENR_LPTIM1EN, LPTIM1)
#elif defined(RCC_APB1SMENR_LPTIM1SMEN)
RCC_DEFINE_SLEEP_EN_DIS(APB1SMENR, RCC_APB1SMENR_LPTIM1SMEN, LPTIM1)
#endif

// RNG and LCD and USB (present on some L0 variants)
#if defined(RCC_APB2ENR_RNGEN)
RCC_DEFINE_EN_DIS(APB2ENR, RCC_APB2ENR_RNGEN, RNG)
#endif
#if defined(RCC_APB1ENR_RTCAPBEN)
RCC_DEFINE_EN_DIS(APB1ENR, RCC_APB1ENR_RTCAPBEN, RTCAPB)
#endif
#if defined(RCC_APB1ENR_LCDEN) && defined(RCC_APB1SMENR_LCDSMEN)
RCC_DEFINE_EN_BOTH(APB1ENR, RCC_APB1ENR_LCDEN, APB1SMENR, RCC_APB1SMENR_LCDSMEN, LCD)
RCC_DEFINE_SLEEP_ALIAS(LCD)
#elif defined(RCC_APB1ENR_LCDEN)
RCC_DEFINE_EN_DIS(APB1ENR, RCC_APB1ENR_LCDEN, LCD)
#elif defined(RCC_APB1SMENR_LCDSMEN)
RCC_DEFINE_SLEEP_EN_DIS(APB1SMENR, RCC_APB1SMENR_LCDSMEN, LCD)
#endif
#if defined(RCC_APB1ENR_USBEN) && defined(RCC_APB1SMENR_USBSMEN)
RCC_DEFINE_EN_BOTH(APB1ENR, RCC_APB1ENR_USBEN, APB1SMENR, RCC_APB1SMENR_USBSMEN, USB)
RCC_DEFINE_SLEEP_ALIAS(USB)
#elif defined(RCC_APB1ENR_USBEN)
RCC_DEFINE_EN_DIS(APB1ENR, RCC_APB1ENR_USBEN, USB)
#elif defined(RCC_APB1SMENR_USBSMEN)
RCC_DEFINE_SLEEP_EN_DIS(APB1SMENR, RCC_APB1SMENR_USBSMEN, USB)
#endif

// CRS (Clock Recovery System)
#if defined(RCC_APB1ENR_CRSEN) && defined(RCC_APB1SMENR_CRSSMEN)
RCC_DEFINE_EN_BOTH(APB1ENR, RCC_APB1ENR_CRSEN, APB1SMENR, RCC_APB1SMENR_CRSSMEN, CRS)
RCC_DEFINE_SLEEP_ALIAS(CRS)
#elif defined(RCC_APB1ENR_CRSEN)
RCC_DEFINE_EN_DIS(APB1ENR, RCC_APB1ENR_CRSEN, CRS)
#elif defined(RCC_APB1SMENR_CRSSMEN)
RCC_DEFINE_SLEEP_EN_DIS(APB1SMENR, RCC_APB1SMENR_CRSSMEN, CRS)
#endif

// TSC (Touch sensing controller)
#if defined(RCC_AHBENR_TSCEN) && defined(RCC_AHBSMENR_TSCSMEN)
RCC_DEFINE_EN_BOTH(AHBENR, RCC_AHBENR_TSCEN, AHBSMENR, RCC_AHBSMENR_TSCSMEN, TSC)
RCC_DEFINE_SLEEP_ALIAS(TSC)
#elif defined(RCC_AHBENR_TSCEN)
RCC_DEFINE_EN_DIS(AHBENR, RCC_AHBENR_TSCEN, TSC)
#elif defined(RCC_AHBSMENR_TSCSMEN)
RCC_DEFINE_SLEEP_EN_DIS(AHBSMENR, RCC_AHBSMENR_TSCSMEN, TSC)
#endif

// SPI/I2C/USART/UART sleep-mode variants already covered above.

// -----------------
// System clock helpers (guarded for STM32 series that support these bits)
// -----------------
// Configure system clock to 32 MHz using HSI via PLL (typically STM32L0 series).
// - Enables HSI
// - Sets FLASH latency to 1 wait state
// - Configures PLL: source HSI16, MUL x4, DIV /2 => 16 MHz * 4 / 2 = 32 MHz
// - Sets AHB/APB1/APB2 prescalers to DIV1
// - Switches SYSCLK to PLL and waits until ready
#if defined(RCC_CR_HSION) && defined(RCC_CR_HSIRDY) && defined(RCC_CR_PLLON) && defined(RCC_CR_PLLRDY) &&              \
defined(RCC_CFGR_SW) && defined(RCC_CFGR_SW_PLL) && defined(RCC_CFGR_SWS) && defined(RCC_CFGR_SWS_PLL) &&              \
defined(RCC_CFGR_PLLMUL4) && (defined(RCC_CFGR_PLLDIV2) || defined(RCC_CFGR_PLLDIV)) && defined(RCC_CFGR_HPRE_DIV1) && \
defined(RCC_CFGR_PPRE1_DIV1) && defined(RCC_CFGR_PPRE2_DIV1) && defined(FLASH_ACR_LATENCY)
inline void setClock32MhzHSI() {
   // Enable HSI and wait ready
   RCC->CR |= RCC_CR_HSION;
   while((RCC->CR & RCC_CR_HSIRDY) == 0) {
   }

   // One wait state for 32 MHz (voltage scaling must allow 32 MHz)
   FLASH->ACR |= FLASH_ACR_LATENCY;

   // Prescalers: AHB=1, APB1=1, APB2=1
   RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2)) |
               (RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV1 | RCC_CFGR_PPRE2_DIV1);

   // Disable PLL if running
   if(RCC->CR & RCC_CR_PLLON) {
      RCC->CR &= ~RCC_CR_PLLON;
      while(RCC->CR & RCC_CR_PLLRDY) {
      }
   }

   // Configure PLL: source = HSI (bit value 0 on L0), MUL x4, DIV /2
   // Clear PLLSRC/PLLMUL/PLLDIV bits first
   RCC->CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLMUL
#if defined(RCC_CFGR_PLLDIV)
                  | RCC_CFGR_PLLDIV
#endif
   );

   // Set multiplier to x4
   RCC->CFGR |= RCC_CFGR_PLLMUL4;

   // Set divider to /2
#if defined(RCC_CFGR_PLLDIV2)
   RCC->CFGR |= RCC_CFGR_PLLDIV2;
#elif defined(RCC_CFGR_PLLDIV)
   // For devices with PLLDIV field: 01b = /2
   RCC->CFGR |= (0x1UL << 22);
#endif

   // Enable PLL and wait ready
   RCC->CR |= RCC_CR_PLLON;
   while((RCC->CR & RCC_CR_PLLRDY) == 0) {
   }

   // Switch system clock to PLL
   RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
   while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {
   }
}
#endif

// Clean up helper macros from public namespace
#undef RCC_DEFINE_EN_DIS
#undef RCC_DEFINE_SLEEP_EN_DIS

} // namespace Rcc

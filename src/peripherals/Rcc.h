#pragma once

#include "Spi.h"
#include <cmsis_device.h>
#include <cstdint>
#include <delaybusywait.h>
#include <optional>
#include <span>
#include <type_traits>

namespace Rcc {

// TODO this needs to move to device specific headers
#if !defined(HSE_VALUE)
#define HSE_VALUE (48000000UL) /*!< Value of the External oscillator in Hz */
#endif                         /* HSE_VALUE */

#if !defined(HSI_VALUE)
#define HSI_VALUE (48000000UL) /*!< Value of the Internal oscillator in Hz*/
#endif                         /* HSI_VALUE */

#if !defined(LSI_VALUE)
#define LSI_VALUE (32000UL) /*!< Value of LSI in Hz*/
#endif                      /* LSI_VALUE */

#if !defined(LSE_VALUE)
#define LSE_VALUE (32768UL) /*!< Value of LSE in Hz*/
#endif                      /* LSE_VALUE */

enum class SystemClockSource : uint32_t {
#if defined(RCC_CFGR_SWS_MSI)
   MSI = RCC_CFGR_SWS_MSI,
#endif
#if defined(RCC_CFGR_SWS_HSISYS)
   // STM32G0: SWS field uses named HSISYS alias
   HSI = RCC_CFGR_SWS_HSISYS,
   HSE = RCC_CFGR_SWS_HSE,
#elif defined(RCC_CFGR_SWS_HSI)
   // STM32L0 and similar: named HSI/HSE/PLL aliases
   HSI = RCC_CFGR_SWS_HSI,
   HSE = RCC_CFGR_SWS_HSE,
#  if defined(RCC_CFGR_SWS_PLL)
   PLL = RCC_CFGR_SWS_PLL,
#  endif
#elif defined(RCC_CFGR_SWS_0)
   // STM32WL: only bit-position defines; SWS encoding: 00=MSI, 01=HSI16, 10=HSE, 11=PLL
   HSI = RCC_CFGR_SWS_0,
   HSE = RCC_CFGR_SWS_1,
   PLL = RCC_CFGR_SWS_0 | RCC_CFGR_SWS_1,
#endif
#if defined(RCC_CFGR_SWS_LSI)
   LSI = RCC_CFGR_SWS_LSI,
#endif
#if defined(RCC_CFGR_SWS_LSE)
   LSE = RCC_CFGR_SWS_LSE
#endif
};
enum class AHBPrescaler : uint32_t {
   div1 = 0,
   div2 = RCC_CFGR_HPRE_3,
   div4 = RCC_CFGR_HPRE_3 | RCC_CFGR_HPRE_0,
   div8 = RCC_CFGR_HPRE_3 | RCC_CFGR_HPRE_1,
   div16 = RCC_CFGR_HPRE_3 | RCC_CFGR_HPRE_1 | RCC_CFGR_HPRE_0,
   div64 = RCC_CFGR_HPRE_3 | RCC_CFGR_HPRE_2 | RCC_CFGR_HPRE_0,
   div128 = RCC_CFGR_HPRE_3 | RCC_CFGR_HPRE_2 | RCC_CFGR_HPRE_0,
   div256 = RCC_CFGR_HPRE_3 | RCC_CFGR_HPRE_2 | RCC_CFGR_HPRE_1,
   div512 = RCC_CFGR_HPRE_3 | RCC_CFGR_HPRE_2 | RCC_CFGR_HPRE_1 | RCC_CFGR_HPRE_0,
};

enum class APBPrescaler : uint32_t {
   div1 = 0,
#if defined(RCC_CFGR_PPRE1_DIV2)
   // STM32L0 and similar: named DIV defines
   div2 = RCC_CFGR_PPRE1_DIV2,
   div4 = RCC_CFGR_PPRE1_DIV4,
   div8 = RCC_CFGR_PPRE1_DIV8,
   div16 = RCC_CFGR_PPRE1_DIV16,
#elif defined(RCC_CFGR_PPRE1_2)
   // STM32WL and similar: PPRE1_x bit defines (encoding: 1xx = divide, xx = shift)
   div2 = RCC_CFGR_PPRE1_2,
   div4 = RCC_CFGR_PPRE1_2 | RCC_CFGR_PPRE1_0,
   div8 = RCC_CFGR_PPRE1_2 | RCC_CFGR_PPRE1_1,
   div16 = RCC_CFGR_PPRE1_2 | RCC_CFGR_PPRE1_1 | RCC_CFGR_PPRE1_0,
#else
   // STM32G0 and similar: PPRE_x bit defines
   div2 = RCC_CFGR_PPRE_2,
   div4 = RCC_CFGR_PPRE_2 | RCC_CFGR_PPRE_0,
   div8 = RCC_CFGR_PPRE_2 | RCC_CFGR_PPRE_1,
   div16 = RCC_CFGR_PPRE_2 | RCC_CFGR_PPRE_1 | RCC_CFGR_PPRE_0,
#endif
};

#if defined(RCC_CR_HSIDIV)
enum class HSIPrescaler : uint32_t {
   div1 = 0,
   div2 = RCC_CR_HSIDIV_0,
   div4 = RCC_CR_HSIDIV_1,
   div8 = RCC_CR_HSIDIV_1 | RCC_CR_HSIDIV_0,
   div16 = RCC_CR_HSIDIV_2,
   div32 = RCC_CR_HSIDIV_2 | RCC_CR_HSIDIV_0,
   div64 = RCC_CR_HSIDIV_2 | RCC_CR_HSIDIV_1,
   div128 = RCC_CR_HSIDIV_2 | RCC_CR_HSIDIV_1 | RCC_CR_HSIDIV_0,
};
#endif

#if defined(RCC_CR_HSIKERDIV)

enum class HSIKerDivider : uint32_t {
   div1 = 0,
   div2 = RCC_CR_HSIKERDIV_0,
   div3 = RCC_CR_HSIKERDIV_1,
   div4 = RCC_CR_HSIKERDIV_1 | RCC_CR_HSIKERDIV_0,
   div5 = RCC_CR_HSIKERDIV_2,
   div6 = RCC_CR_HSIKERDIV_2 | RCC_CR_HSIKERDIV_0,
   div7 = RCC_CR_HSIKERDIV_2 | RCC_CR_HSIKERDIV_1,
   div8 = RCC_CR_HSIKERDIV_2 | RCC_CR_HSIKERDIV_1 | RCC_CR_HSIKERDIV_0,
};
#endif
void enableHSI();
#if defined(RCC_CR_HSIDIV)
inline void setHSIPrescaler(HSIPrescaler prescaler) {
   RCC->CR = (RCC->CR & ~RCC_CR_HSIDIV) | static_cast<uint32_t>(prescaler);
}
#endif


inline void setHSITrim(uint32_t trim) {
   RCC->ICSCR = (RCC->ICSCR & ~RCC_ICSCR_HSITRIM) | (trim << RCC_ICSCR_HSITRIM_Pos);
}

#if defined(RCC_CR_HSIDIV)
inline HSIPrescaler getHSIPrescaler() {
   return static_cast<HSIPrescaler>(RCC->CR & RCC_CR_HSIDIV);
}
#endif

#if defined(RCC_CR_HSIKERDIV)
inline void setHSIKerDivider(HSIKerDivider prescaler) {
   RCC->CR = (RCC->CR & ~RCC_CR_HSIKERDIV) | static_cast<uint32_t>(prescaler);
}

inline HSIKerDivider getHSIKerDivider() {
   return static_cast<HSIKerDivider>(RCC->CR & RCC_CR_HSIKERDIV);
}
#endif

inline SystemClockSource getSystemClockSource() {
   return static_cast<SystemClockSource>(RCC->CFGR & RCC_CFGR_SWS);
}

void setSystemClockSource(SystemClockSource source);

inline void setAHBPrescaler(AHBPrescaler prescaler) {
   RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_HPRE) | static_cast<uint32_t>(prescaler);
}

inline AHBPrescaler getAHBPrescaler() {
   return static_cast<AHBPrescaler>(RCC->CFGR & RCC_CFGR_HPRE);
}

inline void setAPBPrescaler(APBPrescaler prescaler) {
#if defined(RCC_CFGR_PPRE1)
   RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_PPRE1) | static_cast<uint32_t>(prescaler);
#else
   RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_PPRE) | static_cast<uint32_t>(prescaler);
#endif
}

inline APBPrescaler getAPBPrescaler() {
#if defined(RCC_CFGR_PPRE1)
   return static_cast<APBPrescaler>(RCC->CFGR & RCC_CFGR_PPRE1);
#else
   return static_cast<APBPrescaler>(RCC->CFGR & RCC_CFGR_PPRE);
#endif
}

#if defined(RCC_CFGR_PPRE2)
inline void setAPB2Prescaler(APBPrescaler prescaler) {
   // APBPrescaler enum values are based on PPRE1, we need to shift them for PPRE2
   uint32_t val = (static_cast<uint32_t>(prescaler) >> RCC_CFGR_PPRE1_Pos) << RCC_CFGR_PPRE2_Pos;
   RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_PPRE2) | val;
}

inline APBPrescaler getAPB2Prescaler() {
   uint32_t val = (RCC->CFGR & RCC_CFGR_PPRE2) >> RCC_CFGR_PPRE2_Pos;
   return static_cast<APBPrescaler>(val << RCC_CFGR_PPRE1_Pos);
}
#endif

uint32_t getAPBClock();
#if defined(RCC_CFGR_PPRE2)
uint32_t getAPB2Clock();
#endif
#if defined(RCC_CR_HSION) && !defined(RCC_CR_HSIDIV)
uint32_t getHSI16Clock();
#endif
uint32_t getAHBClock();
#if defined(RCC_CR_HSIKERDIV)
uint32_t getHSIKerClock();
#elif defined(RCC_CR_HSIDIV)
uint32_t getHSIKerClock();
#elif defined(RCC_CR_HSION)
inline uint32_t getHSIKerClock() {
   return getHSI16Clock();
}
#endif

// Small internal helpers to keep bit operations readable without macros.
namespace detail {
inline void setBit(volatile uint32_t &reg, uint32_t bit) {
   reg |= bit;
}
inline void clearBit(volatile uint32_t &reg, uint32_t bit) {
   reg &= ~bit;
}
} // namespace detail

// -----------------
// GPIO (multiple series compatibility: L0 uses IOPENR/IOPSMENR, others use AHBxENR/AHBSMENR)
// -----------------
// STM32L0 uses RCC_IOPENR_IOPxEN naming, STM32G0 uses RCC_IOPENR_GPIOxEN naming
#if defined(RCC_IOPENR_IOPAEN) && defined(RCC_IOPSMENR_IOPASMEN)
inline void enableGPIOAClock() {
   detail::setBit(RCC->IOPENR, RCC_IOPENR_IOPAEN);
   detail::setBit(RCC->IOPSMENR, RCC_IOPSMENR_IOPASMEN);
}
inline void disableGPIOAClock() {
   detail::clearBit(RCC->IOPENR, RCC_IOPENR_IOPAEN);
   detail::clearBit(RCC->IOPSMENR, RCC_IOPSMENR_IOPASMEN);
}
inline void enableGPIOAClockInSleepMode() {
   enableGPIOAClock();
}
inline void disableGPIOAClockInSleepMode() {
   disableGPIOAClock();
}
#elif defined(RCC_IOPENR_IOPAEN)
inline void enableGPIOAClock() {
   detail::setBit(RCC->IOPENR, RCC_IOPENR_IOPAEN);
}
inline void disableGPIOAClock() {
   detail::clearBit(RCC->IOPENR, RCC_IOPENR_IOPAEN);
}
#elif defined(RCC_IOPENR_GPIOAEN)
inline void enableGPIOAClock() {
   detail::setBit(RCC->IOPENR, RCC_IOPENR_GPIOAEN);
}
inline void disableGPIOAClock() {
   detail::clearBit(RCC->IOPENR, RCC_IOPENR_GPIOAEN);
}
#endif
#if defined(RCC_IOPENR_IOPBEN) && defined(RCC_IOPSMENR_IOPBSMEN)
inline void enableGPIOBClock() {
   detail::setBit(RCC->IOPENR, RCC_IOPENR_IOPBEN);
   detail::setBit(RCC->IOPSMENR, RCC_IOPSMENR_IOPBSMEN);
}
inline void disableGPIOBClock() {
   detail::clearBit(RCC->IOPENR, RCC_IOPENR_IOPBEN);
   detail::clearBit(RCC->IOPSMENR, RCC_IOPSMENR_IOPBSMEN);
}
inline void enableGPIOBClockInSleepMode() {
   enableGPIOBClock();
}
inline void disableGPIOBClockInSleepMode() {
   disableGPIOBClock();
}
#elif defined(RCC_IOPENR_IOPBEN)
inline void enableGPIOBClock() {
   detail::setBit(RCC->IOPENR, RCC_IOPENR_IOPBEN);
}
inline void disableGPIOBClock() {
   detail::clearBit(RCC->IOPENR, RCC_IOPENR_IOPBEN);
}
#elif defined(RCC_IOPENR_GPIOBEN)
inline void enableGPIOBClock() {
   detail::setBit(RCC->IOPENR, RCC_IOPENR_GPIOBEN);
}
inline void disableGPIOBClock() {
   detail::clearBit(RCC->IOPENR, RCC_IOPENR_GPIOBEN);
}
#endif
#if defined(RCC_IOPENR_IOPCEN) && defined(RCC_IOPSMENR_IOPCSMEN)
inline void enableGPIOCClock() {
   detail::setBit(RCC->IOPENR, RCC_IOPENR_IOPCEN);
   detail::setBit(RCC->IOPSMENR, RCC_IOPSMENR_IOPCSMEN);
}
inline void disableGPIOCClock() {
   detail::clearBit(RCC->IOPENR, RCC_IOPENR_IOPCEN);
   detail::clearBit(RCC->IOPSMENR, RCC_IOPSMENR_IOPCSMEN);
}
inline void enableGPIOCClockInSleepMode() {
   enableGPIOCClock();
}
inline void disableGPIOCClockInSleepMode() {
   disableGPIOCClock();
}
#elif defined(RCC_IOPENR_IOPCEN)
inline void enableGPIOCClock() {
   detail::setBit(RCC->IOPENR, RCC_IOPENR_IOPCEN);
}
inline void disableGPIOCClock() {
   detail::clearBit(RCC->IOPENR, RCC_IOPENR_IOPCEN);
}
#elif defined(RCC_IOPENR_GPIOCEN)
inline void enableGPIOCClock() {
   detail::setBit(RCC->IOPENR, RCC_IOPENR_GPIOCEN);
}
inline void disableGPIOCClock() {
   detail::clearBit(RCC->IOPENR, RCC_IOPENR_GPIOCEN);
}
#endif
#if defined(RCC_IOPENR_IOPDEN) && defined(RCC_IOPSMENR_IOPDSMEN)
inline void enableGPIODClock() {
   detail::setBit(RCC->IOPENR, RCC_IOPENR_IOPDEN);
   detail::setBit(RCC->IOPSMENR, RCC_IOPSMENR_IOPDSMEN);
}
inline void disableGPIODClock() {
   detail::clearBit(RCC->IOPENR, RCC_IOPENR_IOPDEN);
   detail::clearBit(RCC->IOPSMENR, RCC_IOPSMENR_IOPDSMEN);
}
inline void enableGPIODClockInSleepMode() {
   enableGPIODClock();
}
inline void disableGPIODClockInSleepMode() {
   disableGPIODClock();
}
#elif defined(RCC_IOPENR_IOPDEN)
inline void enableGPIODClock() {
   detail::setBit(RCC->IOPENR, RCC_IOPENR_IOPDEN);
}
inline void disableGPIODClock() {
   detail::clearBit(RCC->IOPENR, RCC_IOPENR_IOPDEN);
}
#elif defined(RCC_IOPENR_GPIODEN)
inline void enableGPIODClock() {
   detail::setBit(RCC->IOPENR, RCC_IOPENR_GPIODEN);
}
inline void disableGPIODClock() {
   detail::clearBit(RCC->IOPENR, RCC_IOPENR_GPIODEN);
}
#endif
#if defined(RCC_IOPENR_IOPEEN) && defined(RCC_IOPSMENR_IOPESMEN)
inline void enableGPIOEClock() {
   detail::setBit(RCC->IOPENR, RCC_IOPENR_IOPEEN);
   detail::setBit(RCC->IOPSMENR, RCC_IOPSMENR_IOPESMEN);
}
inline void disableGPIOEClock() {
   detail::clearBit(RCC->IOPENR, RCC_IOPENR_IOPEEN);
   detail::clearBit(RCC->IOPSMENR, RCC_IOPSMENR_IOPESMEN);
}
inline void enableGPIOEClockInSleepMode() {
   enableGPIOEClock();
}
inline void disableGPIOEClockInSleepMode() {
   disableGPIOEClock();
}
#elif defined(RCC_IOPENR_IOPEEN)
inline void enableGPIOEClock() {
   detail::setBit(RCC->IOPENR, RCC_IOPENR_IOPEEN);
}
inline void disableGPIOEClock() {
   detail::clearBit(RCC->IOPENR, RCC_IOPENR_IOPEEN);
}
#elif defined(RCC_IOPENR_GPIOEEN)
inline void enableGPIOEClock() {
   detail::setBit(RCC->IOPENR, RCC_IOPENR_GPIOEEN);
}
inline void disableGPIOEClock() {
   detail::clearBit(RCC->IOPENR, RCC_IOPENR_GPIOEEN);
}
#endif
#if defined(RCC_IOPENR_GPIOFEN)
inline void enableGPIOFClock() {
   detail::setBit(RCC->IOPENR, RCC_IOPENR_GPIOFEN);
}
inline void disableGPIOFClock() {
   detail::clearBit(RCC->IOPENR, RCC_IOPENR_GPIOFEN);
}
#endif
#if defined(RCC_IOPENR_IOPHEN) && defined(RCC_IOPSMENR_IOPHSMEN)
inline void enableGPIOHClock() {
   detail::setBit(RCC->IOPENR, RCC_IOPENR_IOPHEN);
   detail::setBit(RCC->IOPSMENR, RCC_IOPSMENR_IOPHSMEN);
}
inline void disableGPIOHClock() {
   detail::clearBit(RCC->IOPENR, RCC_IOPENR_IOPHEN);
   detail::clearBit(RCC->IOPSMENR, RCC_IOPSMENR_IOPHSMEN);
}
inline void enableGPIOHClockInSleepMode() {
   enableGPIOHClock();
}
inline void disableGPIOHClockInSleepMode() {
   disableGPIOHClock();
}
#elif defined(RCC_IOPENR_IOPHEN)
inline void enableGPIOHClock() {
   detail::setBit(RCC->IOPENR, RCC_IOPENR_IOPHEN);
}
inline void disableGPIOHClock() {
   detail::clearBit(RCC->IOPENR, RCC_IOPENR_IOPHEN);
}
#endif

// Alternative GPIO for other series (AHB1ENR/AHB2ENR etc.)
#if defined(RCC_AHB2ENR_GPIOAEN)
inline void enableGPIOAClock() {
   detail::setBit(RCC->AHB2ENR, RCC_AHB2ENR_GPIOAEN);
}
inline void disableGPIOAClock() {
   detail::clearBit(RCC->AHB2ENR, RCC_AHB2ENR_GPIOAEN);
}
#elif defined(RCC_AHB1ENR_GPIOAEN)
inline void enableGPIOAClock() {
   detail::setBit(RCC->AHB1ENR, RCC_AHB1ENR_GPIOAEN);
}
inline void disableGPIOAClock() {
   detail::clearBit(RCC->AHB1ENR, RCC_AHB1ENR_GPIOAEN);
}
#endif
#if defined(RCC_AHB2ENR_GPIOBEN)
inline void enableGPIOBClock() {
   detail::setBit(RCC->AHB2ENR, RCC_AHB2ENR_GPIOBEN);
}
inline void disableGPIOBClock() {
   detail::clearBit(RCC->AHB2ENR, RCC_AHB2ENR_GPIOBEN);
}
#elif defined(RCC_AHB1ENR_GPIOBEN)
inline void enableGPIOBClock() {
   detail::setBit(RCC->AHB1ENR, RCC_AHB1ENR_GPIOBEN);
}
inline void disableGPIOBClock() {
   detail::clearBit(RCC->AHB1ENR, RCC_AHB1ENR_GPIOBEN);
}
#endif
#if defined(RCC_AHB2ENR_GPIOCEN)
inline void enableGPIOCClock() {
   detail::setBit(RCC->AHB2ENR, RCC_AHB2ENR_GPIOCEN);
}
inline void disableGPIOCClock() {
   detail::clearBit(RCC->AHB2ENR, RCC_AHB2ENR_GPIOCEN);
}
#elif defined(RCC_AHB1ENR_GPIOCEN)
inline void enableGPIOCClock() {
   detail::setBit(RCC->AHB1ENR, RCC_AHB1ENR_GPIOCEN);
}
inline void disableGPIOCClock() {
   detail::clearBit(RCC->AHB1ENR, RCC_AHB1ENR_GPIOCEN);
}
#endif
#if defined(RCC_AHB1ENR_GPIODEN)
inline void enableGPIODClock() {
   detail::setBit(RCC->AHB1ENR, RCC_AHB1ENR_GPIODEN);
}
inline void disableGPIODClock() {
   detail::clearBit(RCC->AHB1ENR, RCC_AHB1ENR_GPIODEN);
}
#endif
#if defined(RCC_AHB1ENR_GPIOEEN)
inline void enableGPIOEClock() {
   detail::setBit(RCC->AHB1ENR, RCC_AHB1ENR_GPIOEEN);
}
inline void disableGPIOEClock() {
   detail::clearBit(RCC->AHB1ENR, RCC_AHB1ENR_GPIOEEN);
}
#endif
#if defined(RCC_AHB1ENR_GPIOFEN)
inline void enableGPIOFClock() {
   detail::setBit(RCC->AHB1ENR, RCC_AHB1ENR_GPIOFEN);
}
inline void disableGPIOFClock() {
   detail::clearBit(RCC->AHB1ENR, RCC_AHB1ENR_GPIOFEN);
}
#endif
#if defined(RCC_AHB1ENR_GPIOGEN)
inline void enableGPIOGClock() {
   detail::setBit(RCC->AHB1ENR, RCC_AHB1ENR_GPIOGEN);
}
inline void disableGPIOGClock() {
   detail::clearBit(RCC->AHB1ENR, RCC_AHB1ENR_GPIOGEN);
}
#endif
#if defined(RCC_AHB2ENR_GPIOHEN)
inline void enableGPIOHClock() {
   detail::setBit(RCC->AHB2ENR, RCC_AHB2ENR_GPIOHEN);
}
inline void disableGPIOHClock() {
   detail::clearBit(RCC->AHB2ENR, RCC_AHB2ENR_GPIOHEN);
}
#elif defined(RCC_AHB1ENR_GPIOHEN)
inline void enableGPIOHClock() {
   detail::setBit(RCC->AHB1ENR, RCC_AHB1ENR_GPIOHEN);
}
inline void disableGPIOHClock() {
   detail::clearBit(RCC->AHB1ENR, RCC_AHB1ENR_GPIOHEN);
}
#endif

// DMA
#if defined(RCC_AHBENR_DMA1EN)
inline void enableDMA1Clock() {
   detail::setBit(RCC->AHBENR, RCC_AHBENR_DMA1EN);
}
inline void disableDMA1Clock() {
   detail::clearBit(RCC->AHBENR, RCC_AHBENR_DMA1EN);
}
#endif
#if defined(RCC_AHB1ENR_DMA1EN)
inline void enableDMA1Clock() {
   detail::setBit(RCC->AHB1ENR, RCC_AHB1ENR_DMA1EN);
}
inline void disableDMA1Clock() {
   detail::clearBit(RCC->AHB1ENR, RCC_AHB1ENR_DMA1EN);
}
#endif
#if defined(RCC_AHB1ENR_DMA2EN)
inline void enableDMA2Clock() {
   detail::setBit(RCC->AHB1ENR, RCC_AHB1ENR_DMA2EN);
}
inline void disableDMA2Clock() {
   detail::clearBit(RCC->AHB1ENR, RCC_AHB1ENR_DMA2EN);
}
#endif

// SYSCFG
#if defined(RCC_APB2ENR_SYSCFGEN) && defined(RCC_APB2SMENR_SYSCFGSMEN)
inline void enableSYSCFGClock() {
   detail::setBit(RCC->APB2ENR, RCC_APB2ENR_SYSCFGEN);
   detail::setBit(RCC->APB2SMENR, RCC_APB2SMENR_SYSCFGSMEN);
}
inline void disableSYSCFGClock() {
   detail::clearBit(RCC->APB2ENR, RCC_APB2ENR_SYSCFGEN);
   detail::clearBit(RCC->APB2SMENR, RCC_APB2SMENR_SYSCFGSMEN);
}
inline void enableSYSCFGClockInSleepMode() {
   enableSYSCFGClock();
}
inline void disableSYSCFGClockInSleepMode() {
   disableSYSCFGClock();
}
#elif defined(RCC_APB2ENR_SYSCFGEN)
inline void enableSYSCFGClock() {
   detail::setBit(RCC->APB2ENR, RCC_APB2ENR_SYSCFGEN);
}
inline void disableSYSCFGClock() {
   detail::clearBit(RCC->APB2ENR, RCC_APB2ENR_SYSCFGEN);
}
#elif defined(RCC_APBENR2_SYSCFGEN)
inline void enableSYSCFGClock() {
   detail::setBit(RCC->APBENR2, RCC_APBENR2_SYSCFGEN);
}
inline void disableSYSCFGClock() {
   detail::clearBit(RCC->APBENR2, RCC_APBENR2_SYSCFGEN);
}
#elif defined(RCC_APB2SMENR_SYSCFGSMEN)
inline void enableSYSCFGClockInSleepMode() {
   detail::setBit(RCC->APB2SMENR, RCC_APB2SMENR_SYSCFGSMEN);
}
inline void disableSYSCFGClockInSleepMode() {
   detail::clearBit(RCC->APB2SMENR, RCC_APB2SMENR_SYSCFGSMEN);
}
#endif

// PWR
#if defined(RCC_APB1ENR_PWREN) && defined(RCC_APB1SMENR_PWRSMEN)
inline void enablePWRClock() {
   detail::setBit(RCC->APB1ENR, RCC_APB1ENR_PWREN);
   detail::setBit(RCC->APB1SMENR, RCC_APB1SMENR_PWRSMEN);
}
inline void disablePWRClock() {
   detail::clearBit(RCC->APB1ENR, RCC_APB1ENR_PWREN);
   detail::clearBit(RCC->APB1SMENR, RCC_APB1SMENR_PWRSMEN);
}
inline void enablePWRClockInSleepMode() {
   enablePWRClock();
}
inline void disablePWRClockInSleepMode() {
   disablePWRClock();
}
#elif defined(RCC_APB1ENR_PWREN)
inline void enablePWRClock() {
   detail::setBit(RCC->APB1ENR, RCC_APB1ENR_PWREN);
}
inline void disablePWRClock() {
   detail::clearBit(RCC->APB1ENR, RCC_APB1ENR_PWREN);
}
#elif defined(RCC_APBENR1_PWREN)
inline void enablePWRClock() {
   detail::setBit(RCC->APBENR1, RCC_APBENR1_PWREN);
}
inline void disablePWRClock() {
   detail::clearBit(RCC->APBENR1, RCC_APBENR1_PWREN);
}
#elif defined(RCC_APB1SMENR_PWRSMEN)
inline void enablePWRClockInSleepMode() {
   detail::setBit(RCC->APB1SMENR, RCC_APB1SMENR_PWRSMEN);
}
inline void disablePWRClockInSleepMode() {
   detail::clearBit(RCC->APB1SMENR, RCC_APB1SMENR_PWRSMEN);
}
#endif

// USART/UART
#if defined(RCC_APB2ENR_USART1EN) && defined(RCC_APB2SMENR_USART1SMEN)
inline void enableUSART1Clock() {
   detail::setBit(RCC->APB2ENR, RCC_APB2ENR_USART1EN);
   detail::setBit(RCC->APB2SMENR, RCC_APB2SMENR_USART1SMEN);
}
inline void disableUSART1Clock() {
   detail::clearBit(RCC->APB2ENR, RCC_APB2ENR_USART1EN);
   detail::clearBit(RCC->APB2SMENR, RCC_APB2SMENR_USART1SMEN);
}
inline void enableUSART1ClockInSleepMode() {
   enableUSART1Clock();
}
inline void disableUSART1ClockInSleepMode() {
   disableUSART1Clock();
}
#elif defined(RCC_APB2ENR_USART1EN)
inline void enableUSART1Clock() {
   detail::setBit(RCC->APB2ENR, RCC_APB2ENR_USART1EN);
}
inline void disableUSART1Clock() {
   detail::clearBit(RCC->APB2ENR, RCC_APB2ENR_USART1EN);
}
#elif defined(RCC_APBENR2_USART1EN)
inline void enableUSART1Clock() {
   detail::setBit(RCC->APBENR2, RCC_APBENR2_USART1EN);
}
inline void disableUSART1Clock() {
   detail::clearBit(RCC->APBENR2, RCC_APBENR2_USART1EN);
}
#endif
#if defined(RCC_APB1ENR_USART2EN)
inline void enableUSART2Clock() {
   detail::setBit(RCC->APB1ENR, RCC_APB1ENR_USART2EN);
}
inline void disableUSART2Clock() {
   detail::clearBit(RCC->APB1ENR, RCC_APB1ENR_USART2EN);
}
#elif defined(RCC_APBENR1_USART2EN)
inline void enableUSART2Clock() {
   detail::setBit(RCC->APBENR1, RCC_APBENR1_USART2EN);
}
inline void disableUSART2Clock() {
   detail::clearBit(RCC->APBENR1, RCC_APBENR1_USART2EN);
}
#endif
#if defined(RCC_APB1ENR_USART3EN)
inline void enableUSART3Clock() {
   detail::setBit(RCC->APB1ENR, RCC_APB1ENR_USART3EN);
}
inline void disableUSART3Clock() {
   detail::clearBit(RCC->APB1ENR, RCC_APB1ENR_USART3EN);
}
#elif defined(RCC_APBENR1_USART3EN)
inline void enableUSART3Clock() {
   detail::setBit(RCC->APBENR1, RCC_APBENR1_USART3EN);
}
inline void disableUSART3Clock() {
   detail::clearBit(RCC->APBENR1, RCC_APBENR1_USART3EN);
}
#endif
#if defined(RCC_APB1ENR_USART4EN)
inline void enableUSART4Clock() {
   detail::setBit(RCC->APB1ENR, RCC_APB1ENR_USART4EN);
}
inline void disableUSART4Clock() {
   detail::clearBit(RCC->APB1ENR, RCC_APB1ENR_USART4EN);
}
#elif defined(RCC_APBENR1_USART4EN)
inline void enableUSART4Clock() {
   detail::setBit(RCC->APBENR1, RCC_APBENR1_USART4EN);
}
inline void disableUSART4Clock() {
   detail::clearBit(RCC->APBENR1, RCC_APBENR1_USART4EN);
}
#endif
#if defined(RCC_APB1ENR_USART5EN)
inline void enableUSART5Clock() {
   detail::setBit(RCC->APB1ENR, RCC_APB1ENR_USART5EN);
}
inline void disableUSART5Clock() {
   detail::clearBit(RCC->APB1ENR, RCC_APB1ENR_USART5EN);
}
#elif defined(RCC_APBENR1_USART5EN)
inline void enableUSART5Clock() {
   detail::setBit(RCC->APBENR1, RCC_APBENR1_USART5EN);
}
inline void disableUSART5Clock() {
   detail::clearBit(RCC->APBENR1, RCC_APBENR1_USART5EN);
}
#endif
#if defined(RCC_APB1ENR_UART4EN)
inline void enableUART4Clock() {
   detail::setBit(RCC->APB1ENR, RCC_APB1ENR_UART4EN);
}
inline void disableUART4Clock() {
   detail::clearBit(RCC->APB1ENR, RCC_APB1ENR_UART4EN);
}
#endif
#if defined(RCC_APB1ENR_UART5EN)
inline void enableUART5Clock() {
   detail::setBit(RCC->APB1ENR, RCC_APB1ENR_UART5EN);
}
inline void disableUART5Clock() {
   detail::clearBit(RCC->APB1ENR, RCC_APB1ENR_UART5EN);
}
#endif
#if defined(RCC_APB1ENR_LPUART1EN)
// Handled below together with SMEN bit (if available)
#endif

#if defined(RCC_APB1SMENR_USART2SMEN)
inline void enableUSART2ClockInSleepMode() {
   detail::setBit(RCC->APB1SMENR, RCC_APB1SMENR_USART2SMEN);
}
inline void disableUSART2ClockInSleepMode() {
   detail::clearBit(RCC->APB1SMENR, RCC_APB1SMENR_USART2SMEN);
}
#endif
#if defined(RCC_APB1SMENR_USART3SMEN)
inline void enableUSART3ClockInSleepMode() {
   detail::setBit(RCC->APB1SMENR, RCC_APB1SMENR_USART3SMEN);
}
inline void disableUSART3ClockInSleepMode() {
   detail::clearBit(RCC->APB1SMENR, RCC_APB1SMENR_USART3SMEN);
}
#endif
#if defined(RCC_APB1SMENR_USART4SMEN)
inline void enableUSART4ClockInSleepMode() {
   detail::setBit(RCC->APB1SMENR, RCC_APB1SMENR_USART4SMEN);
}
inline void disableUSART4ClockInSleepMode() {
   detail::clearBit(RCC->APB1SMENR, RCC_APB1SMENR_USART4SMEN);
}
#endif
#if defined(RCC_APB1SMENR_USART5SMEN)
inline void enableUSART5ClockInSleepMode() {
   detail::setBit(RCC->APB1SMENR, RCC_APB1SMENR_USART5SMEN);
}
inline void disableUSART5ClockInSleepMode() {
   detail::clearBit(RCC->APB1SMENR, RCC_APB1SMENR_USART5SMEN);
}
#endif
#if defined(RCC_APB1SMENR_LPUART1SMEN) && defined(RCC_APB1ENR_LPUART1EN)
inline void enableLPUART1Clock() {
   detail::setBit(RCC->APB1ENR, RCC_APB1ENR_LPUART1EN);
   detail::setBit(RCC->APB1SMENR, RCC_APB1SMENR_LPUART1SMEN);
}
inline void disableLPUART1Clock() {
   detail::clearBit(RCC->APB1ENR, RCC_APB1ENR_LPUART1EN);
   detail::clearBit(RCC->APB1SMENR, RCC_APB1SMENR_LPUART1SMEN);
}
inline void enableLPUART1ClockInSleepMode() {
   enableLPUART1Clock();
}
inline void disableLPUART1ClockInSleepMode() {
   disableLPUART1Clock();
}
#elif defined(RCC_APB1ENR_LPUART1EN)
inline void enableLPUART1Clock() {
   detail::setBit(RCC->APB1ENR, RCC_APB1ENR_LPUART1EN);
}
inline void disableLPUART1Clock() {
   detail::clearBit(RCC->APB1ENR, RCC_APB1ENR_LPUART1EN);
}
#elif defined(RCC_APBENR1_LPUART1EN)
inline void enableLPUART1Clock() {
   detail::setBit(RCC->APBENR1, RCC_APBENR1_LPUART1EN);
}
inline void disableLPUART1Clock() {
   detail::clearBit(RCC->APBENR1, RCC_APBENR1_LPUART1EN);
}
#elif defined(RCC_APB1SMENR_LPUART1SMEN)
inline void enableLPUART1ClockInSleepMode() {
   detail::setBit(RCC->APB1SMENR, RCC_APB1SMENR_LPUART1SMEN);
}
inline void disableLPUART1ClockInSleepMode() {
   detail::clearBit(RCC->APB1SMENR, RCC_APB1SMENR_LPUART1SMEN);
}
#endif

// I2C
#if defined(RCC_APB1ENR1_I2C1EN)
// STM32WL and similar: I2C1 clock on APB1ENR1
inline void enableI2C1Clock() {
   detail::setBit(RCC->APB1ENR1, RCC_APB1ENR1_I2C1EN);
}
inline void disableI2C1Clock() {
   detail::clearBit(RCC->APB1ENR1, RCC_APB1ENR1_I2C1EN);
}
#elif defined(RCC_APB1ENR_I2C1EN) && defined(RCC_APB1SMENR_I2C1SMEN)
inline void enableI2C1Clock() {
   detail::setBit(RCC->APB1ENR, RCC_APB1ENR_I2C1EN);
   detail::setBit(RCC->APB1SMENR, RCC_APB1SMENR_I2C1SMEN);
}
inline void disableI2C1Clock() {
   detail::clearBit(RCC->APB1ENR, RCC_APB1ENR_I2C1EN);
   detail::clearBit(RCC->APB1SMENR, RCC_APB1SMENR_I2C1SMEN);
}
inline void enableI2C1ClockInSleepMode() {
   enableI2C1Clock();
}
inline void disableI2C1ClockInSleepMode() {
   disableI2C1Clock();
}
#elif defined(RCC_APB1ENR_I2C1EN)
inline void enableI2C1Clock() {
   detail::setBit(RCC->APB1ENR, RCC_APB1ENR_I2C1EN);
}
inline void disableI2C1Clock() {
   detail::clearBit(RCC->APB1ENR, RCC_APB1ENR_I2C1EN);
}
#elif defined(RCC_APBENR1_I2C1EN)
inline void enableI2C1Clock() {
   detail::setBit(RCC->APBENR1, RCC_APBENR1_I2C1EN);
}
inline void disableI2C1Clock() {
   detail::clearBit(RCC->APBENR1, RCC_APBENR1_I2C1EN);
}
#endif
#if defined(RCC_APB1ENR1_I2C2EN) && defined(RCC_APB1SMENR1_I2C2SMEN)
inline void enableI2C2Clock() {
   detail::setBit(RCC->APB1ENR1, RCC_APB1ENR1_I2C2EN);
   detail::setBit(RCC->APB1SMENR1, RCC_APB1SMENR1_I2C2SMEN);
}
inline void disableI2C2Clock() {
   detail::clearBit(RCC->APB1ENR1, RCC_APB1ENR1_I2C2EN);
   detail::clearBit(RCC->APB1SMENR1, RCC_APB1SMENR1_I2C2SMEN);
}
inline void enableI2C2ClockInSleepMode() {
   enableI2C2Clock();
}
inline void disableI2C2ClockInSleepMode() {
   disableI2C2Clock();
}
#elif defined(RCC_APB1ENR1_I2C2EN)
inline void enableI2C2Clock() {
   detail::setBit(RCC->APB1ENR1, RCC_APB1ENR1_I2C2EN);
}
inline void disableI2C2Clock() {
   detail::clearBit(RCC->APB1ENR1, RCC_APB1ENR1_I2C2EN);
}
#elif defined(RCC_APB1ENR_I2C2EN) && defined(RCC_APB1SMENR_I2C2SMEN)
inline void enableI2C2Clock() {
   detail::setBit(RCC->APB1ENR, RCC_APB1ENR_I2C2EN);
   detail::setBit(RCC->APB1SMENR, RCC_APB1SMENR_I2C2SMEN);
}
inline void disableI2C2Clock() {
   detail::clearBit(RCC->APB1ENR, RCC_APB1ENR_I2C2EN);
   detail::clearBit(RCC->APB1SMENR, RCC_APB1SMENR_I2C2SMEN);
}
inline void enableI2C2ClockInSleepMode() {
   enableI2C2Clock();
}
inline void disableI2C2ClockInSleepMode() {
   disableI2C2Clock();
}
#elif defined(RCC_APB1ENR_I2C2EN)
inline void enableI2C2Clock() {
   detail::setBit(RCC->APB1ENR, RCC_APB1ENR_I2C2EN);
}
inline void disableI2C2Clock() {
   detail::clearBit(RCC->APB1ENR, RCC_APB1ENR_I2C2EN);
}
#elif defined(RCC_APBENR1_I2C2EN)
inline void enableI2C2Clock() {
   detail::setBit(RCC->APBENR1, RCC_APBENR1_I2C2EN);
}
inline void disableI2C2Clock() {
   detail::clearBit(RCC->APBENR1, RCC_APBENR1_I2C2EN);
}
#endif
#if defined(RCC_APB1ENR1_I2C3EN) && defined(RCC_APB1SMENR1_I2C3SMEN)
inline void enableI2C3Clock() {
   detail::setBit(RCC->APB1ENR1, RCC_APB1ENR1_I2C3EN);
   detail::setBit(RCC->APB1SMENR1, RCC_APB1SMENR1_I2C3SMEN);
}
inline void disableI2C3Clock() {
   detail::clearBit(RCC->APB1ENR1, RCC_APB1ENR1_I2C3EN);
   detail::clearBit(RCC->APB1SMENR1, RCC_APB1SMENR1_I2C3SMEN);
}
inline void enableI2C3ClockInSleepMode() {
   enableI2C3Clock();
}
inline void disableI2C3ClockInSleepMode() {
   disableI2C3Clock();
}
#elif defined(RCC_APB1ENR1_I2C3EN)
inline void enableI2C3Clock() {
   detail::setBit(RCC->APB1ENR1, RCC_APB1ENR1_I2C3EN);
}
inline void disableI2C3Clock() {
   detail::clearBit(RCC->APB1ENR1, RCC_APB1ENR1_I2C3EN);
}
#elif defined(RCC_APB1ENR_I2C3EN) && defined(RCC_APB1SMENR_I2C3SMEN)
inline void enableI2C3Clock() {
   detail::setBit(RCC->APB1ENR, RCC_APB1ENR_I2C3EN);
   detail::setBit(RCC->APB1SMENR, RCC_APB1SMENR_I2C3SMEN);
}
inline void disableI2C3Clock() {
   detail::clearBit(RCC->APB1ENR, RCC_APB1ENR_I2C3EN);
   detail::clearBit(RCC->APB1SMENR, RCC_APB1SMENR_I2C3SMEN);
}
inline void enableI2C3ClockInSleepMode() {
   enableI2C3Clock();
}
inline void disableI2C3ClockInSleepMode() {
   disableI2C3Clock();
}
#elif defined(RCC_APB1ENR_I2C3EN)
inline void enableI2C3Clock() {
   detail::setBit(RCC->APB1ENR, RCC_APB1ENR_I2C3EN);
}
inline void disableI2C3Clock() {
   detail::clearBit(RCC->APB1ENR, RCC_APB1ENR_I2C3EN);
}
#endif

// SPI
#if defined(RCC_APB2ENR_SPI1EN) && defined(RCC_APB2SMENR_SPI1SMEN)
inline void enableSPI1Clock() {
   detail::setBit(RCC->APB2ENR, RCC_APB2ENR_SPI1EN);
   detail::setBit(RCC->APB2SMENR, RCC_APB2SMENR_SPI1SMEN);
}
inline void disableSPI1Clock() {
   detail::clearBit(RCC->APB2ENR, RCC_APB2ENR_SPI1EN);
   detail::clearBit(RCC->APB2SMENR, RCC_APB2SMENR_SPI1SMEN);
}
inline void enableSPI1ClockInSleepMode() {
   enableSPI1Clock();
}
inline void disableSPI1ClockInSleepMode() {
   disableSPI1Clock();
}
#elif defined(RCC_APB2ENR_SPI1EN)
inline void enableSPI1Clock() {
   detail::setBit(RCC->APB2ENR, RCC_APB2ENR_SPI1EN);
}
inline void disableSPI1Clock() {
   detail::clearBit(RCC->APB2ENR, RCC_APB2ENR_SPI1EN);
}
#elif defined(RCC_APBENR2_SPI1EN)
inline void enableSPI1Clock() {
   detail::setBit(RCC->APBENR2, RCC_APBENR2_SPI1EN);
}
inline void disableSPI1Clock() {
   detail::clearBit(RCC->APBENR2, RCC_APBENR2_SPI1EN);
}
#endif
#if defined(RCC_APB1ENR1_SPI2EN) && defined(RCC_APB1SMENR1_SPI2SMEN)
inline void enableSPI2Clock() {
   detail::setBit(RCC->APB1ENR1, RCC_APB1ENR1_SPI2EN);
   detail::setBit(RCC->APB1SMENR1, RCC_APB1SMENR1_SPI2SMEN);
}
inline void disableSPI2Clock() {
   detail::clearBit(RCC->APB1ENR1, RCC_APB1ENR1_SPI2EN);
   detail::clearBit(RCC->APB1SMENR1, RCC_APB1SMENR1_SPI2SMEN);
}
inline void enableSPI2ClockInSleepMode() {
   enableSPI2Clock();
}
inline void disableSPI2ClockInSleepMode() {
   disableSPI2Clock();
}
#elif defined(RCC_APB1ENR1_SPI2EN)
inline void enableSPI2Clock() {
   detail::setBit(RCC->APB1ENR1, RCC_APB1ENR1_SPI2EN);
}
inline void disableSPI2Clock() {
   detail::clearBit(RCC->APB1ENR1, RCC_APB1ENR1_SPI2EN);
}
#elif defined(RCC_APB1ENR_SPI2EN) && defined(RCC_APB1SMENR_SPI2SMEN)
inline void enableSPI2Clock() {
   detail::setBit(RCC->APB1ENR, RCC_APB1ENR_SPI2EN);
   detail::setBit(RCC->APB1SMENR, RCC_APB1SMENR_SPI2SMEN);
}
inline void disableSPI2Clock() {
   detail::clearBit(RCC->APB1ENR, RCC_APB1ENR_SPI2EN);
   detail::clearBit(RCC->APB1SMENR, RCC_APB1SMENR_SPI2SMEN);
}
inline void enableSPI2ClockInSleepMode() {
   enableSPI2Clock();
}
inline void disableSPI2ClockInSleepMode() {
   disableSPI2Clock();
}
#elif defined(RCC_APB1ENR_SPI2EN)
inline void enableSPI2Clock() {
   detail::setBit(RCC->APB1ENR, RCC_APB1ENR_SPI2EN);
}
inline void disableSPI2Clock() {
   detail::clearBit(RCC->APB1ENR, RCC_APB1ENR_SPI2EN);
}
#elif defined(RCC_APBENR1_SPI2EN)
inline void enableSPI2Clock() {
   detail::setBit(RCC->APBENR1, RCC_APBENR1_SPI2EN);
}
inline void disableSPI2Clock() {
   detail::clearBit(RCC->APBENR1, RCC_APBENR1_SPI2EN);
}
#endif

// ADC
#if defined(RCC_APB2ENR_ADC1EN) && defined(RCC_APB2SMENR_ADC1SMEN)
inline void enableADC1Clock() {
   detail::setBit(RCC->APB2ENR, RCC_APB2ENR_ADC1EN);
   detail::setBit(RCC->APB2SMENR, RCC_APB2SMENR_ADC1SMEN);
}
inline void disableADC1Clock() {
   detail::clearBit(RCC->APB2ENR, RCC_APB2ENR_ADC1EN);
   detail::clearBit(RCC->APB2SMENR, RCC_APB2SMENR_ADC1SMEN);
}
inline void enableADC1ClockInSleepMode() {
   enableADC1Clock();
}
inline void disableADC1ClockInSleepMode() {
   disableADC1Clock();
}
#elif defined(RCC_APB2ENR_ADC1EN)
inline void enableADC1Clock() {
   detail::setBit(RCC->APB2ENR, RCC_APB2ENR_ADC1EN);
}
inline void disableADC1Clock() {
   detail::clearBit(RCC->APB2ENR, RCC_APB2ENR_ADC1EN);
}
#elif defined(RCC_APBENR2_ADCEN)
inline void enableADC1Clock() {
   detail::setBit(RCC->APBENR2, RCC_APBENR2_ADCEN);
}
inline void disableADC1Clock() {
   detail::clearBit(RCC->APBENR2, RCC_APBENR2_ADCEN);
}
#elif defined(RCC_APB2SMENR_ADC1SMEN)
inline void enableADC1ClockInSleepMode() {
   detail::setBit(RCC->APB2SMENR, RCC_APB2SMENR_ADC1SMEN);
}
inline void disableADC1ClockInSleepMode() {
   detail::clearBit(RCC->APB2SMENR, RCC_APB2SMENR_ADC1SMEN);
}
#endif

// Timers (selection varies by series)
#if defined(RCC_APB1ENR_TIM2EN) && defined(RCC_APB1SMENR_TIM2SMEN)
inline void enableTIM2Clock() {
   detail::setBit(RCC->APB1ENR, RCC_APB1ENR_TIM2EN);
   detail::setBit(RCC->APB1SMENR, RCC_APB1SMENR_TIM2SMEN);
}
inline void disableTIM2Clock() {
   detail::clearBit(RCC->APB1ENR, RCC_APB1ENR_TIM2EN);
   detail::clearBit(RCC->APB1SMENR, RCC_APB1SMENR_TIM2SMEN);
}
inline void enableTIM2ClockInSleepMode() {
   enableTIM2Clock();
}
inline void disableTIM2ClockInSleepMode() {
   disableTIM2Clock();
}
#elif defined(RCC_APB1ENR_TIM2EN)
inline void enableTIM2Clock() {
   detail::setBit(RCC->APB1ENR, RCC_APB1ENR_TIM2EN);
}
inline void disableTIM2Clock() {
   detail::clearBit(RCC->APB1ENR, RCC_APB1ENR_TIM2EN);
}
#elif defined(RCC_APBENR1_TIM2EN)
inline void enableTIM2Clock() {
   detail::setBit(RCC->APBENR1, RCC_APBENR1_TIM2EN);
}
inline void disableTIM2Clock() {
   detail::clearBit(RCC->APBENR1, RCC_APBENR1_TIM2EN);
}
#endif
#if defined(RCC_APB1ENR_TIM3EN) && defined(RCC_APB1SMENR_TIM3SMEN)
inline void enableTIM3Clock() {
   detail::setBit(RCC->APB1ENR, RCC_APB1ENR_TIM3EN);
   detail::setBit(RCC->APB1SMENR, RCC_APB1SMENR_TIM3SMEN);
}
inline void disableTIM3Clock() {
   detail::clearBit(RCC->APB1ENR, RCC_APB1ENR_TIM3EN);
   detail::clearBit(RCC->APB1SMENR, RCC_APB1SMENR_TIM3SMEN);
}
inline void enableTIM3ClockInSleepMode() {
   enableTIM3Clock();
}
inline void disableTIM3ClockInSleepMode() {
   disableTIM3Clock();
}
#elif defined(RCC_APB1ENR_TIM3EN)
inline void enableTIM3Clock() {
   detail::setBit(RCC->APB1ENR, RCC_APB1ENR_TIM3EN);
}
inline void disableTIM3Clock() {
   detail::clearBit(RCC->APB1ENR, RCC_APB1ENR_TIM3EN);
}
#elif defined(RCC_APBENR1_TIM3EN)
inline void enableTIM3Clock() {
   detail::setBit(RCC->APBENR1, RCC_APBENR1_TIM3EN);
}
inline void disableTIM3Clock() {
   detail::clearBit(RCC->APBENR1, RCC_APBENR1_TIM3EN);
}
#endif
#if defined(RCC_APB1ENR_TIM6EN)
inline void enableTIM6Clock() {
   detail::setBit(RCC->APB1ENR, RCC_APB1ENR_TIM6EN);
}
inline void disableTIM6Clock() {
   detail::clearBit(RCC->APB1ENR, RCC_APB1ENR_TIM6EN);
}
#elif defined(RCC_APBENR1_TIM6EN)
inline void enableTIM6Clock() {
   detail::setBit(RCC->APBENR1, RCC_APBENR1_TIM6EN);
}
inline void disableTIM6Clock() {
   detail::clearBit(RCC->APBENR1, RCC_APBENR1_TIM6EN);
}
#endif
#if defined(RCC_APB1ENR_TIM7EN)
inline void enableTIM7Clock() {
   detail::setBit(RCC->APB1ENR, RCC_APB1ENR_TIM7EN);
}
inline void disableTIM7Clock() {
   detail::clearBit(RCC->APB1ENR, RCC_APB1ENR_TIM7EN);
}
#elif defined(RCC_APBENR1_TIM7EN)
inline void enableTIM7Clock() {
   detail::setBit(RCC->APBENR1, RCC_APBENR1_TIM7EN);
}
inline void disableTIM7Clock() {
   detail::clearBit(RCC->APBENR1, RCC_APBENR1_TIM7EN);
}
#endif
#if defined(RCC_APB2ENR_TIM21EN) && defined(RCC_APB2SMENR_TIM21SMEN)
inline void enableTIM21Clock() {
   detail::setBit(RCC->APB2ENR, RCC_APB2ENR_TIM21EN);
   detail::setBit(RCC->APB2SMENR, RCC_APB2SMENR_TIM21SMEN);
}
inline void disableTIM21Clock() {
   detail::clearBit(RCC->APB2ENR, RCC_APB2ENR_TIM21EN);
   detail::clearBit(RCC->APB2SMENR, RCC_APB2SMENR_TIM21SMEN);
}
inline void enableTIM21ClockInSleepMode() {
   enableTIM21Clock();
}
inline void disableTIM21ClockInSleepMode() {
   disableTIM21Clock();
}
#elif defined(RCC_APB2ENR_TIM21EN)
inline void enableTIM21Clock() {
   detail::setBit(RCC->APB2ENR, RCC_APB2ENR_TIM21EN);
}
inline void disableTIM21Clock() {
   detail::clearBit(RCC->APB2ENR, RCC_APB2ENR_TIM21EN);
}
#endif
#if defined(RCC_APB2ENR_TIM22EN) && defined(RCC_APB2SMENR_TIM22SMEN)
inline void enableTIM22Clock() {
   detail::setBit(RCC->APB2ENR, RCC_APB2ENR_TIM22EN);
   detail::setBit(RCC->APB2SMENR, RCC_APB2SMENR_TIM22SMEN);
}
inline void disableTIM22Clock() {
   detail::clearBit(RCC->APB2ENR, RCC_APB2ENR_TIM22EN);
   detail::clearBit(RCC->APB2SMENR, RCC_APB2SMENR_TIM22SMEN);
}
inline void enableTIM22ClockInSleepMode() {
   enableTIM22Clock();
}
inline void disableTIM22ClockInSleepMode() {
   disableTIM22Clock();
}
#elif defined(RCC_APB2ENR_TIM22EN)
inline void enableTIM22Clock() {
   detail::setBit(RCC->APB2ENR, RCC_APB2ENR_TIM22EN);
}
inline void disableTIM22Clock() {
   detail::clearBit(RCC->APB2ENR, RCC_APB2ENR_TIM22EN);
}
#endif

// G0-series timers on APB2 (TIM1, TIM14, TIM15, TIM16, TIM17)
#if defined(RCC_APBENR2_TIM1EN)
inline void enableTIM1Clock() {
   detail::setBit(RCC->APBENR2, RCC_APBENR2_TIM1EN);
}
inline void disableTIM1Clock() {
   detail::clearBit(RCC->APBENR2, RCC_APBENR2_TIM1EN);
}
#endif
#if defined(RCC_APBENR2_TIM14EN)
inline void enableTIM14Clock() {
   detail::setBit(RCC->APBENR2, RCC_APBENR2_TIM14EN);
}
inline void disableTIM14Clock() {
   detail::clearBit(RCC->APBENR2, RCC_APBENR2_TIM14EN);
}
#endif
#if defined(RCC_APBENR2_TIM15EN)
inline void enableTIM15Clock() {
   detail::setBit(RCC->APBENR2, RCC_APBENR2_TIM15EN);
}
inline void disableTIM15Clock() {
   detail::clearBit(RCC->APBENR2, RCC_APBENR2_TIM15EN);
}
#endif
#if defined(RCC_APBENR2_TIM16EN)
inline void enableTIM16Clock() {
   detail::setBit(RCC->APBENR2, RCC_APBENR2_TIM16EN);
}
inline void disableTIM16Clock() {
   detail::clearBit(RCC->APBENR2, RCC_APBENR2_TIM16EN);
}
#endif
#if defined(RCC_APBENR2_TIM17EN)
inline void enableTIM17Clock() {
   detail::setBit(RCC->APBENR2, RCC_APBENR2_TIM17EN);
}
inline void disableTIM17Clock() {
   detail::clearBit(RCC->APBENR2, RCC_APBENR2_TIM17EN);
}
#endif

#if defined(RCC_APB1SMENR_TIM2SMEN) && !defined(RCC_APB1ENR_TIM2EN)
inline void enableTIM2ClockInSleepMode() {
   detail::setBit(RCC->APB1SMENR, RCC_APB1SMENR_TIM2SMEN);
}
inline void disableTIM2ClockInSleepMode() {
   detail::clearBit(RCC->APB1SMENR, RCC_APB1SMENR_TIM2SMEN);
}
#endif
#if defined(RCC_APB1SMENR_TIM3SMEN) && !defined(RCC_APB1ENR_TIM3EN)
inline void enableTIM3ClockInSleepMode() {
   detail::setBit(RCC->APB1SMENR, RCC_APB1SMENR_TIM3SMEN);
}
inline void disableTIM3ClockInSleepMode() {
   detail::clearBit(RCC->APB1SMENR, RCC_APB1SMENR_TIM3SMEN);
}
#endif
#if defined(RCC_APB1SMENR_TIM6SMEN) && !defined(RCC_APB1ENR_TIM6EN)
inline void enableTIM6ClockInSleepMode() {
   detail::setBit(RCC->APB1SMENR, RCC_APB1SMENR_TIM6SMEN);
}
inline void disableTIM6ClockInSleepMode() {
   detail::clearBit(RCC->APB1SMENR, RCC_APB1SMENR_TIM6SMEN);
}
#endif
#if defined(RCC_APB1SMENR_TIM7SMEN) && !defined(RCC_APB1ENR_TIM7EN)
inline void enableTIM7ClockInSleepMode() {
   detail::setBit(RCC->APB1SMENR, RCC_APB1SMENR_TIM7SMEN);
}
inline void disableTIM7ClockInSleepMode() {
   detail::clearBit(RCC->APB1SMENR, RCC_APB1SMENR_TIM7SMEN);
}
#endif
#if defined(RCC_APB2SMENR_TIM21SMEN) && !defined(RCC_APB2ENR_TIM21EN)
inline void enableTIM21ClockInSleepMode() {
   detail::setBit(RCC->APB2SMENR, RCC_APB2SMENR_TIM21SMEN);
}
inline void disableTIM21ClockInSleepMode() {
   detail::clearBit(RCC->APB2SMENR, RCC_APB2SMENR_TIM21SMEN);
}
#endif
#if defined(RCC_APB2SMENR_TIM22SMEN) && !defined(RCC_APB2ENR_TIM22EN)
inline void enableTIM22ClockInSleepMode() {
   detail::setBit(RCC->APB2SMENR, RCC_APB2SMENR_TIM22SMEN);
}
inline void disableTIM22ClockInSleepMode() {
   detail::clearBit(RCC->APB2SMENR, RCC_APB2SMENR_TIM22SMEN);
}
#endif

// LPTIM
#if defined(RCC_APB1ENR_LPTIM1EN) && defined(RCC_APB1SMENR_LPTIM1SMEN)
inline void enableLPTIM1Clock() {
   detail::setBit(RCC->APB1ENR, RCC_APB1ENR_LPTIM1EN);
   detail::setBit(RCC->APB1SMENR, RCC_APB1SMENR_LPTIM1SMEN);
}
inline void disableLPTIM1Clock() {
   detail::clearBit(RCC->APB1ENR, RCC_APB1ENR_LPTIM1EN);
   detail::clearBit(RCC->APB1SMENR, RCC_APB1SMENR_LPTIM1SMEN);
}
inline void enableLPTIM1ClockInSleepMode() {
   enableLPTIM1Clock();
}
inline void disableLPTIM1ClockInSleepMode() {
   disableLPTIM1Clock();
}
#elif defined(RCC_APB1ENR_LPTIM1EN)
inline void enableLPTIM1Clock() {
   detail::setBit(RCC->APB1ENR, RCC_APB1ENR_LPTIM1EN);
}
inline void disableLPTIM1Clock() {
   detail::clearBit(RCC->APB1ENR, RCC_APB1ENR_LPTIM1EN);
}
#elif defined(RCC_APBENR1_LPTIM1EN)
inline void enableLPTIM1Clock() {
   detail::setBit(RCC->APBENR1, RCC_APBENR1_LPTIM1EN);
}
inline void disableLPTIM1Clock() {
   detail::clearBit(RCC->APBENR1, RCC_APBENR1_LPTIM1EN);
}
#elif defined(RCC_APB1SMENR_LPTIM1SMEN)
inline void enableLPTIM1ClockInSleepMode() {
   detail::setBit(RCC->APB1SMENR, RCC_APB1SMENR_LPTIM1SMEN);
}
inline void disableLPTIM1ClockInSleepMode() {
   detail::clearBit(RCC->APB1SMENR, RCC_APB1SMENR_LPTIM1SMEN);
}
#endif

// RNG and LCD and USB (present on some L0 variants)
#if defined(RCC_APB2ENR_RNGEN)
inline void enableRNGClock() {
   detail::setBit(RCC->APB2ENR, RCC_APB2ENR_RNGEN);
}
inline void disableRNGClock() {
   detail::clearBit(RCC->APB2ENR, RCC_APB2ENR_RNGEN);
}
#endif
#if defined(RCC_APB1ENR_RTCAPBEN)
inline void enableRTCAPBClock() {
   detail::setBit(RCC->APB1ENR, RCC_APB1ENR_RTCAPBEN);
}
inline void disableRTCAPBClock() {
   detail::clearBit(RCC->APB1ENR, RCC_APB1ENR_RTCAPBEN);
}
#elif defined(RCC_APB1ENR1_RTCAPBEN)
// STM32L4/L5/U5/WL: RTC APB bus interface clock enable lives in APB1ENR1
// rather than a unified APB1ENR. Required before the RTC's registers
// (CR, ICSR/ISR, WUTR, ...) are readable/writable at all -- without it,
// every RTC register reads back as 0.
inline void enableRTCAPBClock() {
   detail::setBit(RCC->APB1ENR1, RCC_APB1ENR1_RTCAPBEN);
}
inline void disableRTCAPBClock() {
   detail::clearBit(RCC->APB1ENR1, RCC_APB1ENR1_RTCAPBEN);
}
#endif
#if defined(RCC_APB1ENR_LCDEN) && defined(RCC_APB1SMENR_LCDSMEN)
inline void enableLCDClock() {
   detail::setBit(RCC->APB1ENR, RCC_APB1ENR_LCDEN);
   detail::setBit(RCC->APB1SMENR, RCC_APB1SMENR_LCDSMEN);
}
inline void disableLCDClock() {
   detail::clearBit(RCC->APB1ENR, RCC_APB1ENR_LCDEN);
   detail::clearBit(RCC->APB1SMENR, RCC_APB1SMENR_LCDSMEN);
}
inline void enableLCDClockInSleepMode() {
   enableLCDClock();
}
inline void disableLCDClockInSleepMode() {
   disableLCDClock();
}
#elif defined(RCC_APB1ENR_LCDEN)
inline void enableLCDClock() {
   detail::setBit(RCC->APB1ENR, RCC_APB1ENR_LCDEN);
}
inline void disableLCDClock() {
   detail::clearBit(RCC->APB1ENR, RCC_APB1ENR_LCDEN);
}
#elif defined(RCC_APB1SMENR_LCDSMEN)
inline void enableLCDClockInSleepMode() {
   detail::setBit(RCC->APB1SMENR, RCC_APB1SMENR_LCDSMEN);
}
inline void disableLCDClockInSleepMode() {
   detail::clearBit(RCC->APB1SMENR, RCC_APB1SMENR_LCDSMEN);
}
#endif
#if defined(RCC_APB1ENR_USBEN) && defined(RCC_APB1SMENR_USBSMEN)
inline void enableUSBClock() {
   detail::setBit(RCC->APB1ENR, RCC_APB1ENR_USBEN);
   detail::setBit(RCC->APB1SMENR, RCC_APB1SMENR_USBSMEN);
}
inline void disableUSBClock() {
   detail::clearBit(RCC->APB1ENR, RCC_APB1ENR_USBEN);
   detail::clearBit(RCC->APB1SMENR, RCC_APB1SMENR_USBSMEN);
}
inline void enableUSBClockInSleepMode() {
   enableUSBClock();
}
inline void disableUSBClockInSleepMode() {
   disableUSBClock();
}
#elif defined(RCC_APB1ENR_USBEN)
inline void enableUSBClock() {
   detail::setBit(RCC->APB1ENR, RCC_APB1ENR_USBEN);
}
inline void disableUSBClock() {
   detail::clearBit(RCC->APB1ENR, RCC_APB1ENR_USBEN);
}
#elif defined(RCC_APB1SMENR_USBSMEN)
inline void enableUSBClockInSleepMode() {
   detail::setBit(RCC->APB1SMENR, RCC_APB1SMENR_USBSMEN);
}
inline void disableUSBClockInSleepMode() {
   detail::clearBit(RCC->APB1SMENR, RCC_APB1SMENR_USBSMEN);
}
#endif

// CRS (Clock Recovery System)
#if defined(RCC_APB1ENR_CRSEN) && defined(RCC_APB1SMENR_CRSSMEN)
inline void enableCRSClock() {
   detail::setBit(RCC->APB1ENR, RCC_APB1ENR_CRSEN);
   detail::setBit(RCC->APB1SMENR, RCC_APB1SMENR_CRSSMEN);
}
inline void disableCRSClock() {
   detail::clearBit(RCC->APB1ENR, RCC_APB1ENR_CRSEN);
   detail::clearBit(RCC->APB1SMENR, RCC_APB1SMENR_CRSSMEN);
}
inline void enableCRSClockInSleepMode() {
   enableCRSClock();
}
inline void disableCRSClockInSleepMode() {
   disableCRSClock();
}
#elif defined(RCC_APB1ENR_CRSEN)
inline void enableCRSClock() {
   detail::setBit(RCC->APB1ENR, RCC_APB1ENR_CRSEN);
}
inline void disableCRSClock() {
   detail::clearBit(RCC->APB1ENR, RCC_APB1ENR_CRSEN);
}
#elif defined(RCC_APB1SMENR_CRSSMEN)
inline void enableCRSClockInSleepMode() {
   detail::setBit(RCC->APB1SMENR, RCC_APB1SMENR_CRSSMEN);
}
inline void disableCRSClockInSleepMode() {
   detail::clearBit(RCC->APB1SMENR, RCC_APB1SMENR_CRSSMEN);
}
#endif

// TSC (Touch sensing controller)
#if defined(RCC_AHBENR_TSCEN) && defined(RCC_AHBSMENR_TSCSMEN)
inline void enableTSCClock() {
   detail::setBit(RCC->AHBENR, RCC_AHBENR_TSCEN);
   detail::setBit(RCC->AHBSMENR, RCC_AHBSMENR_TSCSMEN);
}
inline void disableTSCClock() {
   detail::clearBit(RCC->AHBENR, RCC_AHBENR_TSCEN);
   detail::clearBit(RCC->AHBSMENR, RCC_AHBSMENR_TSCSMEN);
}
inline void enableTSCClockInSleepMode() {
   enableTSCClock();
}
inline void disableTSCClockInSleepMode() {
   disableTSCClock();
}
#elif defined(RCC_AHBENR_TSCEN)
inline void enableTSCClock() {
   detail::setBit(RCC->AHBENR, RCC_AHBENR_TSCEN);
}
inline void disableTSCClock() {
   detail::clearBit(RCC->AHBENR, RCC_AHBENR_TSCEN);
}
#elif defined(RCC_AHBSMENR_TSCSMEN)
inline void enableTSCClockInSleepMode() {
   detail::setBit(RCC->AHBSMENR, RCC_AHBSMENR_TSCSMEN);
}
inline void disableTSCClockInSleepMode() {
   detail::clearBit(RCC->AHBSMENR, RCC_AHBSMENR_TSCSMEN);
}
#endif

// DAC
#if defined(RCC_APB1ENR_DACEN) && defined(RCC_APB1SMENR_DACSMEN)
inline void enableDACClock() {
   detail::setBit(RCC->APB1ENR, RCC_APB1ENR_DACEN);
   detail::setBit(RCC->APB1SMENR, RCC_APB1SMENR_DACSMEN);
}
inline void disableDACClock() {
   detail::clearBit(RCC->APB1ENR, RCC_APB1ENR_DACEN);
   detail::clearBit(RCC->APB1SMENR, RCC_APB1SMENR_DACSMEN);
}
inline void enableDACClockInSleepMode() {
   enableDACClock();
}
inline void disableDACClockInSleepMode() {
   disableDACClock();
}
#elif defined(RCC_APB1ENR_DACEN)
inline void enableDACClock() {
   detail::setBit(RCC->APB1ENR, RCC_APB1ENR_DACEN);
}
inline void disableDACClock() {
   detail::clearBit(RCC->APB1ENR, RCC_APB1ENR_DACEN);
}
#elif defined(RCC_APB1ENR_DAC1EN)
inline void enableDACClock() {
   detail::setBit(RCC->APB1ENR, RCC_APB1ENR_DAC1EN);
}
inline void disableDACClock() {
   detail::clearBit(RCC->APB1ENR, RCC_APB1ENR_DAC1EN);
}
#elif defined(RCC_APBENR1_DAC1EN)
inline void enableDACClock() {
   detail::setBit(RCC->APBENR1, RCC_APBENR1_DAC1EN);
}
inline void disableDACClock() {
   detail::clearBit(RCC->APBENR1, RCC_APBENR1_DAC1EN);
}
#endif

// LSI (Low-Speed Internal oscillator)
#if defined(RCC_CSR_LSION) && defined(RCC_CSR_LSIRDY)
inline void enableLSI() {
   detail::setBit(RCC->CSR, RCC_CSR_LSION);
   while(!(RCC->CSR & RCC_CSR_LSIRDY)) {
   }
}
inline void disableLSI() {
   detail::clearBit(RCC->CSR, RCC_CSR_LSION);
}
inline bool isLSIReady() {
   return (RCC->CSR & RCC_CSR_LSIRDY) != 0;
}
#endif

// MIF (NVM interface)
#if defined(RCC_AHBENR_MIFEN)
inline void enableMIFClock() {
   detail::setBit(RCC->AHBENR, RCC_AHBENR_MIFEN);
}
inline void disableMIFClock() {
   detail::clearBit(RCC->AHBENR, RCC_AHBENR_MIFEN);
}
#endif

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

// Configure system clock to 64 MHz using HSI16 via PLL (STM32G0 series).
// - Enables HSI16
// - Sets FLASH latency to 2 wait states (required for >48 MHz)
// - Enables prefetch and instruction cache
// - Configures PLL: source HSI16, M=/1, N=x8, R=/2 => 16 MHz * 8 / 2 = 64 MHz
// - Sets AHB/APB prescalers to DIV1
// - Switches SYSCLK to PLL and waits until ready
#if defined(RCC_PLLCFGR_PLLSRC_HSI) && defined(RCC_PLLCFGR_PLLM) && defined(RCC_PLLCFGR_PLLN) &&               \
defined(RCC_PLLCFGR_PLLR) && defined(RCC_PLLCFGR_PLLREN) && defined(RCC_CR_PLLON) && defined(RCC_CR_PLLRDY) && \
defined(RCC_CFGR_SW) && defined(RCC_CFGR_SWS) && defined(FLASH_ACR_LATENCY) && defined(RCC_CR_HSIDIV)
inline void setClock64MhzHSI() {
   // Enable HSI16 with no division and wait ready
   RCC->CR = (RCC->CR & ~RCC_CR_HSIDIV);
   RCC->CR |= RCC_CR_HSION;
   while((RCC->CR & RCC_CR_HSIRDY) == 0) {
   }

   // Set flash latency to 2 wait states for 64 MHz, enable prefetch and instruction cache
   // Note: FLASH_ACR_LATENCY_n defines are bit positions, not wait-state counts.
   // 2 wait states = value 2 = FLASH_ACR_LATENCY_1 (bit 1).
   FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY) | (2UL << FLASH_ACR_LATENCY_Pos)
#if defined(FLASH_ACR_PRFTEN)
                | FLASH_ACR_PRFTEN
#endif
#if defined(FLASH_ACR_ICEN)
                | FLASH_ACR_ICEN
#endif
   ;

   // Prescalers: AHB=1, APB=1
   RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_HPRE) | static_cast<uint32_t>(AHBPrescaler::div1);

   // Disable PLL if running
   if(RCC->CR & RCC_CR_PLLON) {
      RCC->CR &= ~RCC_CR_PLLON;
      while(RCC->CR & RCC_CR_PLLRDY) {
      }
   }

   // Configure PLL: source = HSI16, M=1 (0b000), N=8, R=2 (0b001)
   // VCO = 16 MHz / 1 * 8 = 128 MHz, PLLR = 128 / 2 = 64 MHz
   RCC->PLLCFGR = RCC_PLLCFGR_PLLSRC_HSI          // HSI16 as PLL source
                  | (0UL << RCC_PLLCFGR_PLLM_Pos) // M = 1 (field value 0)
                  | (8UL << RCC_PLLCFGR_PLLN_Pos) // N = 8
                  | (1UL << RCC_PLLCFGR_PLLR_Pos) // R = 2 (field value 1)
                  | RCC_PLLCFGR_PLLREN;           // Enable PLLR output

   // Enable PLL and wait ready
   RCC->CR |= RCC_CR_PLLON;
   while((RCC->CR & RCC_CR_PLLRDY) == 0) {
   }

   // Switch system clock to PLL
   RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | (0x2UL << RCC_CFGR_SW_Pos); // PLL = 0b010
   while((RCC->CFGR & RCC_CFGR_SWS) != (0x2UL << RCC_CFGR_SWS_Pos)) {
   }
}
#endif

// STM32WL: 48 MHz SYSCLK via HSE TCXO through SUBGHZSPI + PLL.
//
// Sequence:
//   1. Voltage range 1 (VOS=01, required for SYSCLK > 16 MHz)
//   2. Enable TCXO on SX1262 DIO3 via SetTcxoMode (opcode 0x97, 1.8 V, 5 ms timeout)
//      using the dedicated SUBGHZSPI peripheral — there is no GPIO power pin on STM32WLE5JC.
//   3. Set flash wait states: 2 WS (FLASH_ACR_LATENCY_1) for 48 MHz at VOS1
//   4. Enable HSE (32 MHz TCXO drives the HSE input) and wait for HSERDY
//   5. Configure PLL: HSE (32 MHz) / M=2 * N=6 / R=2 = 48 MHz, enable PLLR output
//   6. Switch SYSCLK to PLL
//
// Guarded by RCC_CR_HSEBYPPWR which is only defined in STM32WL CMSIS headers.
// This function runs before DATA/BSS initialisation (called from SystemClock_Config),
// so it must not access global variables — only registers and literal constants.
#if defined(RCC_CR_HSEBYPPWR) && defined(RCC_APB3ENR_SUBGHZSPIEN) && defined(PWR_CR1_VOS_0) && defined(PWR_SR2_RFBUSYS)
namespace detail {
// Exchange one byte with the sub-GHz radio over SUBGHZSPI, returning what the
// radio clocked back, or std::nullopt if TXE or RXNE never asserted before
// hw::Spi::transfer()'s bounded wait gave up. Delegates to hw::Spi rather
// than reimplementing the poll loop, because hw::Spi::transfer() also drains
// the RX FIFO and clears OVR on a timeout (see Spi.h) -- reimplementing the
// raw register polling here previously left a timed-out byte's stale data
// sitting in DR for the *next* transaction to read. hw::Spi is a reference
// wrapper with no static state, so this stays safe to call from the
// pre-DATA/BSS boot path below. The caller frames the transfer with
// PWR_SUBGHZSPICR.NSS; hw::Spi itself only owns TXE/RXNE, not NSS.
inline std::optional<uint8_t> subghzTransfer(uint8_t value) {
   const std::optional<uint16_t> response = hw::Spi(SUBGHZSPI).transfer(value);
   if(!response) {
      return std::nullopt;
   }
   return static_cast<uint8_t>(*response);
}
} // namespace detail

// Bounded poll of PWR_SR2.RFBUSYS, the STM32WL's mirror of the integrated
// radio's BUSY line. Used both as subghzCommand()'s own pre-wait and, on its
// own, as the SX126x board-HAL layer's SX126xWaitOnBusy() -- the radio has no
// discrete BUSY GPIO on this SoC, so this register poll is the only way to
// implement it. Returns false if the radio never went idle within the bound.
[[nodiscard]] inline bool subghzWaitOnBusy() {
   uint32_t spins = 100'000u;
   while((PWR->SR2 & PWR_SR2_RFBUSYS) && --spins) {
   }
   return spins != 0u;
}

// Frame one sub-GHz radio command: wait for the radio to go idle, assert NSS,
// exchange the bytes in place, wait for the shift register to drain, deassert
// NSS. Per SX1261/2 §13.5 the radio answers on every byte, so frame holds the
// responses on return; byte 0's response is RFU and byte 1 carries the status.
//
// Public (not detail::) because it is also the transport primitive for
// LoRaMac-node's SX126x board-HAL layer: SX126xWriteCommand/ReadCommand and
// SX126xWriteRegister(s)/ReadRegister(s) are each a single NSS-framed
// transaction that sends an opcode (plus address/data) and reads back the
// radio's per-byte status responses — see bsp/loramac_board's subghz_spi
// bindings. Both the "write" case (opcode + write-data, response bytes
// unread) and the "read" case (opcode + address/NOP turnaround + read-data)
// are the same in-place full-duplex exchange; callers just ignore the bytes
// they don't need back.
//
// SX126xWakeup() is deliberately not one of these "friends": it only pulses
// NSS to rouse the radio from Sleep and does not frame an SX126x command at
// all (see subghzWakeup() below).
//
// Precondition: SUBGHZSPI must already be enabled (SPE=1, via
// hw::Spi(SUBGHZSPI).enable()). This is the caller's responsibility rather
// than something this call manages itself, since a caller issuing a run of
// commands (a whole radio session) holds one enable/disable window around
// all of them instead of paying the toggle cost on every single command —
// see bsp/loramac_board's init()/deInit(). Calling this with SPE clear would
// otherwise fail silently: TXE reads set trivially while the peripheral is
// disabled, so nothing ever shifts and the frame comes back unchanged.
//
// Callers are responsible for serializing access to SUBGHZSPI: subghzCommand()
// and subghzWakeup() both perform a non-atomic NSS assert/deassert around the
// PWR_SUBGHZSPICR register, and LoRaMac-node's SX126x driver only ever issues
// one radio transaction at a time from its own task context, so no lock is
// taken here.
//
// Returns false — without partially reframing the failed transaction; NSS is
// always deasserted again before returning — if SPE is clear on entry, the
// radio never went idle (RFBUSYS), a byte's TXE/RXNE never asserted, or the
// shift register never drained (BSY) within the bounded wait. A caller must
// check the return value; a stalled link has no other way to be reported.
[[nodiscard]] inline bool subghzCommand(std::span<uint8_t> frame) {
   if(!(SUBGHZSPI->CR1 & SPI_CR1_SPE)) {
      return false;
   }

   if(!subghzWaitOnBusy()) {
      return false;
   }

   PWR->SUBGHZSPICR &= ~PWR_SUBGHZSPICR_NSS; // assert NSS
   bool ok = true;
   for(uint8_t &byte : frame) {
      const std::optional<uint8_t> response = detail::subghzTransfer(byte);
      if(!response) {
         ok = false;
         break;
      }
      byte = *response;
   }

   ok = ok && hw::Spi(SUBGHZSPI).waitUntilIdle();
   PWR->SUBGHZSPICR |= PWR_SUBGHZSPICR_NSS; // deassert NSS
   return ok;
}

// Wakes the sub-GHz radio from Sleep mode per RM0453's Sleep mode wakeup
// sequence: assert NSS, hold it low for a fixed duration, deassert NSS, then
// confirm the radio actually came up. No opcode or data byte is clocked
// out — this is the same no-SPI-bytes pulse ST's own STM32WL HAL uses to
// wake the integrated radio, not the GetStatus+NOP pulse discrete-SX126x
// reference board ports use to wake an externally-clocked chip; the two are
// not interchangeable, since clocking an opcode byte in here would shift it
// into a still-waking register with undefined effect.
//
// The hold cannot be a RFBUSYS poll the way subghzCommand()'s pre-wait is:
// while the radio is genuinely asleep, RFBUSYS already reads low, the same
// as when it is idle and ready (confirmed on hardware after a Stop2
// wakeup — see the comment on step 2 of setClock48MhzHseTcxoImpl() above).
// Polling it as the hold condition would therefore return on the very first
// read and barely dip NSS at all. Instead this holds NSS low for a fixed
// ~1 ms, the same duration ST's HAL uses, via delayuS() (calibrated off
// SystemCoreClock, so it stays correct regardless of which clock this runs
// under) — and only polls subghzWaitOnBusy() afterwards, once NSS is back
// high, to confirm the radio is actually ready rather than to time the hold
// itself. LoRaMac-node's own RADIO_WAKEUP_TIME budget for this step is 3 ms
// (sx126x.h), so a 1 ms hold plus the bounded post-wait comfortably fits.
//
// Deliberately does not reuse subghzCommand(): this call *is* what rouses
// the radio out of Sleep, so subghzCommand()'s own pre-wait on RFBUSYS would
// either return immediately (same reasoning as above) or, worse, go on to
// clock an opcode byte into the still-waking radio.
//
// Precondition: SPE already set, same as subghzCommand() and for the same
// reason — see the precondition note above subghzCommand()'s declaration.
// Returns false if the radio never reports idle within the post-wait bound;
// NSS is always deasserted before returning either way, matching
// subghzCommand()'s contract. See also the serialization note above
// subghzCommand()'s declaration, which applies here too.
[[nodiscard]] inline bool subghzWakeup() {
   if(!(SUBGHZSPI->CR1 & SPI_CR1_SPE)) {
      return false;
   }

   PWR->SUBGHZSPICR &= ~PWR_SUBGHZSPICR_NSS; // assert NSS
   delayuS(1000u); // fixed ~1 ms hold, see comment above
   PWR->SUBGHZSPICR |= PWR_SUBGHZSPICR_NSS; // deassert NSS
   return subghzWaitOnBusy();
}

namespace detail {
// GetDeviceErrors (0x17): OpError(15:0) comes back in bytes 2-3.
inline uint16_t subghzDeviceErrors() {
   uint8_t buf[4] = {0x17u, 0x00u, 0x00u, 0x00u};
   // Best-effort diagnostics: SPE is already enabled at every call site of
   // these detail:: helpers, so failure is not expected; on the rare timeout
   // buf keeps its zero-initialised bytes and the diagnostic reads as zero
   // rather than corrupting a caller's own data.
   (void)subghzCommand(buf);
   return (uint16_t)(((uint16_t)buf[2] << 8) | buf[3]);
}

// GetStatus (0xC0): the status byte comes back in byte 1.
inline uint8_t subghzStatus() {
   uint8_t buf[2] = {0xC0u, 0x00u};
   (void)subghzCommand(buf);
   return buf[1];
}

// ReadRegister (0x1D) of the LoRa sync word at 0x0740/0x0741, reset value
// 0x1424. Data only starts at byte 4, after the opcode, the two address bytes
// and one mandatory NOP (SX1261/2 Table 13-25).
inline uint16_t subghzSyncWord() {
   uint8_t buf[6] = {0x1Du, 0x07u, 0x40u, 0x00u, 0x00u, 0x00u};
   (void)subghzCommand(buf);
   return (uint16_t)(((uint16_t)buf[4] << 8) | buf[5]);
}

// Turn a bounded-wait countdown remainder into "iterations consumed", or -1 if
// the wait expired. Ambiguous only when the condition comes true on the very
// last iteration, which is reported as a timeout.
constexpr int32_t spinsUsed(uint32_t limit, uint32_t remaining) {
   return remaining == 0u ? -1 : (int32_t)(limit - remaining);
}
} // namespace detail

// Optional per-step record of what setClock48MhzHseTcxo() did, for diagnosing
// bring-up failures. Every *Spins field is the number of poll iterations that
// wait consumed, or -1 if it timed out, so both which step failed and how long
// each one took are visible, and a boot run can be diffed against a wakeup run.
//
// Pass a pointer to fill it; pass nullptr (the default) and nothing is recorded
// and no diagnostic radio traffic is generated. Optimised builds fold the whole
// thing away at the call site.
//
// POD with no member initialisers on purpose, so callers can place it in a
// .noinit section and read a capture taken before DATA/BSS initialisation.
struct ClockDiag {
   // State on entry, before the sequence touches anything.
   uint32_t rccCrEntry;
   uint32_t rccCsrEntry;
   uint32_t rccCfgrEntry;
   uint32_t rccApb3EnrEntry;
   uint32_t pwrCr1Entry;
   uint32_t pwrSr2Entry;

   // Radio reset: held asserted coming out of a system reset, already released
   // on a Stop2 wakeup.
   bool radioResetReleased;
   int32_t rfrstReleaseSpins;
   uint32_t rccCsrAfterReset;

   // SUBGHZSPI bring-up and the SetDIO3AsTCXOCtrl transfer.
   uint32_t spiSrAfterInit;
   int32_t busyIdleSpins;
   int32_t busyAfterNssSpins;
   uint8_t tcxoResponse[5];
   // Bit i set means tcxoResponse[i] is a genuine radio reply; bit i clear
   // means byte i timed out and tcxoResponse[i] is just 0, which is also a
   // value the radio could legitimately send back -- the mask, not the byte,
   // is what tells the two apart.
   uint8_t tcxoResponseOkMask;
   int32_t spiBusySpins;

   // Radio error controller, before and after ClearDeviceErrors. Bit 5 is
   // XOSC_START_ERR, bit 6 PLL_LOCK_ERR (SX1261/2 Table 13-86).
   uint8_t statusAfterTcxo;
   uint16_t opErrorAfterTcxo;
   uint16_t opErrorAfterClear;

   // Ground truth that SUBGHZSPI reaches the radio: must read 0x1424. Taken
   // under both PWR_CR1.SUBGHZSPINSSSEL settings, so a regression in the NSS
   // routing shows up as a difference rather than as garbage everywhere.
   uint16_t syncWordNssSel0;
   uint16_t syncWordNssSel1;

   // HSE32 (TCXO) start-up, then the radio re-read once the 32 MHz reference
   // is up: XOSC_START_ERR here means the radio really did not see the TCXO.
   uint32_t rccCrAfterHseByp;
   int32_t hseRdySpins;
   uint32_t rccCrAfterHse;
   uint16_t opErrorAfterHse;
   uint8_t statusAfterHse;

   // PLL lock and the SYSCLK switch.
   uint32_t rccPllCfgr;
   int32_t pllRdySpins;
   uint32_t rccCrAfterPll;
   int32_t swsSpins;

   // Final state.
   uint32_t rccCrFinal;
   uint32_t rccCfgrFinal;
   uint32_t pwrSr2Final;
   uint32_t flashAcrFinal;
   bool ok;
};

namespace detail {
// Shared implementation of the two setClock48MhzHseTcxo() overloads below.
//
// Record is a template parameter rather than a runtime flag so that the
// non-recording instantiation contains no diagnostic code whatsoever. The
// discarded `if constexpr` branches are never instantiated, so neither the
// recording nor the radio probe helpers it calls are odr-used, and none of it
// reaches a binary that does not ask for diagnostics. A runtime `if(diag)`
// could not achieve that: at ~1 KB this function is far too large for the
// compiler to inline and constant-fold at a nullptr call site, so every branch
// and every probe helper stayed in flash.
// Stand-in for the diagnostics parameter in the non-recording instantiation,
// so that instantiation carries no pointer and there is no nullability to
// reason about: the recording instantiation takes a reference that cannot be
// null, and the non-recording one takes an empty object it never touches.
struct NoClockDiag {};

template<bool Record>
inline bool setClock48MhzHseTcxoImpl(std::conditional_t<Record, ClockDiag &, NoClockDiag> diag) {

   if constexpr(Record) {
      diag.rccCrEntry = RCC->CR;
      diag.rccCsrEntry = RCC->CSR;
      diag.rccCfgrEntry = RCC->CFGR;
      diag.rccApb3EnrEntry = RCC->APB3ENR;
      diag.pwrCr1Entry = PWR->CR1;
      diag.pwrSr2Entry = PWR->SR2;
   }

   // 1. Ensure voltage range 1 (VOS = 01); default after reset but verify.
   if((PWR->CR1 & PWR_CR1_VOS) != PWR_CR1_VOS_0) {
      PWR->CR1 = (PWR->CR1 & ~PWR_CR1_VOS) | PWR_CR1_VOS_0;
      while(PWR->SR2 & PWR_SR2_VOSF) {
      }
   }

   // 2. Enable the TCXO via the SX1262 SetDIO3AsTCXOCtrl command.
   //
   // Release the radio reset if it is asserted; never re-assert it. The radio
   // is held in reset coming out of a system reset -- measured on a LoRa-E5,
   // RCC_CSR reads 0x0401C600 on entry at boot, with both RFRST and RFRSTF set
   // -- so the reset has to be released here. On a Stop2 wakeup both already
   // read 0 and the radio keeps the state it had before the sleep, so this
   // leaves it untouched.
   //
   // The previous unconditional reset pulse worked around the radio being left
   // with PWR_SR2.RFBUSYS stuck high when SetTcxoMode was re-sent to a
   // non-fresh radio. That symptom was a consequence of the SUBGHZSPINSSSEL
   // defect: NSS never toggled, so the radio sat mid-command indefinitely.
   // With the link working the workaround is unnecessary, and confirmed so on
   // hardware -- after a Stop2 wakeup with no reset, RFBUSYS is already low on
   // the first poll, GetDeviceErrors reads 0x0000 (the radio kept its TCXO
   // configuration across the sleep), and HSE/PLL lock exactly as at boot.
   //
   // Keeping the reset would also be actively harmful: it discards radio state
   // on every wakeup, which would break an active LoRaWAN session.
   uint32_t spins = 100'000u;
#if defined(RCC_CSR_RFRST) && defined(RCC_CSR_RFRSTF)
   const bool radioWasInReset = (RCC->CSR & RCC_CSR_RFRST) != 0u;
   if(radioWasInReset) {
      RCC->CSR &= ~RCC_CSR_RFRST;
      while((RCC->CSR & RCC_CSR_RFRSTF) && --spins) {
      }
   }
   if constexpr(Record) {
      diag.radioResetReleased = radioWasInReset;
      diag.rfrstReleaseSpins = radioWasInReset ? detail::spinsUsed(100'000u, spins) : 0;
   }
#elif defined(RCC_CSR_RFRST)
   RCC->CSR &= ~RCC_CSR_RFRST;
   if constexpr(Record) {
      diag.radioResetReleased = true;
      diag.rfrstReleaseSpins = 0;
   }
#endif
   if constexpr(Record) {
      diag.rccCsrAfterReset = RCC->CSR;
   }
   RCC->APB3ENR |= RCC_APB3ENR_SUBGHZSPIEN;
   RCC->APB3RSTR |= RCC_APB3RSTR_SUBGHZSPIRST;
   RCC->APB3RSTR &= ~RCC_APB3RSTR_SUBGHZSPIRST;
   // 8-bit data frame, RXFIFO threshold = 1 byte
   SUBGHZSPI->CR2 = SPI_CR2_FRXTH | (7u << SPI_CR2_DS_Pos);
   // Master, mode 0, BR=001 (~1 MHz on MSI), software NSS
   SUBGHZSPI->CR1 = SPI_CR1_MSTR | SPI_CR1_BR_0 | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_SPE;
   // Route NSS to PWR_SUBGHZSPICR for software control.
   //
   // SUBGHZSPINSSSEL must be *cleared* for this, not set. Determined by
   // measurement on a LoRa-E5 (STM32WLE5JC), since neither RM0453's SVD nor the
   // CMSIS headers describe the field: with the bit set from the start, every
   // response byte reads 0xAA and a ReadRegister of the LoRa sync word at
   // 0x0740 returns 0xAAAA instead of its 0x1424 reset value. With it cleared,
   // every command answers correctly and GetStatus returns a valid STBY_RC
   // status. Setting it again after the link is established does not break it,
   // so it is not a simple NSS on/off gate -- it has to be 0 while the link is
   // first established. 0 is also its reset value, so setting it here was not
   // merely redundant: it silently broke every SUBGHZSPI exchange below.
   PWR->CR1 &= ~PWR_CR1_SUBGHZSPINSSSEL;
   if constexpr(Record) {
      diag.spiSrAfterInit = SUBGHZSPI->SR;
   }
   PWR->SUBGHZSPICR |= PWR_SUBGHZSPICR_NSS; // NSS high — deasserted
   spins = 100'000u;
   while((PWR->SR2 & PWR_SR2_RFBUSYS) && --spins) {
   }
   if constexpr(Record) {
      diag.busyIdleSpins = detail::spinsUsed(100'000u, spins);
   }
   PWR->SUBGHZSPICR &= ~PWR_SUBGHZSPICR_NSS; // assert NSS
   spins = 100'000u;
   while((PWR->SR2 & PWR_SR2_RFBUSYS) && --spins) {
   }
   if constexpr(Record) {
      diag.busyAfterNssSpins = detail::spinsUsed(100'000u, spins);
   }
   // SetDIO3AsTCXOCtrl: opcode 0x97, tcxoVoltage 0x02 (DIO3 outputs 1.8 V),
   // delay 0x000140 (320 * 15.625 µs = 5 ms). SX1261/2 datasheet §13.3.6.
   uint8_t tcxoCmd[] = {0x97u, 0x02u, 0x00u, 0x01u, 0x40u};
   if constexpr(Record) {
      diag.tcxoResponseOkMask = 0u;
   }
   for(uint32_t i = 0u; i < sizeof(tcxoCmd); ++i) {
      const std::optional<uint8_t> response = detail::subghzTransfer(tcxoCmd[i]);
      if constexpr(Record) {
         diag.tcxoResponse[i] = response.value_or(0u);
         if(response) {
            diag.tcxoResponseOkMask |= static_cast<uint8_t>(1u << i);
         }
      }
   }
   spins = 100'000u;
   while((SUBGHZSPI->SR & SPI_SR_BSY) && --spins) {
   }
   if constexpr(Record) {
      diag.spiBusySpins = detail::spinsUsed(100'000u, spins);
   }
   PWR->SUBGHZSPICR |= PWR_SUBGHZSPICR_NSS; // deassert NSS

   if constexpr(Record) {
      diag.statusAfterTcxo = detail::subghzStatus();
      diag.opErrorAfterTcxo = detail::subghzDeviceErrors();
   }

   // Clear the radio's error controller. Per SX1261/2 §13.3.6, XOSC_START_ERR
   // "is raised at POR or at wake-up from Sleep mode in a cold-start condition,
   // when a TCXO is used. It is an expected behaviour since the chip is not yet
   // aware of being clocked by a TCXO. The user should simply clear this flag
   // with the ClearDeviceErrors command." Confirmed on hardware: after the
   // command above, GetDeviceErrors reads 0x0020 (bit 5, XOSC_START_ERR), and
   // ClearDeviceErrors takes it back to 0x0000 -- where it stays once HSE32 is
   // running, so the TCXO really does start.
   uint8_t clearErrCmd[] = {0x07u, 0x00u, 0x00u};
   (void)subghzCommand(clearErrCmd);

   if constexpr(Record) {
      diag.opErrorAfterClear = detail::subghzDeviceErrors();
      // Read the sync word under both NSS source settings. The link is already
      // established at this point, so flipping the bit here does not disturb
      // it; the pair is what makes a future regression in the routing above
      // legible instead of showing up as garbage in every field.
      diag.syncWordNssSel0 = detail::subghzSyncWord();
      PWR->CR1 |= PWR_CR1_SUBGHZSPINSSSEL;
      diag.syncWordNssSel1 = detail::subghzSyncWord();
      PWR->CR1 &= ~PWR_CR1_SUBGHZSPINSSSEL;
   }

   SUBGHZSPI->CR1 &= ~SPI_CR1_SPE;
   // Spin ~5 ms at reset clock (MSI 4 MHz ≈ 4 cycles/iteration)
   volatile uint32_t ticks = 20000u;
   while(ticks > 0) {
      ticks = ticks - 1;
   }

   // 3. Flash wait states: 2 WS required for SYSCLK > 36 MHz at VOS1
   FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY) | FLASH_ACR_LATENCY_1;
   while((FLASH->ACR & FLASH_ACR_LATENCY) != FLASH_ACR_LATENCY_1) {
   }

   // 4. Enable HSE in bypass/TCXO mode and wait for oscillator ready.
   //    HSEBYPPWR must be set before HSEON so the oscillator driver treats the input
   //    as an external clock (TCXO) rather than a crystal — without it HSERDY never asserts.
   //    PLL input: 32 MHz (HSE) / PLLM=÷2 = 16 MHz, which is the rated PLL-input maximum.
   RCC->CR |= RCC_CR_HSEBYPPWR;
   if constexpr(Record) {
      diag.rccCrAfterHseByp = RCC->CR;
   }
   RCC->CR |= RCC_CR_HSEON;
   spins = 500'000u;
   while(!(RCC->CR & RCC_CR_HSERDY) && --spins) {
   }
   if constexpr(Record) {
      diag.hseRdySpins = detail::spinsUsed(500'000u, spins);
      diag.rccCrAfterHse = RCC->CR;
      // Re-read the radio now the 32 MHz reference should be running. If
      // XOSC_START_ERR (bit 5) is set again here, the radio genuinely did not
      // see the TCXO; if it stays clear, the flag recorded above was the
      // expected cold-start artefact SX1261/2 §13.3.6 describes.
      SUBGHZSPI->CR1 |= SPI_CR1_SPE;
      diag.opErrorAfterHse = detail::subghzDeviceErrors();
      diag.statusAfterHse = detail::subghzStatus();
      SUBGHZSPI->CR1 &= ~SPI_CR1_SPE;
   }

   // 5. Configure and enable PLL
   //    PLLSRC = HSE (11), PLLM = 000 (÷2), PLLN = 6, PLLR = 001 (÷2), PLLREN = 1
   // BUG FIX: PLLR field bits were never actually set here (left at the
   // reserved value 0, "division factor cannot be 0" per RM0453), so the
   // real PLLR division was undefined/hardware-dependent instead of the
   // intended ÷2 -- and SystemCoreClockUpdate()'s standard field+1 formula
   // read it as ÷1, computing 96 MHz instead of the real ~48 MHz. Setting
   // RCC_PLLCFGR_PLLR_0 (field value 1) makes ÷2 explicit and RM-compliant.
   RCC->PLLCFGR = RCC_PLLCFGR_PLLREN | (6u << RCC_PLLCFGR_PLLN_Pos) | RCC_PLLCFGR_PLLM_0 |
                   RCC_PLLCFGR_PLLR_0 | RCC_PLLCFGR_PLLSRC_0 | RCC_PLLCFGR_PLLSRC_1;
   if constexpr(Record) {
      diag.rccPllCfgr = RCC->PLLCFGR;
   }
   RCC->CR |= RCC_CR_PLLON;
   spins = 500'000u;
   while(!(RCC->CR & RCC_CR_PLLRDY) && --spins) {
   }
   if constexpr(Record) {
      diag.pllRdySpins = detail::spinsUsed(500'000u, spins);
      diag.rccCrAfterPll = RCC->CR;
   }

   // 6. Switch SYSCLK to PLL (SW = 11) and wait for hardware confirmation
   RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_0 | RCC_CFGR_SW_1;
   spins = 500'000u;
   while(((RCC->CFGR & RCC_CFGR_SWS) != (RCC_CFGR_SWS_0 | RCC_CFGR_SWS_1)) && --spins) {
   }

   const bool locked = (RCC->CFGR & RCC_CFGR_SWS) == (RCC_CFGR_SWS_0 | RCC_CFGR_SWS_1);
   if constexpr(Record) {
      diag.swsSpins = detail::spinsUsed(500'000u, spins);
      diag.rccCrFinal = RCC->CR;
      diag.rccCfgrFinal = RCC->CFGR;
      diag.pwrSr2Final = PWR->SR2;
      diag.flashAcrFinal = FLASH->ACR;
      diag.ok = locked;
   }
   return locked;
}
} // namespace detail

// Returns true if SYSCLK is confirmed running on PLL (48 MHz) when this
// returns, false if HSE/PLL failed to lock and SYSCLK is left on whatever it
// was before (typically MSI). Callers that must keep working in the failure
// case should resync anything assuming a fixed 48 MHz (e.g. UART BRR, via
// SystemCoreClockUpdate() + Usart::setBaudRate()) when false is returned.
//
// On a successful call the clock is identical whether this ran at boot or
// after a Stop2 wakeup, so no resync is needed on the success path.
//
// This function talks to the sub-GHz radio (releasing its reset if held, then
// SetDIO3AsTCXOCtrl and ClearDeviceErrors). It does not reset the radio when
// it is already running, so calling it after a Stop2 wakeup preserves radio
// state -- but it is still not free, and callers should skip it entirely when
// RCC_CFGR.SWS already reads PLL.
inline bool setClock48MhzHseTcxo() {
   return detail::setClock48MhzHseTcxoImpl<false>({});
}

// As above, but records what every step did into @p diag, for diagnosing a
// bring-up that fails or misbehaves. See ClockDiag. This overload adds a few
// read-only radio transactions; the no-argument one above costs nothing.
inline bool setClock48MhzHseTcxo(ClockDiag &diag) {
   return detail::setClock48MhzHseTcxoImpl<true>(diag);
}
#endif

// -----------------
// Reset Cause & Flags
// -----------------
enum class ResetFlag : uint32_t {
#if defined(RCC_CSR_LPWRRSTF)
   LowPower = RCC_CSR_LPWRRSTF,
#endif
#if defined(RCC_CSR_WWDGRSTF)
   WindowWatchdog = RCC_CSR_WWDGRSTF,
#endif
#if defined(RCC_CSR_IWDGRSTF)
   IndependentWatchdog = RCC_CSR_IWDGRSTF,
#endif
#if defined(RCC_CSR_SFTRSTF)
   Software = RCC_CSR_SFTRSTF,
#endif
#if defined(RCC_CSR_BORRSTF)
   Brownout = RCC_CSR_BORRSTF,
#elif defined(RCC_CSR_PWRRSTF)
   Power = RCC_CSR_PWRRSTF,
#endif
#if defined(RCC_CSR_PINRSTF)
   Pin = RCC_CSR_PINRSTF,
#endif
#if defined(RCC_CSR_OBLRSTF)
   OptionByteLoader = RCC_CSR_OBLRSTF,
#endif
};

inline uint32_t getResetCause() {
   return RCC->CSR;
}

inline void clearResetFlags() {
#if defined(RCC_CSR_RMVF)
   RCC->CSR |= RCC_CSR_RMVF;
#endif
}

inline bool hasResetFlag(uint32_t resetCause, ResetFlag flag) {
   return (resetCause & static_cast<uint32_t>(flag)) != 0u;
}

// -----------------
// LSE (Low-Speed External Oscillator)
// -----------------
#if defined(RCC_BDCR_LSEDRV)
enum class LseDrive : uint32_t {
   Low = 0u,
   MediumLow = RCC_BDCR_LSEDRV_0,
   MediumHigh = RCC_BDCR_LSEDRV_1,
   High = RCC_BDCR_LSEDRV,
};
inline void setLSEDrive(LseDrive drive) {
   RCC->BDCR = (RCC->BDCR & ~RCC_BDCR_LSEDRV) | static_cast<uint32_t>(drive);
}
inline LseDrive getLSEDrive() {
   return static_cast<LseDrive>(RCC->BDCR & RCC_BDCR_LSEDRV);
}
#endif

#if defined(RCC_BDCR_LSEON)
inline void disableLSE() {
   RCC->BDCR &= ~RCC_BDCR_LSEON;
}
inline bool isLSEReady() {
   return (RCC->BDCR & RCC_BDCR_LSERDY) != 0u;
}
inline bool waitLSEReady(uint32_t timeoutLoops = 15'000'000u) {
   while(!isLSEReady()) {
      if(--timeoutLoops == 0u) {
         return false;
      }
   }
   return true;
}
inline bool enableLSE(uint32_t timeoutLoops = 15'000'000u) {
   RCC->BDCR |= RCC_BDCR_LSEON;
   return waitLSEReady(timeoutLoops);
}
#elif defined(RCC_CSR_LSEON)
inline void disableLSE() {
   RCC->CSR &= ~RCC_CSR_LSEON;
}
inline bool isLSEReady() {
   return (RCC->CSR & RCC_CSR_LSERDY) != 0u;
}
inline bool waitLSEReady(uint32_t timeoutLoops = 15'000'000u) {
   while(!isLSEReady()) {
      if(--timeoutLoops == 0u) {
         return false;
      }
   }
   return true;
}
inline bool enableLSE(uint32_t timeoutLoops = 15'000'000u) {
   RCC->CSR |= RCC_CSR_LSEON;
   return waitLSEReady(timeoutLoops);
}
#endif

// -----------------
// RTC Clock Source & Enable
// -----------------
enum class RtcClockSource : uint32_t {
   NoClock = 0,
   LSE = 1,
   LSI = 2,
   HSE = 3,
};

#if defined(RCC_BDCR_RTCSEL)
inline void setRtcClockSource(RtcClockSource source) {
   RCC->BDCR = (RCC->BDCR & ~RCC_BDCR_RTCSEL) | (static_cast<uint32_t>(source) << RCC_BDCR_RTCSEL_Pos);
}
inline RtcClockSource getRtcClockSource() {
   return static_cast<RtcClockSource>((RCC->BDCR & RCC_BDCR_RTCSEL) >> RCC_BDCR_RTCSEL_Pos);
}
inline void enableRTCClock() {
   RCC->BDCR |= RCC_BDCR_RTCEN;
}
inline void disableRTCClock() {
   RCC->BDCR &= ~RCC_BDCR_RTCEN;
}
inline bool isRTCClockEnabled() {
   return (RCC->BDCR & RCC_BDCR_RTCEN) != 0u;
}
#elif defined(RCC_CSR_RTCSEL)
inline void setRtcClockSource(RtcClockSource source) {
   RCC->CSR = (RCC->CSR & ~RCC_CSR_RTCSEL) | (static_cast<uint32_t>(source) << RCC_CSR_RTCSEL_Pos);
}
inline RtcClockSource getRtcClockSource() {
   return static_cast<RtcClockSource>((RCC->CSR & RCC_CSR_RTCSEL) >> RCC_CSR_RTCSEL_Pos);
}
inline void enableRTCClock() {
   RCC->CSR |= RCC_CSR_RTCEN;
}
inline void disableRTCClock() {
   RCC->CSR &= ~RCC_CSR_RTCEN;
}
inline bool isRTCClockEnabled() {
   return (RCC->CSR & RCC_CSR_RTCEN) != 0u;
}
#endif

} // namespace Rcc

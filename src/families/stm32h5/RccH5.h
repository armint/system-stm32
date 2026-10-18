#pragma once

// RCC of the STM32H5 (RM0481 section 11). peripherals/Rcc.h includes this
// file on the H5. The H5 RCC is not the L0/G0 RCC: CFGR1 (clock switch) and
// CFGR2 (bus prescalers), three PLLs with M/N/P/Q/R dividers, a 64 MHz HSI
// with HSIDIV, and the bus enables in AHB1ENR..APB3ENR. Thus the H5 has its
// own header. Function names follow the shared Rcc.h where a function exists
// there.

#include <cmsis_device.h>
#include <cstdint>
#include <optional>

// Keep in step with system_stm32h5xx.c: SystemCoreClockUpdate() uses the
// same value. CMakeLists.txt defines HSE_VALUE for all translation units.
#if !defined(HSE_VALUE)
#error "HSE_VALUE is not defined: set SYSTEM_STM32_HSE_VALUE in CMake"
#endif
#if !defined(LSE_VALUE)
#define LSE_VALUE (32768UL)
#endif

namespace Rcc {

// HSI before HSIDIV. HSIDIV is /2 after reset, so hsi_ck is 32 MHz.
constexpr uint32_t HSIMaxHz = 64000000UL;
constexpr uint32_t CSIHz = 4000000UL;
constexpr uint32_t SysClock250MHz = 250000000UL;

namespace detail {
// Peripheral clock enable. The read-back gives the delay of RM0481
// section 11.4.16 before the first access to the peripheral.
inline void enableClock(volatile uint32_t &reg, uint32_t bit) {
   reg |= bit;
   [[maybe_unused]] const uint32_t readBack = reg;
}
} // namespace detail

// -----------------
// Peripheral clocks (only the peripherals that system-stm32 drives on the H5)
// -----------------
inline void enableGPIOAClock() {
   detail::enableClock(RCC->AHB2ENR, RCC_AHB2ENR_GPIOAEN);
}
inline void enableGPIOBClock() {
   detail::enableClock(RCC->AHB2ENR, RCC_AHB2ENR_GPIOBEN);
}
inline void enableGPIOCClock() {
   detail::enableClock(RCC->AHB2ENR, RCC_AHB2ENR_GPIOCEN);
}
inline void enableGPIODClock() {
   detail::enableClock(RCC->AHB2ENR, RCC_AHB2ENR_GPIODEN);
}
inline void enableGPIOHClock() {
   detail::enableClock(RCC->AHB2ENR, RCC_AHB2ENR_GPIOHEN);
}
inline void enableTIM1Clock() {
   detail::enableClock(RCC->APB2ENR, RCC_APB2ENR_TIM1EN);
}
inline void enableTIM2Clock() {
   detail::enableClock(RCC->APB1LENR, RCC_APB1LENR_TIM2EN);
}
inline void enableTIM3Clock() {
   detail::enableClock(RCC->APB1LENR, RCC_APB1LENR_TIM3EN);
}
inline void enableUSART1Clock() {
   detail::enableClock(RCC->APB2ENR, RCC_APB2ENR_USART1EN);
}
inline void enableUSART2Clock() {
   detail::enableClock(RCC->APB1LENR, RCC_APB1LENR_USART2EN);
}
inline void enableUSART3Clock() {
   detail::enableClock(RCC->APB1LENR, RCC_APB1LENR_USART3EN);
}
inline void enableSPI1Clock() {
   detail::enableClock(RCC->APB2ENR, RCC_APB2ENR_SPI1EN);
}
inline void enableSPI2Clock() {
   detail::enableClock(RCC->APB1LENR, RCC_APB1LENR_SPI2EN);
}
inline void enableSPI3Clock() {
   detail::enableClock(RCC->APB1LENR, RCC_APB1LENR_SPI3EN);
}
inline void enableUSBClock() {
   detail::enableClock(RCC->APB2ENR, RCC_APB2ENR_USBEN);
}
inline void enableGPDMA1Clock() {
   detail::enableClock(RCC->AHB1ENR, RCC_AHB1ENR_GPDMA1EN);
}
inline void enableGPDMA2Clock() {
   detail::enableClock(RCC->AHB1ENR, RCC_AHB1ENR_GPDMA2EN);
}
inline void enableCRSClock() {
   detail::enableClock(RCC->APB1LENR, RCC_APB1LENR_CRSEN);
}

// -----------------
// Bus clocks
// -----------------
// hsi_ck: 64 MHz / HSIDIV (1, 2, 4 or 8).
inline uint32_t getHSIClock() {
   return HSIMaxHz >> ((RCC->CR & RCC_CR_HSIDIV) >> RCC_CR_HSIDIV_Pos);
}
// On the H5, SystemCoreClock is hclk: SystemCoreClockUpdate() applies HPRE.
inline uint32_t getAHBClock() {
   return SystemCoreClock;
}
uint32_t getAPBClock(); // APB1 (pclk1)
uint32_t getAPB2Clock();
uint32_t getAPB3Clock();
// pll1_q_ck: the reset kernel clock of SPI1, SPI2 and SPI3 (RCC_CCIPR3).
// 0 if PLL1 is off or its Q output is disabled.
uint32_t getPLL1QClock();

// -----------------
// HSI48 and the USB kernel clock
// -----------------
// Starts HSI48 and waits until it is ready. false: no ready flag in time.
[[nodiscard]] bool enableHSI48();
inline void disableHSI48() {
   RCC->CR &= ~RCC_CR_HSI48ON;
}
inline bool isHSI48Ready() {
   return (RCC->CR & RCC_CR_HSI48RDY) != 0;
}

// RCC_CCIPR4.USBSEL. USB needs 48 MHz: use HSI48, trimmed by the CRS from
// USB SOF (peripherals/Crs.h), as on L0 and G0B1.
enum class USBClockSource : uint32_t {
   None = 0,
   PLL1Q = RCC_CCIPR4_USBSEL_0,
   PLL3Q = RCC_CCIPR4_USBSEL_1,
   HSI48 = RCC_CCIPR4_USBSEL,
};
inline void setUSBClockSource(USBClockSource source) {
   RCC->CCIPR4 = (RCC->CCIPR4 & ~RCC_CCIPR4_USBSEL) | static_cast<uint32_t>(source);
}
inline USBClockSource getUSBClockSource() {
   return static_cast<USBClockSource>(RCC->CCIPR4 & RCC_CCIPR4_USBSEL);
}

// -----------------
// System clock: 250 MHz from PLL1
// -----------------
namespace detail {
struct Pll1Config {
   uint32_t cfgr; // RCC_PLL1CFGR without PLL1SRC
   uint32_t divr; // RCC_PLL1DIVR
};

// PLL1 settings for sys_ck = 250 MHz from an input clock of inputHz, or
// std::nullopt if no setting gives exactly 250 MHz.
// ref1_ck = 2 MHz: the wide VCO range needs 2 to 16 MHz (DS14539 table 47),
// and 2 MHz gives the lowest long-term jitter. vco1_ck = 2 MHz x 250 =
// 500 MHz (wide range: 128 to 560 MHz). P = 2: 250 MHz. Q = 4: 125 MHz SPI
// kernel clock, so the SPI baud prescaler (/2 to /256) gives 62.5, 31.25,
// 15.6, 7.8, 3.9, 1.95, 0.98 and 0.49 MHz: each just below a usual JTAG and
// SWD clock request of 64, 32, 16, 8, 4, 2, 1 or 0.5 MHz.
constexpr std::optional<Pll1Config> pll1For250Mhz(uint32_t inputHz) {
   constexpr uint32_t refHz = 2000000UL;
   constexpr uint32_t n = 250, p = 2, q = 4, r = 2;
   if(inputHz % refHz != 0) {
      return std::nullopt;
   }
   const uint32_t m = inputHz / refHz;
   if(m < 1 || m > 63) {
      return std::nullopt;
   }
   // PLL1RGE = 01: ref1_ck 2 to 4 MHz. PLL1VCOSEL = 0: wide VCO range.
   return Pll1Config{ (m << RCC_PLL1CFGR_PLL1M_Pos) | RCC_PLL1CFGR_PLL1RGE_0 | RCC_PLL1CFGR_PLL1PEN |
                         RCC_PLL1CFGR_PLL1QEN,
                      ((n - 1) << RCC_PLL1DIVR_PLL1N_Pos) | ((p - 1) << RCC_PLL1DIVR_PLL1P_Pos) |
                         ((q - 1) << RCC_PLL1DIVR_PLL1Q_Pos) | ((r - 1) << RCC_PLL1DIVR_PLL1R_Pos) };
}
// NUCLEO-H533RE: HSE 24 MHz -> M = 12, N = 250, P = 2, Q = 4.
static_assert(pll1For250Mhz(24000000UL)->cfgr == 0x00030C04UL);
static_assert(pll1For250Mhz(24000000UL)->divr == 0x010302F9UL);
// HSI after reset (64 MHz / 2) -> M = 16; HSI /1 -> M = 32; HSE 25 MHz: no setting.
static_assert(pll1For250Mhz(32000000UL)->cfgr == 0x00031004UL);
static_assert(pll1For250Mhz(64000000UL)->cfgr == 0x00032004UL);
static_assert(!pll1For250Mhz(25000000UL) && !pll1For250Mhz(128000000UL) && !pll1For250Mhz(0));

// The waits run from HSI (32 MHz), a few cycles per iteration. HSE: some
// 10 ms (crystal start-up is about 2 ms); PLL: about 1 ms (lock is 100 us
// max); VOS: about 1 ms for each step.
constexpr uint32_t HSEReadyLoops = 200000;
constexpr uint32_t PLLReadyLoops = 10000;
constexpr uint32_t VOSReadyLoops = 10000;
} // namespace detail

/**
 * @brief Sets sys_ck, hclk and pclk1..3 to 250 MHz from PLL1, with the HSE
 * crystal (HSE_VALUE) as the PLL input. Sets VOS0, 5 flash wait states,
 * WRHIGHFREQ = 2 and the prefetch, then calls SystemCoreClockUpdate().
 * pll1_q_ck is 125 MHz (see detail::pll1For250Mhz()).
 * No frequency parameter: SystemCoreClockUpdate() also uses HSE_VALUE, so
 * SystemCoreClock always agrees with the real clock.
 * @return false if HSE_VALUE gives no exact 250 MHz, if PLL2 or PLL3 is on
 * (the hardware then keeps PLL1SRC), or if HSE, VOS0 or PLL1 is not ready in
 * time. The system clock then stays on HSI.
 */
[[nodiscard]] bool setClock250MhzHSE();

/**
 * @brief As setClock250MhzHSE(), but with hsi_ck (64 MHz / HSIDIV) as the
 * PLL input. For boards without an HSE crystal.
 */
[[nodiscard]] bool setClock250MhzHSI();

// -----------------
// Instruction cache
// -----------------
/**
 * @brief Waits for the end of the cache invalidation after reset, then
 * enables the ICACHE (n-way, the reset configuration).
 *
 * The ICACHE caches the code region (0x0000 0000 to 0x1FFF FFFF), thus also
 * the flash. It reports a cacheable write as an error: disable it while the
 * firmware programs the flash (RM0481 section 8.4.5).
 *
 * The read-only flash area (UID, VREFINT_CAL, TS_CAL) must not be cached
 * (RM0481 section 7.3.2): without the MPU region of
 * disableCacheOfReadOnlyFlash(), reading it faults. Call that function
 * before this one, or program an equivalent MPU region.
 */
inline void enableICache() {
   while(ICACHE->SR & ICACHE_SR_BUSYF) {
   }
   ICACHE->CR |= ICACHE_CR_EN;
}

/**
 * @brief Makes the read-only flash area 0x08FFF000 to 0x08FFFFFF normal
 * non-cacheable memory (RM0481 section 7.3.2), so the ICACHE skips it.
 *
 * Uses MPU region 0 and memory attribute 0 (MAIR0 byte 0), and enables the
 * MPU with the default memory map as background for privileged code. The
 * region covers the OTP area (0x08FFF000 to 0x08FFF7FF) and the read-only
 * area above it. It is read-only and never executable, thus a write to the
 * OTP faults: change the region first to program the OTP. Unprivileged code
 * cannot access anything outside this region. Call it before enableICache().
 * Limit: it overwrites region 0 and attribute 0. If the firmware has its own
 * MPU setup, program an equivalent region there instead.
 */
inline void disableCacheOfReadOnlyFlash() {
   constexpr uint32_t base = 0x08FFF000UL;
   constexpr uint32_t last = 0x08FFFFFFUL;
   constexpr uint32_t attrNormalNonCacheable = 0x44UL; // inner and outer
   constexpr uint32_t apReadOnlyAny = 3UL << MPU_RBAR_AP_Pos;
   MPU->CTRL &= ~MPU_CTRL_ENABLE_Msk;
   __DSB();
   __ISB();
   MPU->MAIR0 = (MPU->MAIR0 & ~0xFFUL) | attrNormalNonCacheable;
   MPU->RNR = 0;
   MPU->RBAR = base | apReadOnlyAny | MPU_RBAR_XN_Msk;
   MPU->RLAR = (last & MPU_RLAR_LIMIT_Msk) | MPU_RLAR_EN_Msk; // AttrIndx 0
   MPU->CTRL |= MPU_CTRL_PRIVDEFENA_Msk | MPU_CTRL_ENABLE_Msk;
   __DSB();
   __ISB();
}

inline void disableICache() {
   ICACHE->CR &= ~ICACHE_CR_EN;
}

} // namespace Rcc

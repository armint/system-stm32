#pragma once

#include <cmsis_device.h>
#include <cstdint>

// STM32F1 alternate-function I/O (AFIO) remap and debug port configuration.
//
// F1 has no per-pin AF number. Each peripheral has a default pin set and
// zero or more remapped pin sets, selected in AFIO_MAPR (RM0008 section
// 9.3 and 9.4.2). All functions enable the AFIO clock themselves.

namespace hw::Afio {

// Serial wire JTAG configuration (AFIO_MAPR.SWJ_CFG).
enum class SwjConfig : uint32_t {
   // JTAG-DP and SW-DP (reset state). Uses PA13, PA14, PA15, PB3, PB4.
   Full = AFIO_MAPR_SWJ_CFG_RESET,
   // JTAG-DP and SW-DP without NJTRST. Frees PB4.
   FullWithoutNjtrst = AFIO_MAPR_SWJ_CFG_NOJNTRST,
   // JTAG-DP off, SW-DP on. Frees PA15, PB3 and PB4. SWD on PA13/PA14
   // continues to work.
   SwdOnly = AFIO_MAPR_SWJ_CFG_JTAGDISABLE,
   // JTAG-DP and SW-DP off. Frees PA13 and PA14 too. A debugger can then
   // only connect under reset.
   Disabled = AFIO_MAPR_SWJ_CFG_DISABLE,
};

void setSwjConfig(SwjConfig config);

/**
 * @brief Writes remap bits in AFIO_MAPR.
 * @param mask the remap field(s) to change, e.g. AFIO_MAPR_USART3_REMAP.
 *        Must not include AFIO_MAPR_SWJ_CFG; use setSwjConfig() for that.
 * @param value the new field value(s), within @p mask.
 */
void setRemap(uint32_t mask, uint32_t value);

// USART1: TX/RX on PA9/PA10 (default) or PB6/PB7 (remap).
inline void remapUsart1(bool remap) {
   setRemap(AFIO_MAPR_USART1_REMAP, remap ? AFIO_MAPR_USART1_REMAP : 0);
}

// USART2: TX/RX/CTS/RTS/CK on PA2/PA3/PA0/PA1/PA4 (default) or
// PD5/PD6/PD3/PD4/PD7 (remap).
inline void remapUsart2(bool remap) {
   setRemap(AFIO_MAPR_USART2_REMAP, remap ? AFIO_MAPR_USART2_REMAP : 0);
}

enum class Usart3Remap : uint32_t {
   // TX/RX/CK/CTS/RTS on PB10/PB11/PB12/PB13/PB14
   None = AFIO_MAPR_USART3_REMAP_NOREMAP,
   // TX/RX/CK on PC10/PC11/PC12, CTS/RTS on PB13/PB14
   Partial = AFIO_MAPR_USART3_REMAP_PARTIALREMAP,
   // TX/RX/CK/CTS/RTS on PD8/PD9/PD10/PD11/PD12
   Full = AFIO_MAPR_USART3_REMAP_FULLREMAP,
};

inline void remapUsart3(Usart3Remap remap) {
   setRemap(AFIO_MAPR_USART3_REMAP, static_cast<uint32_t>(remap));
}

// SPI1: NSS/SCK/MISO/MOSI on PA4/PA5/PA6/PA7 (default) or
// PA15/PB3/PB4/PB5 (remap; needs SwjConfig::SwdOnly or Disabled).
inline void remapSpi1(bool remap) {
   setRemap(AFIO_MAPR_SPI1_REMAP, remap ? AFIO_MAPR_SPI1_REMAP : 0);
}

// I2C1: SCL/SDA on PB6/PB7 (default) or PB8/PB9 (remap).
inline void remapI2c1(bool remap) {
   setRemap(AFIO_MAPR_I2C1_REMAP, remap ? AFIO_MAPR_I2C1_REMAP : 0);
}

} // namespace hw::Afio

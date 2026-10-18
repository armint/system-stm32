#pragma once

#include "utils/Span.h"

#include <cmsis_device.h>
#include <cstdint>

namespace hw::Uid {

constexpr size_t SizeBytes = 12u;
constexpr size_t SizeWords = 3u;

/**
 * @brief Returns a 32-bit word from the 96-bit UID (index 0, 1, or 2).
 */
inline uint32_t word(size_t index) {
#if defined(UID_BASE)
   if(index < SizeWords) {
      const auto *uid = reinterpret_cast<const uint32_t *>(UID_BASE);
      return uid[index];
   }
#endif
   return 0u;
}

/**
 * @brief Reads the full 96-bit UID into a caller-supplied 12-byte span.
 * The bytes are in memory order (little endian).
 *
 * The UID is read with 32-bit loads: an 8-bit load from the read-only flash
 * area takes a bus fault on the STM32H533 (issue #44).
 */
inline void read(hw::span<uint8_t, SizeBytes> dest) {
#if defined(UID_BASE)
   for(size_t i = 0u; i < SizeBytes; ++i) {
      dest[i] = static_cast<uint8_t>(word(i / 4u) >> (8u * (i % 4u)));
   }
#else
   for(size_t i = 0u; i < SizeBytes; ++i) {
      dest[i] = 0u;
   }
#endif
}

/**
 * @brief Generates a pseudo-random seed or hash by XORing the 3 words of the 96-bit UID.
 */
inline uint32_t hash() {
   return word(0) ^ word(1) ^ word(2);
}

} // namespace hw::Uid

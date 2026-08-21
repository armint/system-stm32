#pragma once

#include <cmsis_device.h>
#include <cstdint>
#include <span>

namespace hw {

/**
 * @brief STM32 Unique Device ID (UID) reader.
 *
 * Provides access to the 96-bit (12-byte / 3-word) factory-programmed unique ID.
 */
struct Uid {
   static constexpr size_t SizeBytes = 12u;
   static constexpr size_t SizeWords = 3u;

   /**
    * @brief Returns a 32-bit word from the 96-bit UID (index 0, 1, or 2).
    */
   static inline uint32_t word(size_t index) {
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
    */
   static inline void read(std::span<uint8_t, SizeBytes> dest) {
#if defined(UID_BASE)
      const auto *uid = reinterpret_cast<const uint8_t *>(UID_BASE);
      for(size_t i = 0u; i < SizeBytes; ++i) {
         dest[i] = uid[i];
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
   static inline uint32_t hash() {
      return word(0) ^ word(1) ^ word(2);
   }
};

} // namespace hw

#pragma once

#include <cmsis_device.h>
#include <cstdint>

namespace hw {

class Iwdg {
   public:
   IWDG_TypeDef &iwdg;

   enum class Prescaler : uint32_t {
      Div4 = 0,
      Div8 = 1,
      Div16 = 2,
      Div32 = 3,
      Div64 = 4,
      Div128 = 5,
      Div256 = 6,
   };

   explicit Iwdg(IWDG_TypeDef *base)
      : iwdg(*base) {
   }

   Iwdg(const Iwdg &) = default;
   Iwdg(Iwdg &&) = default;
   Iwdg &operator=(const Iwdg &) = delete;
   Iwdg &operator=(Iwdg &&) = delete;

   void enable() const {
      iwdg.KR = 0xCCCC;
   }

   void reload() const {
      iwdg.KR = 0xAAAA;
   }

   void configure(Prescaler prescaler, uint16_t reloadValue) const {
      iwdg.KR = 0x5555; // Enable write access
      iwdg.PR = static_cast<uint32_t>(prescaler);
      iwdg.RLR = reloadValue & 0xFFF;
      waitReady();
   }

   void waitReady() const {
      while((iwdg.SR & (IWDG_SR_PVU | IWDG_SR_RVU)) != 0) {
      }
   }
};

} // namespace hw

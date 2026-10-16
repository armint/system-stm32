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
#if defined(IWDG_PR_PR_3)
      // STM32H5: 4-bit PR field
      Div512 = 7,
      Div1024 = 8,
#endif
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

   /**
    * @brief Sets the prescaler and the reload value, and waits for the update.
    * @note On STM32H5, call enable() first: the register update needs the IWDG
    * kernel clock (LSI), which enable() starts (RM0481 49.4.4). Else
    * waitReady() does not return while LSI is off.
    */
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

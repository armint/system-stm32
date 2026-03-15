#pragma once

#include <cmsis_device.h>
#include <cstdint>

namespace hw {

class Timer {
   public:
   TIM_TypeDef &tim;

   enum class Channel : uint8_t { Ch1 = 0, Ch2 = 1, Ch3 = 2, Ch4 = 3 };

   explicit Timer(TIM_TypeDef *base)
      : tim(*base) {
   }

   Timer(const Timer &) = default;
   Timer(Timer &&) = default;
   Timer &operator=(const Timer &) = delete;
   Timer &operator=(Timer &&) = delete;

   void enableCounter() const {
      tim.CR1 |= TIM_CR1_CEN;
   }
   void disableCounter() const {
      tim.CR1 &= ~TIM_CR1_CEN;
   }
   bool isUpdateFlag() const {
      return getStatusFlag(TIM_SR_UIF);
   }

   // Individual flag clearing methods (write 1 to clear)
   void clearUpdateFlag() const {
      tim.SR = TIM_SR_UIF;
   }
   void clearCaptureCompare1Flag() const {
      tim.SR = TIM_SR_CC1IF;
   }
   void clearCaptureCompare2Flag() const {
      tim.SR = TIM_SR_CC2IF;
   }
   void clearCaptureCompare3Flag() const {
      tim.SR = TIM_SR_CC3IF;
   }
   void clearCaptureCompare4Flag() const {
      tim.SR = TIM_SR_CC4IF;
   }

#if defined(TIM_SR_TIF)
   void clearOverflowFlag() const {
      tim.SR = TIM_SR_TIF;
   }
#endif

#if defined(TIM_SR_COMIF)
   void clearControlUpdateFlag() const {
      tim.SR = TIM_SR_COMIF;
   }
#endif

#if defined(TIM_SR_BIF)
   void clearBreakFlag() const {
      tim.SR = TIM_SR_BIF;
   }
#endif

   void setUpdateRequestSourceOnlyOverflow(bool enable) const {
      if(enable) {
         tim.CR1 |= TIM_CR1_URS;
      } else {
         tim.CR1 &= ~TIM_CR1_URS;
      }
   }

   void setOnePulseMode(bool single) const {
      if(single) {
         tim.CR1 |= TIM_CR1_OPM;
      } else {
         tim.CR1 &= ~TIM_CR1_OPM;
      }
   }

   void setAutoReloadPreload(bool enable) const {
      if(enable) {
         tim.CR1 |= TIM_CR1_ARPE;
      } else {
         tim.CR1 &= ~TIM_CR1_ARPE;
      }
   }

   void setDirectionDown(bool down) const {
      if(down) {
         tim.CR1 |= TIM_CR1_DIR;
      } else {
         tim.CR1 &= ~TIM_CR1_DIR;
      }
   }

   void generateUpdate() const {
      tim.EGR = TIM_EGR_UG;
   }

   void setPrescaler(uint16_t psc) const {
      tim.PSC = psc;
   }
   uint16_t getPrescaler() const {
      return static_cast<uint16_t>(tim.PSC);
   }

   void setAutoReload(uint32_t arr) const {
      tim.ARR = arr;
   }
   uint32_t getAutoReload() const {
      return tim.ARR;
   }

   void setCounter(uint32_t cnt) const {
      tim.CNT = cnt;
   }
   uint32_t getCounter() const {
      return tim.CNT;
   }

   void enableUpdateDma() const {
      tim.DIER |= TIM_DIER_UDE;
   }
   void disableUpdateDma() const {
      tim.DIER &= ~TIM_DIER_UDE;
   }

   void enableCcDma(Channel channel) const {
      tim.DIER |= (TIM_DIER_CC1DE << static_cast<uint8_t>(channel));
   }
   void disableCcDma(Channel channel) const {
      tim.DIER &= ~(TIM_DIER_CC1DE << static_cast<uint8_t>(channel));
   }

   void enableUpdateInterrupt() const {
      tim.DIER |= TIM_DIER_UIE;
   }
   void disableUpdateInterrupt() const {
      tim.DIER &= ~TIM_DIER_UIE;
   }
   bool isUpdateInterruptEnabled() const {
      return (tim.DIER & TIM_DIER_UIE) != 0;
   }

   void enableCcInterrupt(Channel channel) const {
      tim.DIER |= (TIM_DIER_CC1IE << static_cast<uint8_t>(channel));
   }
   void disableCcInterrupt(Channel channel) const {
      tim.DIER &= ~(TIM_DIER_CC1IE << static_cast<uint8_t>(channel));
   }

   bool getStatusFlag(uint32_t flag) const {
      return (tim.SR & flag) == flag;
   }
   void clearStatusFlag(uint32_t flag) const {
      tim.SR &= ~flag;
   }

   void clearAllStatusFlags() const {
      uint32_t flags = TIM_SR_UIF | TIM_SR_CC1IF | TIM_SR_CC2IF | TIM_SR_CC3IF | TIM_SR_CC4IF | TIM_SR_TIF;
#if defined(TIM_SR_COMIF)
      flags |= TIM_SR_COMIF;
#endif
#if defined(TIM_SR_BIF)
      flags |= TIM_SR_BIF;
#endif
      tim.SR &= ~flags;
   }

   // Output Compare mode configuration
   enum class OcMode : uint32_t {
      Frozen = 0,
      ActiveOnMatch = TIM_CCMR1_OC1M_0,
      InactiveOnMatch = TIM_CCMR1_OC1M_1,
      Toggle = TIM_CCMR1_OC1M_0 | TIM_CCMR1_OC1M_1,
      ForceInactive = TIM_CCMR1_OC1M_2,
      ForceActive = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_0,
      Pwm1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1,
      Pwm2 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_0,
   };

   void setOcMode(Channel channel, OcMode mode) const {
      auto ch = static_cast<uint8_t>(channel);
      // OC1M bits are at [6:4] in CCMR1 for Ch1, [14:12] for Ch2
      // CCMR2 holds Ch3 and Ch4
      volatile uint32_t &ccmr = (ch < 2) ? tim.CCMR1 : tim.CCMR2;
      uint32_t shift = (ch & 1) ? 8 : 0;
      uint32_t mask = TIM_CCMR1_OC1M << shift;
      ccmr = (ccmr & ~mask) | (static_cast<uint32_t>(mode) << shift);
   }

   void enableOcPreload(Channel channel) const {
      auto ch = static_cast<uint8_t>(channel);
      volatile uint32_t &ccmr = (ch < 2) ? tim.CCMR1 : tim.CCMR2;
      uint32_t shift = (ch & 1) ? 8 : 0;
      ccmr |= (TIM_CCMR1_OC1PE << shift);
   }

   void disableOcPreload(Channel channel) const {
      auto ch = static_cast<uint8_t>(channel);
      volatile uint32_t &ccmr = (ch < 2) ? tim.CCMR1 : tim.CCMR2;
      uint32_t shift = (ch & 1) ? 8 : 0;
      ccmr &= ~(TIM_CCMR1_OC1PE << shift);
   }

   void setTriggerOutput(uint32_t trgo) const {
      tim.CR2 = (tim.CR2 & ~TIM_CR2_MMS) | trgo;
   }

   void disableMasterSlaveMode() const {
      tim.SMCR &= ~TIM_SMCR_MSM;
   }

   void enableChannel(Channel channel) const {
      tim.CCER |= (TIM_CCER_CC1E << (static_cast<uint8_t>(channel) * 4));
   }

   void disableChannel(Channel channel) const {
      tim.CCER &= ~(TIM_CCER_CC1E << (static_cast<uint8_t>(channel) * 4));
   }

   void setCompare(Channel channel, uint32_t value) const {
      (&tim.CCR1)[static_cast<uint8_t>(channel)] = value;
   }

   uint32_t getCompare(Channel channel) const {
      return (&tim.CCR1)[static_cast<uint8_t>(channel)];
   }
};

} // namespace hw

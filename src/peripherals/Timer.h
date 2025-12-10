#pragma once

#include <cmsis_device.h>
#include <cstdint>

// Lightweight peripheral access wrapper for general-purpose/advanced STM32 timers
// Mirrors the style and intent of Uart.h: thin, constexpr-free, direct register access.

class Timer {
 public:
  TIM_TypeDef &tim;

  // Capture/Compare channel selector
  enum class Channel : uint8_t { Ch1 = 0, Ch2 = 1, Ch3 = 2, Ch4 = 3 };

  explicit Timer(TIM_TypeDef *base)
      : tim(*base) {}

  // =============================================================
  // Basic control
  // =============================================================
  void enableCounter() const { tim.CR1 |= TIM_CR1_CEN; }
  void disableCounter() const { tim.CR1 &= ~TIM_CR1_CEN; }

  void setUpdateRequestSourceOnlyOverflow(bool enable) const {
#ifdef TIM_CR1_URS
    if (enable) {
      tim.CR1 |= TIM_CR1_URS; // Only counter overflow/underflow generates update
    } else {
      tim.CR1 &= ~TIM_CR1_URS; // Any UG or setting generates update
    }
#endif
  }

  void setOnePulseMode(bool single) const {
#ifdef TIM_CR1_OPM
    if (single) {
      tim.CR1 |= TIM_CR1_OPM;
    } else {
      tim.CR1 &= ~TIM_CR1_OPM;
    }
#endif
  }

  void setAutoReloadPreload(bool enable) const {
#ifdef TIM_CR1_ARPE
    if (enable) {
      tim.CR1 |= TIM_CR1_ARPE;
    } else {
      tim.CR1 &= ~TIM_CR1_ARPE;
    }
#endif
  }

  void setDirectionDown(bool down) const {
#ifdef TIM_CR1_DIR
    if (down) {
      tim.CR1 |= TIM_CR1_DIR;
    } else {
      tim.CR1 &= ~TIM_CR1_DIR;
    }
#endif
  }

  // Generate update event (load PSC/ARR when ARPE is set)
  void generateUpdate() const { tim.EGR = TIM_EGR_UG; }

  // =============================================================
  // Timing configuration
  // =============================================================
  void setPrescaler(uint16_t psc) const { tim.PSC = psc; }
  uint16_t getPrescaler() const { return static_cast<uint16_t>(tim.PSC); }

  void setAutoReload(uint32_t arr) const { tim.ARR = arr; }
  uint32_t getAutoReload() const { return tim.ARR; }

  void setCounter(uint32_t cnt) const { tim.CNT = cnt; }
  uint32_t getCounter() const { return tim.CNT; }

  // =============================================================
  // DMA enables (if available)
  // =============================================================
  void enableUpdateDma() const {
#ifdef TIM_DIER_UDE
    tim.DIER |= TIM_DIER_UDE;
#endif
  }
  void disableUpdateDma() const {
#ifdef TIM_DIER_UDE
    tim.DIER &= ~TIM_DIER_UDE;
#endif
  }

  void enableCcDma(Channel channel) const {
#ifdef TIM_DIER_CC1DE
    tim.DIER |= (TIM_DIER_CC1DE << static_cast<uint8_t>(channel));
#endif
  }
  void disableCcDma(Channel channel) const {
#ifdef TIM_DIER_CC1DE
    tim.DIER &= ~(TIM_DIER_CC1DE << static_cast<uint8_t>(channel));
#endif
  }

  // =============================================================
  // Interrupts
  // =============================================================
  void enableUpdateInterrupt() const { tim.DIER |= TIM_DIER_UIE; }
  void disableUpdateInterrupt() const { tim.DIER &= ~TIM_DIER_UIE; }
  bool isUpdateInterruptEnabled() const { return (tim.DIER & TIM_DIER_UIE) != 0; }

  void enableCcInterrupt(Channel channel) const {
#ifdef TIM_DIER_CC1IE
    tim.DIER |= (TIM_DIER_CC1IE << static_cast<uint8_t>(channel));
#endif
  }
  void disableCcInterrupt(Channel channel) const {
#ifdef TIM_DIER_CC1IE
    tim.DIER &= ~(TIM_DIER_CC1IE << static_cast<uint8_t>(channel));
#endif
  }

  // =============================================================
  // Status flags
  // =============================================================
  bool getStatusFlag(uint32_t flag) const { return (tim.SR & flag) == flag; }
  void clearStatusFlag(uint32_t flag) const { tim.SR &= ~flag; }

  void clearAllStatusFlags() const {
    // Clear common flags present on most timers
    uint32_t mask = 0;
#ifdef TIM_SR_UIF
    mask |= TIM_SR_UIF;
#endif
#ifdef TIM_SR_CC1IF
    mask |= TIM_SR_CC1IF;
#endif
#ifdef TIM_SR_CC2IF
    mask |= TIM_SR_CC2IF;
#endif
#ifdef TIM_SR_CC3IF
    mask |= TIM_SR_CC3IF;
#endif
#ifdef TIM_SR_CC4IF
    mask |= TIM_SR_CC4IF;
#endif
#ifdef TIM_SR_TIF
    mask |= TIM_SR_TIF; // Trigger interrupt flag
#endif
#ifdef TIM_SR_COMIF
    mask |= TIM_SR_COMIF; // Commutation flag (advanced timers)
#endif
#ifdef TIM_SR_BIF
    mask |= TIM_SR_BIF; // Break flag (advanced timers)
#endif
    if (mask)
      tim.SR &= ~mask;
  }

  bool isUpdateFlag() const {
#ifdef TIM_SR_UIF
    return getStatusFlag(TIM_SR_UIF);
#else
    return false;
#endif
  }

  // =============================================================
  // Capture/Compare channels
  // =============================================================
  void enableChannel(Channel channel) const {
#ifdef TIM_CCER_CC1E
    tim.CCER |= (TIM_CCER_CC1E << (static_cast<uint8_t>(channel) * 4));
#endif
  }

  void disableChannel(Channel channel) const {
#ifdef TIM_CCER_CC1E
    tim.CCER &= ~(TIM_CCER_CC1E << (static_cast<uint8_t>(channel) * 4));
#endif
  }

  void setCompare(Channel channel, uint32_t value) const {
    (&tim.CCR1)[static_cast<uint8_t>(channel)] = value;
  }

  uint32_t getCompare(Channel channel) const {
    return (&tim.CCR1)[static_cast<uint8_t>(channel)];
  }
};

#pragma once

#include <cmsis_device.h>
#include <cstdint>

class Adc {
   public:
   struct TemperatureSensorCalibration {
      // Factory calibration data (Vref+ = 3.0 V, RM0453 §3.13.2)
      static uint16_t cal1() {
         return *reinterpret_cast<const volatile uint16_t *>(0x1FFF75A8UL);
      } // 30 °C
      static uint16_t cal2() {
         return *reinterpret_cast<const volatile uint16_t *>(0x1FFF75CAUL);
      } // 130 °C
      static constexpr int32_t temp1 = 30;
      static constexpr int32_t temp2 = 130;
      static constexpr uint32_t vrefAnalog = 3000; // mV
   };
};

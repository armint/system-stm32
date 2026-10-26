# STM32F1

## ADC

The STM32F1 `Adc` does not match the interface of the other families,
because the F1 ADC has a different register block. It offers single
software-triggered conversions, `ADCPRE` setup (ADCCLK at most 14 MHz),
calibration and a VDDA measurement from Vrefint. Vrefint has no factory
calibration value on F1. The datasheet gives 1.16 to 1.24 V (up to 85 °C)
and up to 1.26 V (105 °C). So VDDA from Vrefint has an error of about
±3.3 % (+5 % at the highest temperatures), plus the ADC error. In the
worst case this is about ±110 mV at 3.3 V and ±120 mV at 3.6 V.

## Clock: 72 MHz from HSE

`Rcc::setClock72MhzHSE(hseHz)` sets the STM32F103 to its maximum clock
speeds. The source is the HSE crystal through the PLL:

| Clock | Frequency |
|-------|-----------|
| SYSCLK, AHB (HCLK), APB2 | 72 MHz |
| APB1 | 36 MHz |
| ADC | 12 MHz (APB2 / 6) |
| USB | 48 MHz (PLL / 1.5) |

The function calculates the PLL setting from the crystal frequency
`hseHz`. It accepts any crystal from 4 MHz to 16 MHz that gives exactly
72 MHz, for example 8 MHz (x 9), 12 MHz (x 6) or 16 MHz (/ 2, x 9).
`hseHz` defaults to `HSE_VALUE`. Set the crystal frequency with
`SYSTEM_STM32_HSE_VALUE`, so that `SystemCoreClockUpdate()` uses the same
value.

The function returns `false` in these cases:

- No PLL setting gives exactly 72 MHz from `hseHz`.
- HSE does not start within approximately 100 ms.
- The PLL does not lock.

After a failure, SYSCLK stays on HSI (8 MHz). The function does not write
global variables, so you can call it before the DATA and BSS
initialization. Call `SystemCoreClockUpdate()` after it.

For USB, call the function before `Rcc::enableUSBClock()`. The function sets
`RCC_CFGR.USBPRE`, and this bit must be valid before the USB clock is
enabled. The bit cannot change while the USB clock is enabled (RM0008
section 7.3.2).

```cpp
#include "peripherals/Rcc.h"

int main() {
   if(!Rcc::setClock72MhzHSE()) {
      // SYSCLK is HSI (8 MHz). USB does not work without HSE.
   }
   SystemCoreClockUpdate();
   // ...
}
```

## Backup registers

`Bkp.h` (STM32F1 only) gives access to backup data register 1
(`BKP_DR1`). The backup registers keep their content over a system reset.
With VBAT supplied, they also keep it over a power loss. A typical use is
a flag for the bootloader entry that survives `NVIC_SystemReset()`.

1. Call `hw::Bkp::enable()`. It enables the PWR and BKP clocks and sets
   `PWR_CR.DBP`.
2. Use `hw::Bkp::readDR1()` and `hw::Bkp::writeDR1()`.

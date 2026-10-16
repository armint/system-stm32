# Agent Instructions for system-stm32

## Purpose
This library provides the Hardware Abstraction Layer (HAL), startup code, and peripheral drivers for STM32 microcontrollers. It uses a single-repository, build-time separation architecture to support multiple STM32 families (e.g., STM32L0, STM32C0) without relying on `#ifdef` macros for hardware variations.

## Key Features
- **Multi-Family Support**: Uses the `STM32_FAMILY` CMake variable to automatically select correct CMSIS headers and family-specific source files.
- **Startup**: Linker scripts and startup code (`startup.cpp`, `vectors_*.c`).
- **Peripherals**: Drivers for ADC, DMA, EXTI, GPIO, USART, Clock control and Timers, and the Clock Recovery System (CRS).
- **USB clocking**: USB needs a 48 MHz clock. On STM32L0 and STM32G0B1/G0C1 this is HSI48, trimmed by the CRS from USB SOF. `Rcc::enableHSI48()` and `Rcc::setUSBClockSource()` hide the register differences between the families (L0: `RCC_CRRCR`, `RCC_CCIPR`, `SYSCFG_CFGR3.ENREF_HSI48`; G0: `RCC_CR`, `RCC_CCIPR2`; H5: `RCC_CR`, `RCC_CCIPR4`). `peripherals/Crs.h` is shared, because the CRS registers are the same. See the README section "USB clock: HSI48 and CRS".
- **USB device**: `hw::UsbDevice` is family-specific. Today `src/families/stm32l0/UsbDevice.{h,cpp}` and the shared core `src/peripherals/usbfs/UsbDeviceCore.{h,cpp}` (F1, with `src/families/stm32f1/UsbFsFamily.h`) exist. The core is a copy of the L0 driver (512-byte packet memory at a 32-bit stride, external D+ pull-up through a board callback, `USB_LP_CAN1_RX0_IRQn`); keep fixes in the shared logic in step in both copies. A future G0B1 driver goes to `src/families/stm32g0/` with the same public interface and the shared types in `peripherals/UsbDeviceTypes.h`. The driver is packet-level and must not depend on libArT. The interrupt handler only decodes events and reports them through the callback; no protocol logic in the ISR. See the README section "USB device driver".
- **System**: Delay functions, interrupt handling, and hardware initialization.
- **STM32H5**: a skeleton family (Cortex-M33, soft float, TrustZone off). Its peripherals are close to L0/G0/L4, not to F1: start from the L0/G0 drivers when you port one. `_H5_EXCLUDE` in `CMakeLists.txt` lists the drivers that are not ported yet. The H5 RCC is in `src/families/stm32h5/RccH5.{h,cpp}`; `peripherals/Rcc.h` includes it when `RCC_CFGR1_SW` exists. The H5 GPDMA is in `src/families/stm32h5/GpdmaH5.{h,cpp}`; `peripherals/Dma.h` includes it when `GPDMA1_BASE` exists. Add new H5 drivers to `tests/stm32h5/main.cpp`. See the README section "STM32H5".

## Guidelines
- **Peripheral Drivers**: Shared or general peripheral interfaces are located in `src/peripherals/`.
- **Family-Specific Code**: Highly hardware-dependent code (like `Adc`, `Rcc`, etc.) must be placed in `src/families/stm32<family>/` (e.g., `src/families/stm32l0/`). Ensure that the class implementations expose the same generic interface (e.g., `#include "Adc.h"`). CMake automatically adds the active family's directory to the target's include directories, so `#include "Adc.h"` seamlessly resolves to the correct hardware-specific header.
- **Avoid `#ifdef` for Families**: Do not use `#ifdef STM32L0` inside common `.cpp` files to handle whole family differences. Instead, rely on CMake-driven file selection and place family-specific files in the `src/families/` directory. However, using `#ifdef` to check for the availability of specific peripheral features (e.g., `#if defined(USART_PRESC_PRESCALER)`) is perfectly acceptable and encouraged for maintaining shared peripheral drivers.
- When modifying or adding peripheral support, ensure it matches the hardware specification of the targeted STM32 MCUs.
- Be mindful of the limited resources (Flash/RAM) on these microcontrollers.
- **Coding Style**: Adhere strictly to the project's `.clang-format` rules. When adding new files, match the naming conventions (e.g., PascalCase for classes, specific prefixing for members) found in surrounding files.
- Document any new functions or changes with clear comments.
- **Memory & Error Handling**: Heap memory usage is strictly not allowed. Avoid exceptions. Handle errors gracefully using boolean status codes for fallible initializations or `assert()` for unrecoverable state errors.

## Verification
- To verify compilation, build the library using the standard CMake workflow targeting an ARM toolchain, or verify it compiles when linked against dependent projects (e.g., `universal-oven-controller`).

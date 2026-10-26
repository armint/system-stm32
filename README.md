# system-stm32

system-stm32 is a C++ hardware layer for STM32 microcontrollers. It
contains the start-up code, the interrupt vector tables and register-level
peripheral drivers. It does not use the ST HAL or the LL drivers. It uses
only the CMSIS device headers.

## Purpose

The library is the lowest layer of a set of bare-metal firmware projects.
It gives these projects one C++ interface to the hardware:

- Small, direct drivers. Many functions are inline register accesses. The
  library uses no heap and no exceptions.
- One code base for many STM32 families. CMake selects the family code at
  configure time. Shared sources do not contain `#ifdef STM32xx` for
  family differences.
- Drivers that follow the hardware. When a family has a different
  peripheral (for example the STM32H5 SPI or ADC), its driver exposes the
  full function set of that peripheral. It does not copy the interface of
  an older family.

Higher layers are not in this library. For example, the USB driver sends
and receives packets only. Descriptors, standard requests and USB classes
are the job of the firmware above it.

## Supported families

| `STM32_FAMILY` | Core | Status |
|---|---|---|
| `STM32L0` | Cortex-M0+ | Supported. |
| `STM32G0` | Cortex-M0+ | Supported. CI builds STM32G071 and STM32G0B1. |
| `STM32WL` | Cortex-M4 | Supported. `GpioInterrupt` is not ported. |
| `STM32F1` | Cortex-M3 | Supported (STM32F103). EXTI, I2C and RTC are not ported. |
| `STM32H5` | Cortex-M33 | Supported (STM32H533). EXTI and I2C are not ported. See [STM32H5](docs/stm32h5.md). |
| `STM32C0` | Cortex-M0+ | Not complete. The CI job is known to fail. |

## Drivers

| Area | Drivers |
|---|---|
| Start-up | Reset handler, C/C++ runtime start, weak exception handlers, vector table per family |
| Clocks and power | `Rcc` (with clock recipes), `Crs`, `Pwr`, `SysTick`, `Iwdg`, `Dbg` |
| GPIO | `GpioPin`, `FastPin`, `Exti`, `GpioInterrupt` |
| Communication | `Usart`, `Spi`, `I2c`, USB full-speed device (`UsbDevice`) |
| Analog and timing | `Adc`, `Timer`, `Rtc`, `delaymS()` / `delayuS()` |
| Other | `Dma` (GPDMA on STM32H5), `Uid`, `Interrupt` (C++ member functions as interrupt handlers) |

## Quick start

Add the repository as a git submodule. Then add it to your CMake project:

```cmake
set(STM32_FAMILY STM32G0)
set(STM32_DEVICE STM32G0B1xx)
add_subdirectory(dependencies/system-stm32)
target_link_libraries(my-firmware PRIVATE system-stm32)
```

CMake gets the CMSIS headers for the family with `FetchContent`. You need
CMake 3.20 or later, an `arm-none-eabi-gcc` toolchain and C++17 or later.
For all options, see [Building and using system-stm32](docs/building.md).

## Repository layout

| Path | Contents |
|---|---|
| `src/peripherals/` | Drivers that more than one family shares. They can use `#if defined(<register bit>)` for small differences in a peripheral. |
| `src/peripherals/usbfs/` | The shared USB full-speed device core. |
| `src/families/stm32<family>/` | Family-specific drivers and the vector table. CMake adds only the directory of the active family to the include path. Thus `#include "Adc.h"` gives the correct driver. |
| `src/bare-metal/` | The runtime start and the default exception handlers. `SYSTEM_STM32_EXCLUDE_STARTUP=ON` removes them for an RTOS. |
| `src/cmsis-plus/` | Cortex-M exception-handler declarations. |
| `src/delaybusywait*.cpp` | The delay functions. |
| `src/utils/` | Interrupt binding (`Interrupt`), `GpioInterrupt`, `hw::span` and helpers. |
| `ldscripts/` | Memory linker scripts for the parts that the test images use. |
| `tests/host/` | Unit tests that run on the build machine with fake registers. |
| `tests/stm32f1/`, `tests/stm32h5/` | Test images for the STM32F1 and the STM32H5. |
| `svd/` | SVD register descriptions, for reference and debug tools. |
| `cmake/` | The ARM toolchain file and the package configuration. |

## Documentation

| Document | Contents |
|---|---|
| [Building and using system-stm32](docs/building.md) | Requirements, CMake variables, C++ standard, `hw::span`, linker scripts |
| [Bare-metal start-up and an external RTOS](docs/startup-and-rtos.md) | `src/bare-metal/`, start-up hooks, use with µOS++ |
| [Tests and CI](docs/testing.md) | CI jobs, test images, host tests |
| [GPIO and FastPin](docs/gpio.md) | STM32F1 GPIO, interrupt safety, fast direction switch |
| [USART](docs/usart.md) | Frame format, STM32F1 USART v1 |
| [USB](docs/usb.md) | HSI48 and CRS, the USB device driver |
| [STM32F1](docs/stm32f1.md) | ADC, 72 MHz clock, backup registers |
| [STM32H5](docs/stm32h5.md) | H5 port: GPDMA, clocks, USART, SPI v2, ADC |

## Contributing

All work goes to the `dev` branch through merge requests. The project has
no releases yet. Use a commit of `dev` in your submodule. For the code
format and the git blame setting, see [CONTRIBUTING.md](CONTRIBUTING.md).

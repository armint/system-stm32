# system-stm32

Hardware abstraction layer, startup code, and peripheral drivers for STM32
microcontrollers, shared across several embedded firmware projects. It
supports multiple STM32 families from a single repository, selecting
family-specific code at CMake configure time instead of littering shared
sources with `#ifdef STM32xx`.

## Supported families

| `STM32_FAMILY` | CMSIS device package | Core       | CI status |
|----------------|-----------------------|------------|-----------|
| `STM32L0`      | cmsis\_device\_l0 v1.9.4 | Cortex-M0+ | built in CI |
| `STM32G0`      | cmsis\_device\_g0 v1.4.4 | Cortex-M0+ | built in CI |
| `STM32WL`      | cmsis\_device\_wl v1.4.0 | Cortex-M4  | built in CI |
| `STM32C0`      | cmsis\_device\_c0 v1.0.0 | Cortex-M0+ | known-broken, non-blocking CI job (see `.gitlab-ci.yml`) |

CI (`.gitlab-ci.yml`) builds the `system-stm32` object library standalone
for every supported family on each push, since that's the only thing that
catches family-specific regressions (a change that only compiles for the
one family a downstream project happens to use).

## Requirements

- CMake >= 3.20
- An `arm-none-eabi-gcc` toolchain (a ready-made toolchain file is provided
  at `cmake/gcc_arm.cmake`)
- C11 / C++23

CMSIS device headers and CMSIS core are fetched automatically via
`FetchContent` for the selected family; nothing needs to be vendored.

## Using it

Add as a git submodule and `add_subdirectory()` it (this is how the
downstream projects mentioned below consume it), or `install()` it and
`find_package(system-stm32)`. Relevant CMake variables:

- `STM32_FAMILY` — one of `STM32L0`, `STM32C0`, `STM32G0`, `STM32WL`.
  Defaults to `STM32L0`.
- `STM32_DEVICE` — the CMSIS device define, e.g. `STM32WLE5xx`.
- `SYSTEM_STM32_EXCLUDE_STARTUP` — see [Bare-metal startup vs. an external
  RTOS](#bare-metal-startup-vs-an-external-rtos) below. Defaults to `OFF`.

The library is added as the `system-stm32` CMake target (an `OBJECT`
library) and, once installed, as `art::system-stm32`.

## Directory layout

- `src/peripherals/` — peripheral driver interfaces shared across families
  (`Rcc`, `GpioPin`, `Usart`, `I2c`, `Dma`, `Timer`, `Exti`, `Spi`, `Rtc`,
  `Iwdg`, `Dbg`). `#ifdef` on specific register/feature availability (e.g.
  `#if defined(USART_PRESC_PRESCALER)`) is fine here; `#ifdef STM32xx` for
  whole-family differences is not — that belongs below.
- `src/families/<family>/` — family-specific implementations (currently
  `Adc`, and the interrupt vector table in `vectors_stm32<family>xx.c`)
  exposing the same interface as their `src/peripherals/` counterpart
  where one exists. CMake adds only the active family's directory to the
  include path, so `#include "Adc.h"` resolves to the right one
  automatically. `Rcc` and `I2c` are the exception: their STM32WL
  differences are small enough to handle with in-file `#if defined(...)`
  branches instead, so they stay in `src/peripherals/` unconditionally
  (see the comment next to their exclusion in `CMakeLists.txt`).
- `src/utils/` — interrupt vector-table relocation and C++ member-function
  binding (`Interrupt`), GPIO interrupt dispatch.
- `src/startup-hooks.h` / `src/initialize-hardware.cpp` — the weak
  `os_startup_initialize_hardware_early()` / `os_startup_initialize_hardware()`
  hooks. Not part of `src/bare-metal/`: unlike the rest of the startup
  path, these run and are useful even with an RTOS (µOS++ IIIe calls
  the same two hooks from its own startup), so a consumer can override
  just the hardware bring-up without pulling in the whole bare-metal
  runtime.
- `src/cmsis-plus/` — Cortex-M exception-handler declarations
  (`Reset_Handler`, `HardFault_Handler`, ... and the exception stack frame
  layout). Needed by the always-built per-family vector table regardless
  of which startup path is active, so it stays outside `src/bare-metal/`.
- `src/bare-metal/` — see below.
- `svd/` — SVD register descriptions, for reference/tooling use.

## Bare-metal startup vs. an external RTOS

Most of this library — peripheral drivers, interrupt binding, the vector
table — is useful whether or not the application runs an RTOS. A small
part of it is not: the C/C++ runtime entry point and the default exception
handlers only make sense when *nothing else* provides them.

That part lives in `src/bare-metal/`:

- `startup.cpp` — `_start()`: copies `.data`, zeroes `.bss`, runs the
  C++ static constructors, and calls `main()`. The weak default
  `Reset_Handler()` in `exception-handlers.c` below calls this on reset;
  nothing else reaches it, so without an RTOS supplying its own
  `Reset_Handler()`, this is the whole of the application's runtime
  entry point.
- `assert.c` — `__assert_func()` / `assert_failed()`, reporting via
  `trace_printf()` (see below) or semihosting before aborting.
- `exception-handlers.c` — weak default `Reset_Handler`, `NMI_Handler`,
  `HardFault_Handler`, and friends, plus an unconditionally-defined
  (not weak, not `TRACE`-gated) `dump_exception_stack()` for a register
  dump on hard faults.
- `trace_compat.{h,cpp}` — weak no-op `trace_printf()` / `trace_flush()`,
  satisfying `exception-handlers.c`'s trace calls when nothing else (no
  RTOS, no ITU/semihosting backend) defines them.

An RTOS with its own startup sequence — this repo's downstream consumers
use [µOS++ IIIe](https://github.com/micro-os-plus) — provides its own
versions of all of the above, generally as weak symbols too, as a
fallback. Building both copies into the same firmware works for the
*weak* symbols (the RTOS's copy simply wins), but **not** for
`dump_exception_stack()`: this library always defines it non-weak, and
µOS++ also defines its own non-weak `dump_exception_stack()` whenever the
consumer defines the `TRACE` macro — so the moment both land in the same
firmware, that's a hard multiple-definition link error. Setting
`SYSTEM_STM32_EXCLUDE_STARTUP=ON` (below) avoids it, by not building
`src/bare-metal/` — including this file — at all.

Set `SYSTEM_STM32_EXCLUDE_STARTUP=ON` to leave `src/bare-metal/` out of
the build entirely when an external RTOS supplies its own startup and
exception handling:

```cmake
set(SYSTEM_STM32_EXCLUDE_STARTUP ON)
add_subdirectory(path/to/system-stm32)
```

`sentinode-ht` (STM32WLE5JC + µOS++ IIIe) builds this way. Leave the
option `OFF` (the default) for a bare-metal application with no RTOS,
where `src/bare-metal/` is the only thing providing a runtime at all.

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
| `STM32G0`      | cmsis\_device\_g0 v1.4.4 | Cortex-M0+ | built in CI (STM32G071 and STM32G0B1) |
| `STM32WL`      | cmsis\_device\_wl v1.4.0 | Cortex-M4  | built in CI |
| `STM32F1`      | cmsis\_device\_f1 v4.3.5 | Cortex-M3  | built in CI (STM32F103xB); EXTI, I2C and RTC not ported; ADC and USART in `families/stm32f1` (see below) |
| `STM32H5`      | cmsis\_device\_h5 v1.7.0 | Cortex-M33 | built in CI (STM32H533xx); start-up, vectors, GPIO, delay, RCC (250 MHz), ICACHE, CRS. USART, SPI, DMA, EXTI and I2C not ported (see [STM32H5](#stm32h5)) |
| `STM32C0`      | cmsis\_device\_c0 v1.0.0 | Cortex-M0+ | known-broken, non-blocking CI job (see `.gitlab-ci.yml`) |

CI (`.gitlab-ci.yml`) builds the `system-stm32` object library standalone
for every supported family on each push, since that's the only thing that
catches family-specific regressions (a change that only compiles for the
one family a downstream project happens to use).

Each family builds in two legs:

- `build`: C++23. The job fails if CMake fetches span-compat. Thus
  `hw::span` is `std::span`.
- `build-cpp17`: C++17. First, a compile with `-Wpedantic -Werror
  -fno-exceptions -fno-rtti` rejects C++20 constructs. Then the normal
  library build runs. The job fails if CMake does not use span-compat with
  `SPAN_COMPAT_USE_STD_SPAN=0`.

The STM32F1 and STM32H5 test images link at both standards. The C0 job also runs at
both standards. The C++17 jobs clone the private span-compat repository
with the CI job token. Thus span-compat must allow job tokens from this
project.

## Requirements

- CMake >= 3.20
- An `arm-none-eabi-gcc` toolchain (a ready-made toolchain file is provided
  at `cmake/gcc_arm.cmake`)
- C11 / C++17 or later. Below C++20, the build also needs span-compat (see
  [Span type](#span-type)).

### C++ standard

The target requires C++17 (`cxx_std_17`). It does not force a later
standard on its consumers. The consumer selects the standard with
`CMAKE_CXX_STANDARD`. A value that the superproject sets before
`add_subdirectory()` also applies to this library. A standalone build of
this library uses C++23 if you do not set `CMAKE_CXX_STANDARD`. To build
it at C++17, give `-D CMAKE_CXX_STANDARD=17`.

CMSIS device headers and CMSIS core are fetched automatically via
`FetchContent` for the selected family; nothing needs to be vendored.
The target exports them as `SYSTEM` include directories. Thus warnings in
these vendor headers do not stop a `-Wpedantic -Werror` build. For example,
`core_cm33.h` has an anonymous struct, which ISO C++ does not allow.

### Span type

The public interface uses one span type: `hw::span` (`src/utils/Span.h`).
`UsbDevice`, `Uid` and `Rcc` use it. Use `hw::span` in your code, not
`std::span` or `span_compat::span`.

- At C++20 and later, `hw::span` is `std::span`. CMake adds no other
  library.
- Below C++20, the standard library has no `std::span`. Then CMake gets
  [span-compat](https://gitlab.vandertogt.org/embedded-libraries/span-compat)
  with `FetchContent` and links it. It also defines
  `SYSTEM_STM32_USE_SPAN_COMPAT`. Then `hw::span` is `span_compat::span`.

CMake selects the type from `CMAKE_CXX_STANDARD`. If `CMAKE_CXX_STANDARD`
is not set, CMake also uses span-compat.

All translation units of one program must see the same span type. The
firmware can build at C++23 and this library at C++17. Thus, when CMake
fetches span-compat, it sets `SPAN_COMPAT_USE_STD_SPAN` to `OFF`. Then
`span_compat::span` is the own implementation at every standard. A value
that the superproject sets before is kept.

If the superproject already has the target `span-compat::span-compat`,
CMake uses that target. If the superproject mixes C++ standards, it must
set `SPAN_COMPAT_USE_STD_SPAN` to `OFF`. The span-compat README tells how
to select its type for the full build. Below C++20, the installed package
needs an installed span-compat package.

## Using it

Add as a git submodule and `add_subdirectory()` it (this is how the
downstream projects mentioned below consume it), or `install()` it and
`find_package(system-stm32)`. Relevant CMake variables:

- `STM32_FAMILY` — one of `STM32L0`, `STM32C0`, `STM32G0`, `STM32WL`,
  `STM32F1`.
  Defaults to `STM32L0`.
- `STM32_DEVICE` — the CMSIS device define, e.g. `STM32WLE5xx`. Use
  `STM32F103xB` for both STM32F103x8 and STM32F103xB. CMSIS has no
  separate define for the 64 KB part.
- `SYSTEM_STM32_EXCLUDE_STARTUP` — see [Bare-metal startup vs. an external
  RTOS](#bare-metal-startup-vs-an-external-rtos) below. Defaults to `OFF`.
- `SYSTEM_STM32_HSE_VALUE` — the HSE crystal frequency in Hz, for example
  `12000000`. CMake gives it to the target and to its consumers as the
  `HSE_VALUE` define. The CMSIS `SystemCoreClockUpdate()` and the `Rcc`
  clock recipes both use it. A wrong value gives a wrong `SystemCoreClock`
  after a switch to HSE or to the HSE PLL. Empty (the default) keeps the
  family default. On `STM32F1` the default is 8 MHz.

The library is added as the `system-stm32` CMake target (an `OBJECT`
library) and, once installed, as `art::system-stm32`.

## Directory layout

- `src/peripherals/` — peripheral driver interfaces shared across families
  (`Rcc`, `Crs`, `GpioPin`, `Usart`, `I2c`, `Dma`, `Timer`, `Exti`, `Spi`,
  `Rtc`, `Iwdg`, `Dbg`). `#ifdef` on specific register/feature availability (e.g.
  `#if defined(USART_PRESC_PRESCALER)`) is fine here; `#ifdef STM32xx` for
  whole-family differences is not — that belongs below. `GpioInterrupt` in
  `src/utils/` is not yet ported to STM32WL and is excluded from the build
  for that family (see `_WL_EXCLUDE` in `CMakeLists.txt`). STM32F1 has
  older EXTI, USART, I2C and RTC peripherals. The build excludes `Exti`,
  `Usart`, `I2c`, `Rtc` and `GpioInterrupt` for STM32F1 (see `_F1_EXCLUDE`
  in `CMakeLists.txt`). `FastPin` is a header-only `GpioPin` for bit-bang
  loops. Port and pin are template arguments, so each access is one
  `BSRR` store or one `IDR` load. On STM32F1, `FastPin` also switches the
  pin direction (see [FastPin direction switch](#fastpin-direction-switch)).
  The USART v1 driver for STM32F1 is in
  `src/families/stm32f1/UsartV1.{h,cpp}`. `peripherals/Usart.h` includes
  it when the CMSIS header defines `USART_SR_RXNE`. See
  [USART frame format](#usart-frame-format).
- `src/delaybusywait.{h,cpp}` — `delaymS()`, `delayuS()` and `delayCycles()`.
  The Cortex-M0+ families use a busy loop of about 3 cycles per iteration.
  STM32F1 (Cortex-M3) and STM32H5 (Cortex-M33) use the DWT cycle counter
  instead, in `src/delaybusywait-dwt.cpp`. The delay is then exact to a few
  cycles, also with flash wait states. The code enables the counter on its
  own. `SystemCoreClock` must be correct.
- `src/families/<family>/` — family-specific implementations (currently
  `Adc`, `UsbDevice` (STM32L0 and STM32F1), `GpioPin.cpp`, `Afio` and `Bkp`
  (STM32F1 only), and the interrupt vector table in
  `vectors_stm32<family>xx.c`)
  exposing the same interface as their `src/peripherals/` counterpart
  where one exists. CMake adds only the active family's directory to the
  include path, so `#include "Adc.h"` resolves to the right one
  automatically. `Rcc` and `I2c` are the exception: their STM32WL
  differences are small enough to handle with in-file `#if defined(...)`
  branches instead, so they stay in `src/peripherals/` unconditionally.
  The STM32F1 `Adc` does not match the interface of the other families,
  because the F1 ADC has a different register block. It offers single
  software-triggered conversions, `ADCPRE` setup (ADCCLK at most 14 MHz),
  calibration and a VDDA measurement from Vrefint. Vrefint has no factory
  calibration value on F1. The datasheet gives 1.16 to 1.24 V (up to 85 °C)
  and up to 1.26 V (105 °C). So VDDA from Vrefint has an error of about
  ±3.3 % (+5 % at the highest temperatures), plus the ADC error. In the
  worst case this is about ±110 mV at 3.3 V and ±120 mV at 3.6 V.
- `src/utils/` — interrupt vector-table relocation and C++ member-function
  binding (`Interrupt`), GPIO interrupt dispatch (`GpioInterrupt`, not
  built for STM32WL and STM32F1 — see above).
- `src/startup-hooks.h` / `src/initialize-hardware.cpp` — declares and
  weakly defines `os_startup_initialize_hardware_early()` /
  `os_startup_initialize_hardware()`. Not part of `src/bare-metal/`
  because it's always compiled, including with
  `SYSTEM_STM32_EXCLUDE_STARTUP=ON`. That is not the same as "safe to use
  with an RTOS": µOS++ IIIe defines its own weak defaults for the same
  two hooks (`initialize-hardware.c` in its own sources), so with an RTOS
  present, *both* weak definitions land in the link, and which one wins
  is link-order dependent, not a deliberate fallback. In `sentinode-ht`
  today, µOS++'s copy happens to win — which matters, since only that
  copy honours `OS_HAS_SYSTEMCLOCK_CONFIG` and actually calls
  `SystemClock_Config()` (which `sentinode-ht` defines, as the callee,
  not as an override of either hook). A consumer that needs to guarantee
  which hook implementation runs should define it as non-weak rather
  than rely on which weak copy the linker happens to pick.
- `src/cmsis-plus/` — Cortex-M exception-handler declarations
  (`Reset_Handler`, `HardFault_Handler`, ... and the exception stack frame
  layout). Needed by the always-built per-family vector table regardless
  of which startup path is active, so it stays outside `src/bare-metal/`.
- `src/bare-metal/` — see below.
- `tests/stm32f1/` — STM32F1 test image: a minimal `main()`. CI links it
  for F103x8 and F103xB. Enable it with
  `-DSYSTEM_STM32_BUILD_TEST_IMAGE=ON` and `STM32_FAMILY=STM32F1`. The image
  never runs on hardware. Add new F1 drivers to it when they land.
- `tests/stm32h5/` — STM32H5 test image, enabled in the same way with
  `STM32_FAMILY=STM32H5`. CI links it for the H533xE. On a NUCLEO-H533RE it
  blinks LD2 (PA5). Add new H5 drivers to it when they land.
- `tests/common/` — the linker scripts of the test images: a copy of the
  µOS++ `sections.ld` and `libs.ld`.
- `svd/` — SVD register descriptions, for reference/tooling use.
- `ldscripts/` — memory linker scripts (`MEMORY` regions only) for the
  parts that this repository supplies. Today these are `mem-F103x8.ld`,
  `mem-F103xB.ld` and `mem-H533xE.ld`. The consumer adds its own sections
  script (for example `sections.ld`) and points the linker at this
  directory with `-L`. The first bytes of RAM stay free:
  `Interrupt::connectHandler()` copies the vector table to `SRAM_BASE`, and
  VTOR needs that copy aligned to the next power of two of its size. That
  is 0x100 bytes on the F103 (59 entries) and 0x400 bytes on the H533
  (149 entries). The scripts start FLASH at 0x08000000. A consumer with a
  bootloader supplies its own script with a different FLASH origin.
  `install()` copies the scripts to `share/system-stm32/ldscripts` in the
  install prefix.

## STM32H5

The STM32H5 family is a skeleton today. It is the base of the port of
versaloon-NG to the NUCLEO-H533RE (embedded-projects/versaloon-ng#89).
The H5 peripherals are close to the L0, G0 and L4 peripherals, not to the
F1 peripherals. The skeleton has these properties:

- Device `STM32H533xx`. The STM32H523xx has the same reference manual
  (RM0481) and the same IRQ positions, so the vector table also fits it.
  CI builds only the H533. Other H5 devices stop the build with `#error`.
- Cortex-M33, compiled with `-mcpu=cortex-m33 -mfloat-abi=soft`. The FPU
  stays off, as on the other families. Thus all libraries of a consumer use
  the same ABI.
- TrustZone is off (option byte `TZEN` at its default). The build uses
  `system_stm32h5xx.c`, not the `_s` or `_ns` variants. `SecureFault_Handler`
  is in the vector table only as a weak alias.
- The start-up and the fault handlers use their Armv7-M branches also for
  Armv8-M Mainline (`__ARM_ARCH_8M_MAIN__`): VTOR, `CFSR`, `MMFAR`, `BFAR`.
- The memory script `ldscripts/mem-H533xE.ld` has 512 KB flash and the
  272 KB of SRAM1 to SRAM3 as one RAM region.
- `GpioPin` (the shared `MODER` driver), `FastPin`, `Timer`, `Crs`, `Rtc`
  and the interrupt table code compile. `Rtc` sets the backup domain write
  access in `PWR_DBPCR` on the H5. None of them is tested on H5 hardware
  yet.
- `_H5_EXCLUDE` in `CMakeLists.txt` removes `Rcc.cpp`, `Usart`, `Spi`,
  `Dma`, `Exti`, `I2c` and `GpioInterrupt`. The H5 has other register
  blocks for these: the USART with FIFO and the SPI v2 (#34), GPDMA (#35),
  and one EXTI IRQ for each line.
- The RCC of the H5 has its own driver, `families/stm32h5/RccH5.{h,cpp}`.
  `peripherals/Rcc.h` includes it on the H5, so the include stays the same.
  See [STM32H5 clocks](#stm32h5-clocks).
- The CMSIS `SystemCoreClockUpdate()` of the H5 uses `float`. The start-up
  calls it, so every image links the soft-float routines (about 1.3 KB).
- Do not reset the backup domain (`RCC_BDCR.VSWRST`) after start-up. The
  reset also erases SRAM2 (errata ES0621, section 2.2.6), and SRAM2 is
  part of the RAM region of `mem-H533xE.ld`.

### STM32H5 clocks

After reset, the core runs on HSI at 32 MHz (64 MHz, `HSIDIV` /2).
Two recipes set 250 MHz, the maximum of the H5:

| Function | PLL1 input | PLL1 settings |
|---|---|---|
| `Rcc::setClock250MhzHSE(hseHz = HSE_VALUE)` | HSE crystal | M = HSE / 2 MHz, N = 250, P = 2, Q = 4 |
| `Rcc::setClock250MhzHSI()` | `hsi_ck` (64 MHz / `HSIDIV`) | M = `hsi_ck` / 2 MHz, N = 250, P = 2, Q = 4 |

Each recipe does these steps:

1. Move the system clock to HSI.
2. Increase the voltage scaling one level at a time to VOS0.
3. Start the HSE (HSE recipe only) and PLL1.
4. Set 5 flash wait states, `WRHIGHFREQ` = 2 and the prefetch.
5. Set all bus prescalers to /1 and select PLL1 as system clock.
6. Call `SystemCoreClockUpdate()`.

The recipe returns `false` if the input clock is not a multiple of 2 MHz
(for example a 25 MHz crystal), or if HSE, VOS0 or PLL1 is not ready in
time. The system clock then stays on HSI.

- The PLL input (`ref1_ck`) is 2 MHz. This is the minimum for the wide VCO
  range, and it gives the lowest long-term jitter (DS14539 table 47).
  The VCO runs at 500 MHz.
- `pll1_q_ck` is 125 MHz. It is the reset kernel clock of SPI1 to SPI3.
  The SPI baud prescaler then gives 62.5 MHz, 31.25 MHz and so on, down to
  0.49 MHz. Each value is just below a power-of-two MHz value, a usual JTAG
  or SWD clock request. `Rcc::getPLL1QClock()` returns the value.
- Set `SYSTEM_STM32_HSE_VALUE` to the crystal frequency (NUCLEO-H533RE:
  24000000). Without it, `HSE_VALUE` is 25 MHz, the CMSIS default.
- USB uses HSI48, trimmed by the CRS from USB SOF, as on L0 and G0B1:
  `Rcc::enableHSI48()`, `Rcc::setUSBClockSource(Rcc::USBClockSource::HSI48)`,
  `Rcc::enableCRSClock()` and `peripherals/Crs.h`.
- `Rcc::enableICache()` enables the instruction cache after the end of the
  cache invalidation after reset. The ICACHE reports a cacheable write as an
  error. Thus, disable it with `Rcc::disableICache()` while the firmware
  writes to the flash.
- For a 1 ms SysTick, call `SysTick_Config(SystemCoreClock / 1000)` after
  the recipe.

## STM32F1 GPIO

STM32F1 has no `MODER`, `OTYPER`, `OSPEEDR`, `PUPDR` or `AFR` registers.
Each pin has one 4-bit `CNF`/`MODE` field in `CRL` or `CRH` (RM0008
section 9.1). `src/families/stm32f1/GpioPin.cpp` maps the `GpioPin` enums
to this field:

| `GpioPin` setting | F1 `CNF`/`MODE` |
|---|---|
| `Mode::Analog` | analog input |
| `Mode::Input`, `Pull::None` | floating input |
| `Mode::Input`, `Pull::Up` / `Pull::Down` | pull input; `ODR` = 1 / 0 |
| `Mode::Output` | general-purpose output, push-pull or open-drain |
| `Mode::Alternate` | alternate-function output, push-pull or open-drain |
| `Speed::Low`, `Speed::Medium` | 2 MHz |
| `Speed::High` | 10 MHz |
| `Speed::VeryHigh` | 50 MHz |

Differences from the other families:

- Outputs have no pull resistors. `Pull::Up` or `Pull::Down` on an output
  or analog pin fails an `assert()`.
- `setOutputType()`, `setSpeed()` and `setAlternateFunction()` have no
  effect on an input pin. `setPull()` has no effect on an output pin.
  Call `setMode()` first, or use `configure()`. `configure()` writes the
  whole field at once.
- `setAlternateFunction()` ignores the AF number. It changes an output pin
  to an alternate-function output. An alternate-function input (for
  example USART RX or SPI MISO) uses `Mode::Input`.
- A pull input sets the `ODR` bit. A later change to an output drives
  that level.

`src/families/stm32f1/Afio.h` selects remapped pin sets in `AFIO_MAPR`
(`remapUsart1()`, `remapUsart2()`, `remapUsart3()`, `remapSpi1()`,
`remapI2c1()`, and `setRemap()` for other fields). Some remaps need a
larger package. On the 48-pin STM32F103C8/CB, the USART2 and USART3
remaps are not available (RM0008 tables 52 and 53). `setSwjConfig()` sets
the debug port. `SwjConfig::SwdOnly` turns off JTAG-DP and frees PA15,
PB3 and PB4. SWD on PA13/PA14 continues to work. The `SWJ_CFG` bits are
write-only, so `Afio` keeps the last written value in RAM. It writes this
value again with each remap. Call `setSwjConfig()` before the first
remap.

### Interrupt safety

`CRL` and `CRH` hold the fields of 8 pins each. Thus each `GpioPin`
configuration change on STM32F1 is a read-modify-write of a shared
register. Each change runs in a short critical section (`PRIMASK`). Thus
an interrupt that configures another pin of the same register cannot
lose a change, or lose its own change.

On the other families, the `GpioPin` setters do a read-modify-write of
`MODER`, `OTYPER`, `OSPEEDR`, `PUPDR` and `AFR` without a critical section.
There, change the configuration of one port from one context only (for
example the main loop), or disable interrupts around the change.

`set()`, `clear()`, `write()` and `toggle()` on every family write
`BSRR` only. They never write `ODR`, so they are safe in any context.

### FastPin direction switch

On STM32F1, `FastPin` can change the pin configuration fast, for example
the SWDIO direction at each SWD turnaround:

```cpp
using Swdio = hw::FastPin<GPIOB_BASE, 4>;

Swdio::setInput();  // floating input
Swdio::setOutput(); // push-pull output, 50 MHz
Swdio::setConfig<hw::gpio_f1::pinConfig(hw::GpioPin::Mode::Input, hw::GpioPin::OutputType::PushPull,
                                        hw::GpioPin::Pull::Up)>(); // any field value
```

Each call is four stores to the Cortex-M3 bit-band aliases of the four
`CNF`/`MODE` bits (RM0008 section 3.3.2). The addresses and values are
constants. With `-Os`, one call is 7 instructions: one address load,
two value loads and four stores. There is no function call, no branch and
no read-modify-write in software. Each bit-band store changes one bit in
one locked bus access, so an interrupt cannot lose a change to another
pin. The calls do not change `ODR`. For a pull input, set the pull
direction with `set()` or `clear()`.

The store order prevents bad intermediate states for a change between an
input and an output (see `gpio_f1::writeOrder()` in
`src/families/stm32f1/GpioConfig.h`):

- Input to output: the pin drives only with the target `CNF`.
- Output to input: the pin stops driving after the `MODE` stores (at
  most two), before `CNF` changes. For example, the default output
  (`MODE` 11) becomes `MODE` 10 after the first store. This is a 2 MHz
  output, so the pin still drives until the second store.
- The reserved input state (`CNF` 11, `MODE` 00) does not occur, except
  when the old or the target `CNF` is 11 (alternate open-drain).

A change between two outputs with a different `CNF` can drive with a
mixed `CNF` for one store. Use `GpioPin::configure()` for that change.

The cycle count of one direction switch at 72 MHz is not measured yet.
Measure it on hardware with the DWT cycle counter (`DWT->CYCCNT`).

## Host tests

`tests/host/` holds unit tests that run on the build machine, with fake
registers. They test the code that needs no hardware. Today they test
this STM32F1 code:

- GPIO: the `CNF`/`MODE` encoding, `GpioPin` on `CRL`/`CRH`, `toggle()`,
  and the address and store order of the `FastPin` direction switch.
- USB device driver: the `COUNTn_RX` encoding, the packet memory
  allocation, the buffer descriptor table, and the packet copy at the
  32-bit CPU stride of the packet memory. The tests also check the
  `USB_EPnR` write encoding: `CTR` bits written as 1, `STAT` written as
  `old ^ wanted`, `DTOG` written as 0, and the configuration bits kept.

The fake USB registers are plain memory. A register holds the last written
value. The fake does not model the toggle bits and the `rc_w0` bits of the
hardware. Thus the host tests do not test these items:

- The endpoint state after a call with more than one `USB_EPnR` write.
- The interrupt handler.

The CI job `host-tests` runs them.

```sh
cmake -S tests/host -B build-host
cmake --build build-host
ctest --test-dir build-host --output-on-failure
```

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
versions of all of the above, generally as weak symbols too. Building
both copies into the same firmware at least *links* for the weak
symbols — which one wins is link-order dependent, as noted for
`os_startup_initialize_hardware_early()` above, so this isn't a
guaranteed fallback either — but it is outright broken for
`dump_exception_stack()`: this library always defines it non-weak, and
µOS++ also defines its own non-weak `dump_exception_stack()` whenever the
consumer defines the `TRACE` macro — so the moment both land in the same
firmware, that's a hard multiple-definition link error.

Setting `SYSTEM_STM32_EXCLUDE_STARTUP=ON` avoids it, by not building
`src/bare-metal/` — including this file — at all. Use it whenever an
external RTOS supplies its own startup and exception handling:

```cmake
set(SYSTEM_STM32_EXCLUDE_STARTUP ON)
add_subdirectory(path/to/system-stm32)
```

`sentinode-ht` (STM32WLE5JC + µOS++ IIIe) builds this way. Leave the
option `OFF` (the default) for a bare-metal application with no RTOS,
where `src/bare-metal/` is the only thing providing a runtime at all.

## STM32F1: 72 MHz from HSE

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

### STM32F1 backup registers

`Bkp.h` (STM32F1 only) gives access to backup data register 1
(`BKP_DR1`). The backup registers keep their content over a system reset.
With VBAT supplied, they also keep it over a power loss. A typical use is
a flag for the bootloader entry that survives `NVIC_SystemReset()`.

1. Call `hw::Bkp::enable()`. It enables the PWR and BKP clocks and sets
   `PWR_CR.DBP`.
2. Use `hw::Bkp::readDR1()` and `hw::Bkp::writeDR1()`.

## USART frame format

`hw::Usart::setFrameFormat(dataBits, parity, stopBits)` sets the data bits,
the parity and the stop bits, for example from a USB CDC line coding. The
frame length is the data bits plus the parity bit. The function disables
the USART for the change and then restores the enable bit. Wait for
`isTransferComplete()` before the call. The word length must not change
during a transfer (RM0008 27.6.4 for STM32F1).

| Format | USART v2 (L0, G0, WL) | USART v1 (STM32F1) |
|--------|-----------------------|--------------------|
| 7 data bits, no parity | 7-bit frame | not supported, returns `false` |
| 7 data bits, even or odd parity | 8-bit frame | 8-bit frame |
| 8 data bits, no parity | 8-bit frame | 8-bit frame |
| 8 data bits, even or odd parity | 9-bit frame | 9-bit frame |

`getData()` removes the received parity bit from a 7-bit + parity frame.

The STM32F1 `Usart` class has the same interface as the v2 class, but it
does not have the functions for hardware that USART v1 does not have:

- Clock source selection: USART1 uses APB2, the other USARTs use APB1.
- Receiver timeout. `isReceiverTimeoutFlag()` always returns `false`, so
  that shared receive code compiles.
- RS485 driver enable, auto baud rate, pin swap, prescaler and 8x
  oversampling.

On USART v1, a read of SR followed by a read of DR clears the PE, FE, NE,
ORE and IDLE flags. The hardware sets PE, FE, NE and ORE only together with
RXNE. The normal receive path (`isRxRegisterNotEmpty()`, then `getData()`)
therefore clears them. `clearFramingError()` and `clearIdleFlag()` also
read DR, so they discard the byte in DR. `clearAllStatusFlags()` does not
read DR. It clears only TC, LBD and CTS, so it cannot drop a received byte.

## USB clock: HSI48 and CRS

The USB full-speed peripheral needs a 48 MHz clock. The STM32L0 and the
STM32G0B1/G0C1 have no crystal requirement for USB. They use the internal
48 MHz RC oscillator (HSI48) for this clock. `Rcc` controls HSI48 and the
USB clock selection:

| Function | STM32L0 | STM32G0B1 / G0C1 |
|---|---|---|
| `Rcc::enableHSI48()` | Sets `SYSCFG_CFGR3.ENREF_HSI48` and `RCC_CRRCR.HSI48ON`, then waits for `HSI48RDY` | Sets `RCC_CR.HSI48ON`, then waits for `HSI48RDY` |
| `Rcc::disableHSI48()` | Clears `HSI48ON` and `ENREF_HSI48` | Clears `HSI48ON` |
| `Rcc::isHSI48Ready()` | Reads `RCC_CRRCR.HSI48RDY` | Reads `RCC_CR.HSI48RDY` |
| `Rcc::setUSBClockSource()` | `RCC_CCIPR.HSI48SEL`: `PLL` or `HSI48` | `RCC_CCIPR2.USBSEL`: `HSI48`, `HSE` or `PLLQ` |
| `Rcc::enableUSBClock()`, `Rcc::enableCRSClock()` | `RCC_APB1ENR` and the Sleep-mode bits in `RCC_APB1SMENR` | `RCC_APBENR1` |

These functions exist only when the CMSIS device header defines the
related register bits. STM32G071, STM32C0 and STM32WL have no HSI48, so
the functions are not declared there.

Family details:

- **STM32L0:** HSI48 needs the VREFINT buffer for HSI48 (RM0367 section
  7.2.4). `enableHSI48()` sets `ENREF_HSI48` and enables the SYSCFG clock
  for this. If `SYSCFG_CFGR3.REF_LOCK` is set and `ENREF_HSI48` is
  clear, `enableHSI48()` returns `false`.
- **STM32L0:** `HSI48SEL` selects the 48 MHz clock for USB *and* RNG.
- **STM32G0B1/G0C1:** the hardware sets `HSI48ON` by itself while HSI48
  is the USB clock and the USB peripheral is enabled (RM0444 section
  5.2.3).
- HSI48 stops in Stop and Standby mode on both families.

`enableHSI48()` returns `false` if `HSI48RDY` does not come within a
bounded wait. The startup time is 6 µs maximum, so this only occurs if the
oscillator does not start.

### Clock Recovery System (CRS)

The factory calibration of HSI48 is not accurate enough for USB (±4 % on
the STM32L073 at 25 °C). USB full speed needs ±0.25 %. The CRS fixes
this. It measures HSI48 against a synchronisation signal and trims HSI48
by itself. For USB, the signal is the start-of-frame (SOF) packet. The
host sends one SOF every 1 ms.

`src/peripherals/Crs.h` (namespace `hw::Crs`) is one driver for STM32L0
and STM32G0B1/G0C1, because the CRS registers are the same on both
families. Only the `TRIM` field width differs: 6 bits on L0, 7 bits on
G0. `hw::Crs::TrimMax` and `hw::Crs::TrimDefault` get the width from the
CMSIS header. The driver exists only when the CMSIS header defines `CRS`.
The driver is header-only. `Crs.cpp` only includes the header, so every
library build and every CI job compiles it.

| Function | Purpose |
|---|---|
| `configure(Config)` | Writes `CRS_CFGR`: sync source, divider, polarity, `RELOAD`, `FELIM`. The defaults are USB SOF, 48 MHz / 1 kHz. Returns `false` while the counter runs, because the hardware then write-protects `CRS_CFGR`. |
| `enable(autoTrim = true)` / `disable()` | Starts or stops the frequency error counter (`CEN`) and automatic trimming (`AUTOTRIMEN`). |
| `getSyncStatus()` | Returns `None`, `Ok`, `Warning` or `Error` from the `CRS_ISR` flags. |
| `isFlagSet(Flag)` / `clearFlags()` | Reads one `CRS_ISR` flag or clears all of them. |
| `setTrim()` / `getTrim()` | Manual trimming. The hardware ignores writes while automatic trimming is on. |
| `getFrequencyErrorCapture()` / `isFrequencyBelowTarget()` | `FECAP` and `FEDIR` of the last SYNC event. |

Start-up sequence for USB from HSI48:

```cpp
#include "peripherals/Crs.h"
#include "peripherals/Rcc.h"

if(!Rcc::enableHSI48()) {
   // HSI48 did not start, or SYSCFG_CFGR3 is locked (L0)
}
Rcc::setUSBClockSource(Rcc::USBClockSource::HSI48);
Rcc::enableCRSClock();
(void)hw::Crs::configure({}); // USB SOF, RELOAD = 47999, FELIM = 34
hw::Crs::enable();             // counter and automatic trimming on
Rcc::enableUSBClock();
```

The host sends SOF packets after the USB bus reset, and only while the
bus is not suspended. Without SOF packets, HSI48 runs at its last trimming
value.

Without SOF packets, the CRS also reports an error. The frequency error
counter reaches its OUTRANGE limit (128 × `FELIM`), and the hardware sets
`SYNCMISS` and `ERRF` (RM0367 8.7.3, RM0444 6.7.3). Thus
`getSyncStatus()` returns `SyncStatus::Error` with `Flag::SyncMissed`
before the first SOF, during USB suspend and after an unplug. This is
expected. It does not mean that HSI48 is broken.

The flags stay set until `clearFlags()`. Call `clearFlags()` before you
read a new status, for example after a USB reset or a resume.

The CRS stops in Stop mode and loses its configuration in Standby mode.
After Standby, run the sequence again.

## USB device driver

`hw::UsbDevice` drives the USB full-speed device peripheral. It is
packet-level only. It knows nothing about descriptors, standard requests
or USB classes. That logic lives above the driver (libArT `usb::Device`
and the libart-stm32 controller). This library does not depend on
libArT.

### Family split

| Family | Driver | Status |
|---|---|---|
| STM32L0 with USB (L052, L053, L062, L063, L072, L073, L082, L083) | `src/families/stm32l0/UsbDevice.{h,cpp}` | Available. On L0 devices without USB (for example L071), the files compile to nothing. |
| STM32F102 / F103 | `src/families/stm32f1/UsbDevice.{h,cpp}` | Available. Same peripheral as on STM32L0, with the differences in [STM32F1 differences](#stm32f1-differences). On F1 devices without USB (for example F101), the files compile to nothing. |
| STM32G0B1 / G0C1 | — | Not yet. USB DRD peripheral: 2 KB packet memory with 32-bit access, `CHEP` registers. A new `src/families/stm32g0/UsbDevice.{h,cpp}` with the same interface is necessary. |
| STM32G071, STM32C0, STM32WL | — | No USB peripheral |

The family-neutral types are in `src/peripherals/UsbDeviceTypes.h`
(namespace `hw::usb`): `Event`, `EventType`, `EndpointType` and
`EventCallback`. Every family driver uses these types and has the same
public interface. Thus a new family driver needs no change above the
driver.

### Requirements

- A 48 MHz USB clock. On STM32L0, use HSI48 with the CRS on USB SOF. See
  [USB clock: HSI48 and CRS](#usb-clock-hsi48-and-crs). On STM32F1, the
  USB clock comes from the PLL: 72 MHz / 1.5 or 48 MHz / 1
  (`RCC_CFGR.USBPRE`). Thus STM32F1 needs a crystal (HSE) for USB.
  `Rcc::setClock72MhzHSE()` sets this clock (see
  [STM32F1: 72 MHz from HSE](#stm32f1-72-mhz-from-hse)).
- The USB bus clock enabled with `Rcc::enableUSBClock()` before `begin()`.
- On STM32F1, call `Rcc::setClock72MhzHSE()` before `Rcc::enableUSBClock()`.
  `USBPRE` must be valid before the USB clock is enabled. It cannot change
  while the USB clock is enabled (RM0008 section 7.3.2).
- On STM32L0, an APB clock of 10 MHz or more (RM0367 section 32.5.2,
  errata ES0292 2.16.1). A lower APB clock causes packet memory overrun or
  underrun. On STM32F1, an APB1 clock of more than 8 MHz (RM0008 section
  23.4.2).
- PA11 (DM) and PA12 (DP) need no GPIO setup. The transceiver uses them
  when the peripheral is powered up.

### Use

```cpp
#include "UsbDevice.h"

hw::UsbDevice usb;

void onUsbEvent(void *context, const hw::usb::Event &event) {
   // Called in the USB interrupt: copy the event into a queue.
}

usb.begin(&onUsbEvent, nullptr); // power-up, reset, IRQ on; still detached
usb.connect();                   // D+ pull-up on: the host sees the device
```

| Function | Purpose |
|---|---|
| `begin(callback, context)` / `end()` | Power-up sequence (STM32L0: RM0367 32.5.2; STM32F1: RM0008 23.4.2), interrupt connection through `Interrupt::connectMemberHandler()` (STM32L0: `USB_IRQn`; STM32F1: `USB_LP_CAN1_RX0_IRQn`), and power-down. |
| `connect()` / `disconnect()` | D+ pull-up on or off. STM32L0: internal pull-up (`USB_BCDR.DPPU`). STM32F1: the board callback from `setPullupCallback()`. |
| `setPullupCallback(callback, context)` | STM32F1 only. Sets the board function that switches the external D+ pull-up. |
| `isConnected()` | STM32L0: the state of `USB_BCDR.DPPU`. STM32F1: the state that the last `connect()` or `disconnect()` requested. |
| `setAddress(address)` | Writes `USB_DADDR`. Call it after the status stage of SET_ADDRESS. |
| `UsbDevice(endpointCount)` | STM32F1 only. `endpointCount` is the highest endpoint number of the device plus 1 (default 8). The buffer descriptor table then has `endpointCount * 8` bytes. |
| `openEndpoint(address, type, maxPacketSize, doubleBuffered)` | Opens one direction and allocates its packet buffer. The endpoint starts at DATA0 and answers NAK. An open direction can open again only with the same `maxPacketSize` and `doubleBuffered`. `doubleBuffered` (default `false`) opens a double-buffered bulk OUT endpoint, on STM32F1 only (see [Double-buffered bulk OUT](#double-buffered-bulk-out-stm32f1)). |
| `closeAllEndpoints()` | Disables all endpoints except EP0 and frees their packet memory (for SET_CONFIGURATION). |
| `startIn(address, data)` | Sends one packet (0 to `maxPacketSize` bytes). Returns `false` while the previous packet is not sent. |
| `armOut(address)` / `readOut(address, buffer)` | Accepts the next OUT packet / copies the received packet. |
| `setStall(address, stall)` / `isStalled(address)` / `setNak(address)` | Endpoint status. Clearing STALL also resets the data toggle to DATA0. |

### Events

The interrupt handler reports these events through the callback:

| `EventType` | Meaning |
|---|---|
| `Reset` | USB bus reset. The driver opens EP0 again (64 bytes, OUT VALID) and sets the address to 0. Other endpoints are closed. |
| `Setup` | SETUP packet. `Event::setup` holds the 8 bytes. Both directions of the endpoint are then NAK. |
| `Out` | OUT packet with `Event::size` bytes. The endpoint answers NAK until the next `armOut()`. A double-buffered endpoint can receive one more packet before `armOut()`. `armOut()` then reports that packet. |
| `InComplete` | The host acknowledged the IN packet. The endpoint answers NAK until the next `startIn()`. |
| `Suspend` | No bus activity for 3 ms. This also occurs when the cable is pulled. The driver sets `FSUSP`. |
| `Resume` | Bus activity after `Suspend`. |

A control endpoint accepts the next SETUP packet even while it answers
NAK. That SETUP overwrites the EP0 OUT buffer in packet memory. Thus the
interrupt handler copies these packets before it clears the `CTR_RX` flag:

- A SETUP packet goes into `Event::setup`.
- An EP0 OUT packet goes into a 64-byte buffer in RAM. `readOut(0x00)`
  reads from this copy.

Bulk and interrupt OUT data stays in packet memory, because the endpoint
answers NAK until the next `armOut()`. For every endpoint, `readOut()`
can run in the callback or later in the main loop, until the next
`armOut()` of that endpoint. EP0 is the only control endpoint.

### Interrupt and main loop

Every public function can run in the event callback (USB interrupt) or in
the main loop. `begin()` and `end()` are the exception: call them from the
main loop only. The endpoint functions mask the USB interrupt (`USB_IRQn`
on STM32L0, `USB_LP_CAN1_RX0_IRQn` on STM32F1) while they run. Thus
a bus reset in the interrupt cannot interleave with `openEndpoint()` or
`closeAllEndpoints()` in the main loop.

The endpoint status bits in `USB_EPnR` are toggle bits. A status change is
a read and an XOR write. If the hardware completes a transaction between
the read and the write, the result is wrong. This can occur only on a VALID
endpoint. Thus `setStall()` and `setNak()` are race-free only on an
endpoint that is not VALID. Do not use them to cancel a pending `startIn()`
or `armOut()`.

### Packet memory

The packet memory (1024 bytes on STM32L0, 512 bytes on STM32F1) holds
the buffer descriptor table (64 bytes at offset 0) and the endpoint
buffers. On STM32F1, the table has `endpointCount * 8` bytes (see the
constructor). The driver allocates the
buffers in sequence. An OUT buffer is rounded up to the `COUNTn_RX`
block size: 2-byte blocks up to 62 bytes, 32-byte blocks above.

The hardware also writes the CRC16 of a received packet into the OUT
buffer, but never past the allocated size (`BL_SIZE` and `NUM_BLOCK`).
On STM32L0, these CRC bytes are wrong (errata ES0292 2.16.3), but the
driver never reads them: `readOut()` copies only the payload.

### Limits

- Endpoint numbers 0 to 7. Both directions of one number share one
  register, so they have the same type.
- No isochronous endpoints. Double-buffered bulk OUT endpoints on STM32F1
  only. No double-buffered IN endpoints.
- No remote wakeup, no Link Power Management (LPM) and no Battery
  Charging Detection (BCD). On STM32L0, two USB errata of ES0292 do not
  apply: 2.16.2 (SUSP timing after resume signalling by the device) and
  2.16.4 (BCD below -20 °C). A driver change that adds remote wakeup must mask
  `SUSPM` for 3 ms after it signals resume (2.16.2 workaround).

### STM32F1 differences

The STM32F1 driver is a copy of the STM32L0 driver. The register
interface, the event model, the `USB_EPnR` toggle logic and the
`COUNTn_RX` encoding are the same. These items are different:

- Packet memory: 512 bytes. The CPU sees each 16-bit word at a 32-bit
  stride. USB offset `o` is at `USB_PMAADDR + o * 2`. The buffer
  descriptor table and the endpoint buffer addresses stay USB offsets.
  Only 16-bit access is possible.
- D+ pull-up: STM32F1 has no `USB_BCDR` and no internal pull-up. The board
  has a 1.5 kΩ pull-up on D+, switched by a GPIO or fixed. Give the switch
  function to `setPullupCallback()` before `connect()`. Without a callback,
  `connect()` and `disconnect()` only change `isConnected()`.
- `isConnected()` reports the requested state, not the bus state. With a
  fixed pull-up, the host sees the device from power-on, before `begin()`.
  `isConnected()` is still `false` until `connect()`.
- Interrupt: `USB_LP_CAN1_RX0_IRQn`. The hardware also sets
  `USB_HP_CAN1_TX_IRQn` for isochronous and double-buffered endpoints.
  The driver does not enable it: the low-priority interrupt also comes for
  these transfers (RM0008 section 23.3.1).
- bxCAN shares the packet memory. Do not use CAN together with USB.
- No LPM and no BCD registers. `USB_CNTR_LPMODE` has the name
  `USB_CNTR_LP_MODE` in the STM32F1 CMSIS header.
- Double-buffered bulk OUT endpoints (see below).
- The constructor argument `endpointCount` makes the buffer descriptor
  table smaller. This gives more packet memory to the endpoint buffers.

```cpp
void usbPullup(void *, bool enable) {
   board::usbPullup.write(enable); // for example PC13 or PA2
}

usb.setPullupCallback(&usbPullup, nullptr);
usb.begin(&onUsbEvent, nullptr);
usb.connect();
```

### Double-buffered bulk OUT (STM32F1)

With a single buffer, an OUT endpoint answers NAK from the end of a packet
until the next `armOut()`. The host then sends the next packet again
later, so the transfer rate goes down. A double-buffered bulk OUT endpoint
(RM0008 section 23.4.3) has two buffers. The hardware receives the next
packet into one buffer while the software reads the other.

`openEndpoint(address, EndpointType::Bulk, maxPacketSize, true)` opens it.
The endpoint uses both buffer descriptors of its number. Thus the IN
direction of that number stays closed. It uses two buffers of
`maxPacketSize` bytes in packet memory.

The interface does not change. The driver reports one `Out` event for each
`armOut()`, and `readOut()` reads that packet. These are the steps:

1. A packet arrives, and no packet is reported. The driver reports it at
   once, and gives the other buffer to the hardware (it toggles `SW_BUF`).
2. A second packet arrives before `armOut()`. The driver keeps it in the
   other buffer and reports nothing. The hardware answers NAK, because
   both buffers are full.
3. `armOut()` frees the buffer of the reported packet. If a packet waits,
   `armOut()` reports it before it returns. Thus the event callback can
   run in the main loop, with the USB interrupt masked.

`setStall(address, false)` and `openEndpoint()` start the buffers again at
buffer 0. A packet that waits is then lost, as a clear of STALL drops the
transfer.

A double-buffered 64-byte endpoint uses 128 bytes. With the default
64-byte table and EP0 (128 bytes), 192 bytes stay free for the other
endpoints. With `hw::UsbDevice usb{ 5 };` (endpoint numbers 0 to 4), the
table has 40 bytes, and 216 bytes stay free.


# Building and using system-stm32

## Requirements

- CMake >= 3.20
- An `arm-none-eabi-gcc` toolchain. `cmake/gcc_arm.cmake` is a toolchain
  file for it. Give the toolchain directory with `GCC_TOOLCHAIN_PREFIX`.
- C11 / C++17 or later. Below C++20, the build also needs span-compat (see
  [Span type](#span-type)).

## Standalone build

A standalone build compiles the library for one family. CI does this for
each family (see [Tests and CI](testing.md)):

```sh
cmake -B build \
    -D CMAKE_TOOLCHAIN_FILE=cmake/gcc_arm.cmake \
    -D GCC_TOOLCHAIN_PREFIX=/opt/arm-gnu-toolchain \
    -D STM32_FAMILY=STM32H5 \
    -D STM32_DEVICE=STM32H533xx
cmake --build build --target system-stm32
```

CMake gets the CMSIS device headers and CMSIS core for the selected family
with `FetchContent`. You do not need a copy of them in your project.

## C++ standard

The target requires C++17 (`cxx_std_17`). It does not force a later
standard on its consumers. The consumer selects the standard with
`CMAKE_CXX_STANDARD`. A value that the superproject sets before
`add_subdirectory()` also applies to this library. A standalone build of
this library uses C++23 if you do not set `CMAKE_CXX_STANDARD`. To build
it at C++17, give `-D CMAKE_CXX_STANDARD=17`.

The target exports the CMSIS headers as `SYSTEM` include directories. Thus warnings in
these vendor headers do not stop a `-Wpedantic -Werror` build. For example,
`core_cm33.h` has an anonymous struct, which ISO C++ does not allow.

## Span type

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

There are two ways to use the library:

- Add it as a git submodule and call `add_subdirectory()`. The downstream
  projects use it this way.
- Install it with `install()`, then call `find_package(system-stm32)`.

The library is the `system-stm32` CMake target (an `OBJECT` library). The
installed package exports it as `art::system-stm32`.

```cmake
set(STM32_FAMILY STM32G0)
set(STM32_DEVICE STM32G0B1xx)
add_subdirectory(dependencies/system-stm32)
target_link_libraries(my-firmware PRIVATE system-stm32)
```

## CMake variables

| Variable | Default | Purpose |
|---|---|---|
| `STM32_FAMILY` | `STM32L0` | The family: `STM32L0`, `STM32C0`, `STM32G0`, `STM32WL`, `STM32F1` or `STM32H5`. It selects the CMSIS packages and `src/families/<family>/`. |
| `STM32_DEVICE` | `STM32L073xx` | The CMSIS device define, for example `STM32WLE5xx` or `STM32H533xx`. Use `STM32F103xB` for STM32F103x8 and STM32F103xB. CMSIS has no separate define for the 64 KB part. |
| `SYSTEM_STM32_EXCLUDE_STARTUP` | `OFF` | Do not compile `src/bare-metal/`. Use it with an RTOS that supplies its own start-up. See [Bare-metal start-up and an external RTOS](startup-and-rtos.md). |
| `SYSTEM_STM32_HSE_VALUE` | empty | The HSE crystal frequency in Hz, for example `12000000`. See below. |
| `SYSTEM_STM32_BUILD_TEST_IMAGE` | `OFF` | Build the test image of the family. Only for `STM32F1` and `STM32H5`. See [Tests and CI](testing.md). |

CMake gives `SYSTEM_STM32_HSE_VALUE` to the target and to its consumers as
the `HSE_VALUE` define. The CMSIS `SystemCoreClockUpdate()` and the `Rcc`
clock recipes both use it. A wrong value gives a wrong `SystemCoreClock`
after a switch to HSE or to the HSE PLL. If the variable is empty, the
family default applies: 8 MHz on STM32F1, 25 MHz on STM32H5, and the CMSIS
default on the other families.

## Family selection

CMake adds only `src/families/<family>/` of the active family to the
include path. A family header in that directory replaces the shared header
of the same name. Some shared sources do not match the registers of a
family. `CMakeLists.txt` removes them from the build with these lists:

| List | Family | Removed sources |
|---|---|---|
| `_WL_EXCLUDE` | STM32WL | `GpioInterrupt` |
| `_F1_EXCLUDE` | STM32F1 | `Exti`, `GpioPin.cpp` (the family has its own), `I2c`, `Rtc`, `Usart.cpp` (USART v1 is in the family directory), `GpioInterrupt` |
| `_H5_EXCLUDE` | STM32H5 | `Dma.cpp`, `Rcc.cpp`, `Spi.cpp` (the family has its own), `Exti`, `I2c`, `GpioInterrupt` |

CMake compiles the USB device core only for a family with a
`UsbFsFamily.h` in its directory.

The delay functions `delaymS()`, `delayuS()` and `delayCycles()` have two
implementations:

- STM32F1 (Cortex-M3) and STM32H5 (Cortex-M33) use the DWT cycle counter
  (`src/delaybusywait-dwt.cpp`). The delay is exact to a few cycles, also
  with flash wait states. The code enables the counter itself.
- The other families use a busy loop of about 3 cycles for each iteration
  (`src/delaybusywait.cpp`).

Both implementations need a correct `SystemCoreClock`.

## Linker scripts

`ldscripts/` holds memory linker scripts (`MEMORY` regions only) for the
parts that this repository supplies. Today these are `mem-F103x8.ld`,
`mem-F103xB.ld` and `mem-H533xE.ld`. The consumer adds its own sections
script (for example `sections.ld`). It points the linker at this directory
with `-L`.

The first bytes of RAM stay free. `Interrupt::connectHandler()` copies the
vector table to `SRAM_BASE`. VTOR needs that copy aligned to the next power
of two of its size: 0x100 bytes on the F103 (59 entries) and 0x400 bytes
on the H533 (149 entries).

The scripts start FLASH at 0x08000000. A consumer with a bootloader
supplies its own script with a different FLASH origin. `install()` copies
the scripts to `share/system-stm32/ldscripts` in the install prefix.

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
    -D CMAKE_BUILD_TYPE=Release \
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
  [span-compat](https://github.com/armint/span-compat)
  with `FetchContent` and links it. It also defines
  `SYSTEM_STM32_USE_SPAN_COMPAT`. Then `hw::span` is `span_compat::span`.

CMake selects the type from `CMAKE_CXX_STANDARD`. If `CMAKE_CXX_STANDARD`
is not set, CMake also uses span-compat.

All translation units of one program must see the same span type. The
firmware can build at C++23 and this library at C++17. Thus, when CMake
fetches span-compat, it sets `SPAN_COMPAT_USE_STD_SPAN` to `OFF`. Then
`span_compat::span` is its own implementation at every standard. A value
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
cmake_minimum_required(VERSION 3.20)
# Before project(): the toolchain file also reads STM32_FAMILY.
set(STM32_FAMILY STM32G0 CACHE STRING "")
set(STM32_DEVICE STM32G0B1xx CACHE STRING "")
project(my-firmware C CXX ASM)

add_subdirectory(dependencies/system-stm32)
add_executable(my-firmware main.cpp)
target_link_libraries(my-firmware PRIVATE system-stm32)
```

Configure with the ARM toolchain file and a build type:

```sh
cmake -B build \
    -D CMAKE_TOOLCHAIN_FILE=dependencies/system-stm32/cmake/gcc_arm.cmake \
    -D GCC_TOOLCHAIN_PREFIX=/opt/arm-gnu-toolchain \
    -D CMAKE_BUILD_TYPE=Release
```

The toolchain file puts the core flags (`-mcpu=...`) only into the flags
of the `Debug`, `Release` and `RelWithDebInfo` build types. Without a build
type, the compiler gets no core flags and the build fails.
Set `STM32_FAMILY` and `STM32_DEVICE` as cache entries (`CACHE STRING`) or
with `-D` on the command line. A plain `set()` has no effect on the first
configure. The library has `cmake_minimum_required(VERSION 3.20)`, so policy
CMP0126 is OLD. The `set(... CACHE ...)` call in the library then removes
the plain variable, and the build uses the defaults (STM32L0).

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
after a switch to HSE or to the HSE PLL. If the variable is empty, CMake
sets 8 MHz on STM32F1 and 25 MHz on STM32H5. On the other families, CMake
sets no `HSE_VALUE`. Then the CMSIS system file uses its own default, and
`Rcc.h` uses 48 MHz. These two values are not the same. Set the variable
on each board that uses HSE.

## Family selection

CMake adds only `src/families/<family>/` of the active family to the
include path. Thus a header that only the family directories have, for
example `Adc.h`, resolves to the driver of the active family. Some shared
sources do not match the registers of a family. `CMakeLists.txt` removes them from the build with these lists:

| List | Family | Removed sources |
|---|---|---|
| `_WL_EXCLUDE` | STM32WL | `GpioInterrupt` |
| `_F1_EXCLUDE` | STM32F1 | `Exti`, `GpioPin.cpp` (the family has its own), `I2c`, `Rtc`, `Usart.cpp` (USART v1 is in the family directory), `GpioInterrupt` |
| `_H5_EXCLUDE` | STM32H5 | `Dma.cpp`, `Rcc.cpp`, `Spi.cpp` (the family has its own), `Exti`, `I2c`, `GpioInterrupt` |

`Rcc` and `I2c` stay in `src/peripherals/` on STM32WL. Their WL
differences are small, so in-file `#if defined(...)` branches handle them.

CMake compiles the USB device core only for a family with a
`UsbFsFamily.h` in its directory.

CMake gets these CMSIS device packages from the STMicroelectronics GitHub
repositories. CMSIS core is `cmsis_core` v5.4.0, in the variant for the
core of the family.

| `STM32_FAMILY` | CMSIS device package |
|---|---|
| `STM32L0` | `cmsis_device_l0` v1.9.4 |
| `STM32C0` | `cmsis_device_c0` v1.0.0 |
| `STM32G0` | `cmsis_device_g0` v1.4.4 |
| `STM32WL` | `cmsis_device_wl` v1.4.0 |
| `STM32F1` | `cmsis_device_f1` v4.3.5 |
| `STM32H5` | `cmsis_device_h5` v1.7.0 |

The delay functions `delaymS()`, `delayuS()` and `delayCycles()` have two
implementations:

- STM32F1 (Cortex-M3) and STM32H5 (Cortex-M33) use the DWT cycle counter
  (`src/delaybusywait-dwt.cpp`). The delay is exact to a few cycles, also
  with flash wait states. The code enables the counter itself.
- The other families use a busy loop of about 3 cycles for each iteration
  (`src/delaybusywait.cpp`).

Both implementations need a correct `SystemCoreClock`.

## Linker scripts

`ldscripts/` holds the linker scripts. The test images use them, and
firmware can use them too:

- Memory scripts (`MEMORY` regions only), one for each part:

  | Script | FLASH | RAM |
  |---|---|---|
  | `mem-L031x6.ld` | 32K | 4K (erratum 2.1.5 of revision A; the part has 8K) |
  | `mem-L053x8.ld` | 64K | 8K |
  | `mem-L073xZ.ld` | 192K | 20K |
  | `mem-G071xB.ld` | 128K | 36K |
  | `mem-G0B1xE.ld` | 512K | 144K |
  | `mem-WLE5xC.ld` | 256K | 64K |
  | `mem-F103x8.ld` | 64K | 20K |
  | `mem-F103xB.ld` | 128K | 20K |
  | `mem-H523xC.ld` | 256K | 272K |
  | `mem-H533xE.ld` | 512K | 272K |

  FLASH starts at 0x08000000 and RAM at 0x20000000.
- `sections.ld`, the µOS++ default Cortex-M sections script.
- `libs.ld`, an empty list of more libraries.

Point the linker at this directory with `-L`. Then give the memory script
first, for example `-Tmem-F103x8.ld -Tlibs.ld -Tsections.ld`.

### Linker script parameters

The scripts have optional parameters. Set them with
`-Wl,--defsym=<symbol>=<value>`. Give the scripts with `-T`, not with
`-Wl,-T` or `-Xlinker -T`. The gcc driver then passes all `-Wl` options to
ld before the scripts, in any order of the options. ld reads a `--defsym`
after a script too late. The script then uses the default, and the link
does not show an error.

| Symbol | Script | Default | Function |
|---|---|---|---|
| `__flash_offset` | memory | 0 | Flash in front of the image, for example for a bootloader. Use a multiple of 1 KB: VTOR needs the flash vector table aligned, and `sections.ld` stops the link otherwise. |
| `__flash_reserved_end` | memory | 0 | Flash at the end of the part that stays out of FLASH, for example for settings or keys. |
| `__flash_length` | memory | rest | Length of FLASH. The default is the flash from the image start to the reserved flash at the end. |
| `__main_stack_size` | `sections.ld` | 8192 | Size of the main stack. |
| `__minimum_stack_size` | `sections.ld` | 256 | Free RAM for the stack. The link fails if less RAM is free. |

The memory scripts define `__flash_reserved_start`. This is the address of
the flash that `__flash_reserved_end` keeps free.

This example links an image behind an 8 KB bootloader. It keeps the last
1 KB page for settings, and makes sure that the full 2 KB stack fits:

```cmake
target_link_options(app PRIVATE
    -Wl,--defsym=__flash_offset=0x2000
    -Wl,--defsym=__flash_reserved_end=0x400
    -Wl,--defsym=__main_stack_size=2048
    -Wl,--defsym=__minimum_stack_size=2048
    -L${system-stm32_SOURCE_DIR}/ldscripts
    -Tmem-F103x8.ld -Tlibs.ld -Tsections.ld)
```

`__flash_length` sets the length directly. Use it for a bootloader that
must fit in its area, or to let a large image link for a size report.

The scripts do not reserve RAM for the vector table.
`Interrupt::connectHandler()` copies the vector table to RAM. The copy is a
static array in the section `.bss.ramVectorTable`, so the sections script
puts it in `.bss` with `*(.bss .bss.*)`. Compile with `-ffunction-sections`
and link with `--gc-sections`: then the array uses RAM only if the firmware
calls `connectHandler()`. VTOR needs the copy aligned to the next power of
two of its size. The minimum is 256 bytes, for the Cortex-M0+. On the F103,
this is 256 bytes (59 entries). On the H533, it is 1024 bytes (149
entries). In `.bss`, the alignment can add padding before the copy.

To prevent this padding, put the section first in RAM. Add this rule to the
sections script before `.data`, as in `ldscripts/sections.ld`:

```
.ram_vector_table (NOLOAD) :
{
    *(.bss.ramVectorTable)
} >RAM
```

If `ORIGIN(RAM)` is a multiple of the table alignment, the copy needs no
padding there. If the firmware does not call `connectHandler()`, this
section is empty.

Call `connectHandler()` only after the start-up code clears `.bss`, thus in
`os_startup_initialize_hardware()` or later. Without the rule above, the
copy is in `.bss`, and an earlier call loses the table.

By default, the scripts start FLASH at 0x08000000. `install()` copies the
scripts to `share/system-stm32/ldscripts` in the install prefix.

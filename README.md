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
  (`Rcc`, `Crs`, `GpioPin`, `Usart`, `I2c`, `Dma`, `Timer`, `Exti`, `Spi`,
  `Rtc`, `Iwdg`, `Dbg`). `#ifdef` on specific register/feature availability (e.g.
  `#if defined(USART_PRESC_PRESCALER)`) is fine here; `#ifdef STM32xx` for
  whole-family differences is not — that belongs below. `GpioInterrupt` in
  `src/utils/` is not yet ported to STM32WL and is excluded from the build
  for that family (see `_WL_EXCLUDE` in `CMakeLists.txt`).
- `src/families/<family>/` — family-specific implementations (currently
  `Adc`, `UsbDevice` (STM32L0 only), and the interrupt vector table in
  `vectors_stm32<family>xx.c`)
  exposing the same interface as their `src/peripherals/` counterpart
  where one exists. CMake adds only the active family's directory to the
  include path, so `#include "Adc.h"` resolves to the right one
  automatically. `Rcc` and `I2c` are the exception: their STM32WL
  differences are small enough to handle with in-file `#if defined(...)`
  branches instead, so they stay in `src/peripherals/` unconditionally.
- `src/utils/` — interrupt vector-table relocation and C++ member-function
  binding (`Interrupt`), GPIO interrupt dispatch (`GpioInterrupt`, not
  built for STM32WL — see above).
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
| STM32G0B1 / G0C1 | — | Not yet. USB DRD peripheral: 2 KB packet memory with 32-bit access, `CHEP` registers. A new `src/families/stm32g0/UsbDevice.{h,cpp}` with the same interface is necessary. |
| STM32G071, STM32C0, STM32WL | — | No USB peripheral |

The family-neutral types are in `src/peripherals/UsbDeviceTypes.h`
(namespace `hw::usb`): `Event`, `EventType`, `EndpointType` and
`EventCallback`. Every family driver uses these types and has the same
public interface. Thus a new family driver needs no change above the
driver.

### Requirements

- A 48 MHz USB clock. On STM32L0, use HSI48 with the CRS on USB SOF. See
  [USB clock: HSI48 and CRS](#usb-clock-hsi48-and-crs).
- `Rcc::enableUSBClock()` before `begin()`.
- An APB clock of 10 MHz or more (RM0367 section 32.5.2).
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
| `begin(callback, context)` / `end()` | Power-up sequence (RM0367 32.5.2), interrupt connection through `Interrupt::connectMemberHandler(USB_IRQn, ...)`, and power-down. |
| `connect()` / `disconnect()` | Internal D+ pull-up (`USB_BCDR.DPPU`). |
| `setAddress(address)` | Writes `USB_DADDR`. Call it after the status stage of SET_ADDRESS. |
| `openEndpoint(address, type, maxPacketSize)` | Opens one direction and allocates its packet buffer. The endpoint starts at DATA0 and answers NAK. |
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
| `Out` | OUT packet with `Event::size` bytes. The endpoint answers NAK until the next `armOut()`. |
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
main loop only. The endpoint functions mask `USB_IRQn` while they run. Thus
a bus reset in the interrupt cannot interleave with `openEndpoint()` or
`closeAllEndpoints()` in the main loop.

The endpoint status bits in `USB_EPnR` are toggle bits. A status change is
a read and an XOR write. If the hardware completes a transaction between
the read and the write, the result is wrong. This can occur only on a VALID
endpoint. Thus `setStall()` and `setNak()` are race-free only on an
endpoint that is not VALID. Do not use them to cancel a pending `startIn()`
or `armOut()`.

### Packet memory

The 1024 bytes of packet memory hold the buffer descriptor table (64
bytes at offset 0) and the endpoint buffers. The driver allocates the
buffers in sequence. An OUT buffer is rounded up to the `COUNTn_RX`
block size: 2-byte blocks up to 62 bytes, 32-byte blocks above.

### Limits

- Endpoint numbers 0 to 7. Both directions of one number share one
  register, so they have the same type.
- No isochronous and no double-buffered endpoints.
- No remote wakeup, no Link Power Management (LPM) and no Battery
  Charging Detection (BCD).

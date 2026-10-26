# GPIO and FastPin

`hw::GpioPin` (`src/peripherals/GpioPin.{h,cpp}`) configures and drives one
pin. On STM32F1, `src/families/stm32f1/GpioPin.cpp` replaces the shared
implementation. `hw::FastPin` (`src/peripherals/FastPin.h`) is a
header-only `GpioPin` for bit-bang loops. Port and pin are template
arguments, so each access is one `BSRR` store or one `IDR` load.

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

## Interrupt safety

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

## FastPin direction switch

`FastPin` can change the pin direction fast, for example the SWDIO
direction at each SWD turnaround.

On the `MODER` families (L0, G0, WL, H5), `setOutput()`, `setInput()` and
`setMode<GpioPin::Mode>()` write the 2-bit `MODER` field of the pin:

```cpp
using Swdio = hw::FastPin<GPIOB_BASE, 4>;

Swdio::pin().configure(hw::GpioPin::Mode::Input, hw::GpioPin::OutputType::PushPull,
                       hw::GpioPin::Pull::Up, hw::GpioPin::Speed::VeryHigh);
Swdio::setOutput(); // push-pull output with the speed and pull from configure()
Swdio::setInput();  // input with the pull-up from configure()
```

The calls do not change `OTYPER`, `OSPEEDR`, `PUPDR` or `ODR`. Set these
once with `pin().configure()`. Each call is a read-modify-write of `MODER`
with interrupts off (`PRIMASK`), so an interrupt cannot lose a change to
another pin. The call restores the old `PRIMASK` value. In the H5 test
image, one call is 6 or 7 instructions: `mrs`, `cpsid`, load, `bic`,
`orr` (not for an input), store and `msr`. An input to
output change and an output to input change each go through no other
mode, because only `MODER` bit 0 of the pin changes.

On STM32F1, `FastPin` changes the 4-bit `CNF`/`MODE` field:

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

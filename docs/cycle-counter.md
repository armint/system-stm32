# Cycle counter

`hw::CycleCounter` in `peripherals/CycleCounter.h` gives a 32-bit counter of
core clock cycles. It reads the DWT (data watchpoint and trace) counter
`CYCCNT`. It has the names of `hw::CycleCounter` in
[ch32v00x-system](https://github.com/armint/ch32v00x-system), which reads the
SysTick counter. Thus code that measures short times compiles for both
libraries without a change.

| Function | Effect |
|---|---|
| `start()` | Sets `TRCENA` in `DEMCR` and `CYCCNTENA` in `DWT->CTRL`. It does not reset the count. |
| `count()` | Returns `DWT->CYCCNT`. The counter runs free and wraps at 2^32. |

Both functions are inline and always inline, because bit-bang timing loops
call them.

## Use

```cpp
#include <peripherals/CycleCounter.h>

hw::CycleCounter::start();
const uint32_t begin = hw::CycleCounter::count();
// ... work ...
const uint32_t cycles = hw::CycleCounter::count() - begin;
```

Use only differences of two counts. The unsigned difference is correct across
the wrap, if the time is less than 2^32 cycles (59.6 s at 72 MHz).

A debugger or other code can clear `TRCENA` or `CYCCNTENA`. The counter then
stops, and a wait for a count does not end. Call `start()` before each timed
operation. `start()` sets the bits each time and costs two read-modify-write
accesses.

## Families

| Family | Core | Counter |
|---|---|---|
| STM32F1 | Cortex-M3 | `DWT->CYCCNT`, `TRCENA` in `CoreDebug->DEMCR` |
| STM32WL | Cortex-M4 | `DWT->CYCCNT`, `TRCENA` in `CoreDebug->DEMCR` |
| STM32H5 | Cortex-M33 | `DWT->CYCCNT`, `TRCENA` in `DCB->DEMCR` |
| STM32C0, STM32G0, STM32L0 | Cortex-M0+ | None. The header stops the build with `#error`. |

The header tests `DWT_CTRL_CYCCNTENA_Msk`. The CMSIS header of a Cortex-M0+
does not define it. On Armv8-M (Cortex-M33), the CMSIS header has `DCB` in
place of `CoreDebug`. The header tests `DCB` to select the register.

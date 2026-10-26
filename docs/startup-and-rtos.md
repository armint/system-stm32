# Bare-metal start-up and an external RTOS

Most of this library works with or without an RTOS: the peripheral
drivers, the interrupt binding and the vector table. One small part is
different. The C/C++ runtime entry point and the default exception
handlers are necessary only when no other code supplies them.

That part is in `src/bare-metal/`:

| File | Contents |
|---|---|
| `startup.cpp` | `_start()`. It copies `.data`, sets `.bss` to zero, runs the C++ static constructors and calls `main()`. The weak default `Reset_Handler()` in `exception-handlers.c` calls it. Without an RTOS, this is the complete runtime entry point. |
| `assert.c` | `__assert_func()` and `assert_failed()`. They report through `trace_printf()` or semihosting, then abort. |
| `exception-handlers.c` | Weak defaults for `Reset_Handler`, `NMI_Handler`, `HardFault_Handler` and the other handlers. It also defines `dump_exception_stack()` (a register dump on a hard fault). This symbol is always strong, also without `TRACE`. |
| `trace_compat.{h,cpp}` | Weak empty `trace_printf()` and `trace_flush()`. They satisfy the trace calls of `exception-handlers.c` when no RTOS or trace back end defines them. |

An RTOS with its own start-up, for example
[µOS++ IIIe](https://github.com/micro-os-plus), supplies its own versions
of these functions, usually as weak symbols. With both copies in the
firmware, the weak symbols link, but the link order selects the copy (see
[Start-up hooks](#start-up-hooks)). `dump_exception_stack()` does not link:
µOS++ also defines it as a strong symbol when the consumer defines `TRACE`.
The result is a multiple-definition link error.

Set `SYSTEM_STM32_EXCLUDE_STARTUP=ON` when an external RTOS supplies the
start-up and the exception handling. The build then does not compile
`src/bare-metal/`:

```cmake
set(SYSTEM_STM32_EXCLUDE_STARTUP ON)
add_subdirectory(path/to/system-stm32)
```

Keep the option `OFF` (the default) for a bare-metal application. There,
`src/bare-metal/` is the only runtime.

## Start-up hooks

`src/startup-hooks.h` declares `os_startup_initialize_hardware_early()` and
`os_startup_initialize_hardware()`. `src/initialize-hardware.cpp` gives weak
default definitions. These files are not in `src/bare-metal/`. The build
always compiles them, also with `SYSTEM_STM32_EXCLUDE_STARTUP=ON`.

This does not make them safe with an RTOS. µOS++ IIIe also has weak
defaults for the same two hooks (`initialize-hardware.c` in its sources).
With µOS++ in the firmware, both weak definitions go into the link. The
link order then selects one of them. This is not a planned fallback. Only
the µOS++ copy uses `OS_HAS_SYSTEMCLOCK_CONFIG` and calls
`SystemClock_Config()`.

If your firmware needs a specific hook implementation, define the hook as
a strong (not weak) symbol. Do not rely on the link order.

## Exception-handler declarations

`src/cmsis-plus/` declares the Cortex-M exception handlers
(`Reset_Handler`, `HardFault_Handler`, ...) and the layout of the exception
stack frame. The per-family vector table needs these declarations, and the
build always compiles the vector table. Thus `src/cmsis-plus/` is not in
`src/bare-metal/`.

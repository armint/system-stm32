# Agent Instructions for system-stm32

## Purpose
This library provides the Hardware Abstraction Layer (HAL), startup code, and peripheral drivers for STM32L0 microcontrollers.

## Key Features
- **Startup**: Linker scripts and startup code (`startup.cpp`, `vectors_*.c`).
- **Peripherals**: Drivers for ADC, DMA, EXTI, GPIO, USART, Clock control and Timers.
- **System**: Delay functions, interrupt handling, and hardware initialization.

## Guidelines
- Peripheral drivers are located in `src/peripherals/`.
- When modifying or adding peripheral support, ensure it matches the hardware specification of the targeted STM32L0 MCUs.
- Use the `Interrupt` and `GpioInterrupt` utilities in `src/utils/` for managing runtime interrupt connections.
- Be mindful of the limited resources (Flash/RAM) on these microcontrollers.
- Always use descriptive variable and function names.
- Follow the coding style and conventions used in the existing codebase.
- Document any new functions or changes with clear comments.
- Ensure that changes are compatible with the embedded nature of the project (low memory footprint).
- Heap memory usage is not allowed.

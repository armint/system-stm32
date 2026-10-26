# USART

The shared driver is `src/peripherals/Usart.{h,cpp}` (USART v2: L0, G0, WL
and H5). The STM32F1 has USART v1. Its driver is in
`src/families/stm32f1/UsartV1.{h,cpp}`. `peripherals/Usart.h` includes it
when the CMSIS header defines `USART_SR_RXNE`. For the H5 additions, see
[STM32H5 USART](stm32h5.md#stm32h5-usart).

## Frame format

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

## STM32F1 USART v1

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

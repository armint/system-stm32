# STM32H5

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
- `FastPin::setOutput()`, `setInput()` and `setMode<>()` change the
  `MODER` field of the pin. See [FastPin direction switch](gpio.md#fastpin-direction-switch).
- `Uid` reads the 96-bit unique ID at `UID_BASE` (0x08FFF800). It uses 32-bit
  loads on all families. On a NUCLEO-H533RE, an 8-bit load took a bus fault
  (issue #44).
  `Dbg::freezeIwdgOnDebug()` and `Dbg::freezeWwdgOnDebug()` set the
  freeze bits in `DBGMCU_APB1FZR1`.
- `Iwdg` has the two extra H5 prescalers `Div512` and `Div1024`. On the H5,
  call `enable()` before `configure()`. The IWDG updates `PR` and `RLR` only
  while its clock (LSI) runs, and `enable()` starts LSI (RM0481 section
  49.4.4).
- After reset, the USB Type-C dead-battery pull-downs of the UCPD are on
  PB13 (CC1) and PB14 (CC2). `Pwr::disableUcpdDeadBattery()` sets
  `PWR_UCPDR.UCPD_DBDIS` and removes them. Call it before you use these
  pins, for example for SPI2.
- `_H5_EXCLUDE` in `CMakeLists.txt` removes `Rcc.cpp`, `Spi.cpp`, `Dma.cpp`,
  `Exti`, `I2c` and `GpioInterrupt`. The H5 has other register blocks for
  these: the SPI v2, GPDMA, and one EXTI IRQ for each line.
  The GPDMA has its own driver, see [STM32H5 GPDMA](#stm32h5-gpdma).
- The shared `Usart` driver works on the H5. See [STM32H5 USART](#stm32h5-usart).
- The SPI v2 driver is in `families/stm32h5/SpiV2.{h,cpp}`.
  `peripherals/Spi.h` includes it on the H5. See [STM32H5 SPI v2](#stm32h5-spi-v2).
- The ADC driver is in `families/stm32h5/Adc.{h,cpp}`. See
  [STM32H5 ADC](#stm32h5-adc).
- The RCC of the H5 has its own driver, `families/stm32h5/RccH5.{h,cpp}`.
  `peripherals/Rcc.h` includes it on the H5, so the include stays the same.
  See [STM32H5 clocks](#stm32h5-clocks).
- `hw::UsbDevice` works on the H5, with the access layer
  `families/stm32h5/UsbFsFamily.h` for the USB DRD block. See
  [STM32H5 USB DRD](usb.md#stm32h5-usb-drd).
- The CMSIS `SystemCoreClockUpdate()` of the H5 uses `float`. The start-up
  calls it, so every image links the soft-float routines (about 1.3 KB).
- Do not reset the backup domain (`RCC_BDCR.VSWRST`) after start-up. The
  reset also erases SRAM2 (errata ES0621, section 2.2.6), and SRAM2 is
  part of the RAM region of `mem-H533xE.ld`.

## STM32H5 GPDMA

The H5 has no DMA and no DMAMUX. It has GPDMA1 and GPDMA2 with eight
channels each, and a request selector in each channel. The driver is in
`families/stm32h5/GpdmaH5.{h,cpp}`. `peripherals/Dma.h` includes it on the
H5, so the include stays the same. The `Dma` namespace has other types than
on L0/G0: the H5 API does not match the DMA API of the other families.

- Enable the clock first: `Rcc::enableGPDMA1Clock()`.
- Channels are numbered from 0, as in the RM and the IRQ names. Channels 0
  to 3 have a 8-byte FIFO. Channels 4 to 7 have a 32-byte FIFO.
- `Dma::Config` holds the direction, the request (`Dma::Request`, for
  example `spi1_tx`, `usart1_rx`, `tim1_upd`), the data widths and the
  address increment. The block size is in bytes, 1 to 65535.
- `Channel::start()` runs a single transfer. `Channel::startList()` runs a
  linked list of `Dma::LinkedListItem`. Build each item with
  `LinkedListItem::make()`. All items stay in SRAM, 4-byte aligned, in the
  same 64 KiB region: align the item array to a power of 2 that is at least
  its size. An item can point to itself for a circular transfer.
- Both AHB ports stay on port 0. The 2D addressing of channels 6 and 7 is
  not used. The trigger inputs, the link step mode and the suspend mode
  are not used.
- The test image runs a memory-to-memory transfer, single and with a list.
  The transfer to a peripheral (the acceptance test of #35) is not yet
  tested on the Nucleo board.

## STM32H5 clocks

After reset, the core runs on HSI at 32 MHz (64 MHz, `HSIDIV` /2).
Two recipes set 250 MHz, the maximum of the H5:

| Function | PLL1 input | PLL1 settings |
|---|---|---|
| `Rcc::setClock250MhzHSE()` | HSE crystal (`HSE_VALUE`) | M = HSE / 2 MHz, N = 250, P = 2, Q = 4 |
| `Rcc::setClock250MhzHSI()` | `hsi_ck` (64 MHz / `HSIDIV`) | M = `hsi_ck` / 2 MHz, N = 250, P = 2, Q = 4 |

Each recipe does these steps:

1. Move the system clock to HSI.
2. Increase the voltage scaling one level at a time to VOS0.
3. Start the HSE (HSE recipe only) and PLL1.
4. Set 5 flash wait states, `WRHIGHFREQ` = 2 and the prefetch.
5. Set all bus prescalers to /1 and select PLL1 as system clock.
6. Call `SystemCoreClockUpdate()`.

The recipe returns `false` if the input clock is not a multiple of 2 MHz
(for example a 25 MHz crystal), or if PLL2 or PLL3 is on. It also returns
`false` if HSE, VOS0 or PLL1 is not ready in time. The system clock then
stays on HSI.

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
  writes to the flash. The ICACHE must not cache the read-only flash area
  0x08FFF000 to 0x08FFFFFF (UID, VREFINT_CAL, TS_CAL): a cached read takes a
  bus fault. Call `Rcc::disableCacheOfReadOnlyFlash()` before
  `Rcc::enableICache()`. It programs MPU region 0 and enables the MPU. Loads
  from this area must be 32 bits wide.
- For a 1 ms SysTick, call `SysTick_Config(SystemCoreClock / 1000)` after
  the recipe.
- The test image (`tests/stm32h5`) uses the HSE recipe. Hold B1 during reset
  to use the HSI recipe. LD2 blinks at 1 Hz from SysTick when the clock is
  correct, and at 5 Hz when the recipe fails.
- The test image also prints self-test results on the ST-LINK virtual COM
  port (USART2, PA2/PA3, 115200 8N1):
  - USART1 loopback. Connect PA9 (TX) to PA10 (RX).
  - SPI2 loopback in master mode. Connect PB15 (MOSI) to PB14 (MISO).
  - ADC1 calibration, VDDA from Vrefint, and PB0 in mV. The image prints
    PB0 again at each LD2 toggle.

  A missing wire gives `FAIL` for that test only. The SPI test needs the
  250 MHz clock, because the SPI kernel clock is `pll1_q_ck`.

## STM32H5 USART

The H5 USART has the register set of the G0 USART. The shared driver
`peripherals/Usart.{h,cpp}` has two H5 additions:

- The kernel clock mux is in `RCC_CCIPR1`, with a 3-bit field for each
  USART. `Usart::ClockSource` has the H5 values `PCLK`, `PLL2Q`, `PLL3Q`,
  `HSIKER`, `CSIKER` and `LSE`. `PCLK` is pclk2 for USART1 and pclk1 for
  USART2 and USART3.
- `getClockFrequency()` decodes the field of USART1 to USART3. It returns 0
  for `PLL2Q`, `PLL3Q` and for the other U(S)ARTs, because system-stm32
  does not start PLL2 or PLL3. `setBaudRate()` then does not change `BRR`.
- `enableFifo()` and `disableFifo()` set `CR1.FIFOEN`. They exist on all
  parts that have FIFO mode (G0 and H5). In FIFO mode, `RXNE` means "RX FIFO
  not empty" and `TXE` means "TX FIFO not full". Thus the receive and
  transmit code stays the same. `FIFOEN` is only writable while the USART
  is disabled: the functions disable it and then restore the enable bit.
- Erratum ES0621 2.15.2: after `disableDmaTx()` and `enableDmaTx()`, the
  USART sends no more DMA requests. Disable and enable the USART after the
  clear of `DMAT`.

## STM32H5 SPI v2

The SPI of the H5 is not the SPI of the other families. It has two
configuration registers (`CFG1`, `CFG2`), a FIFO with a threshold, a
transfer size counter (`TSIZE`), a master start bit (`CSTART`), separate
`TXDR` and `RXDR` registers, and flags that `IFCR` clears. It has no `BSY`
flag. Thus `hw::Spi` on the H5 has its own interface, which follows this
hardware:

| Item | Function |
|---|---|
| Configuration | `configure()`: prescaler, master or slave, CPOL, CPHA, data size (4 to 32 bits), bit order, NSS mode, direction (full-duplex, TX only, RX only, half-duplex). It disables the SPI, writes `CFG1`/`CFG2`, sets `TSIZE` = 0 and restores `SPE`. |
| Single setters | `setBaudRatePrescaler()`, `setDataSize()`, `setFifoThreshold()`, `setTransferSize()`, `enableTxDma()`, ... Only while the SPI is disabled: the hardware ignores writes to `CFG1`, `CFG2` and `CR2` while `SPE` = 1. |
| Master transfer | `enable()`, then `startTransfer()` (`CSTART`). `suspend()`, `setHalfDuplexDirection()`. |
| Data | `write()` and `read()`. The access width (8, 16 or 32 bits) follows the data size, so that one access is one frame. |
| Blocking frame | `transfer()`: one full-duplex frame in master mode. It sets `CSTART` if necessary. |
| End of transfer | `waitUntilIdle()` waits for `TXC`. Call it before `disable()`. |
| Flags | `isTxPacketSpaceAvailable()` (`TXP`), `isRxPacketAvailable()` (`RXP`), `isEndOfTransfer()`, `isTxComplete()`, `isOverrun()`, `isUnderrun()`, `isModeFault()`, `remainingFrames()`, and a `clear...()` function for each `IFCR` bit. |
| Interrupts | RXP, TXP, end of transfer (EOT, SUSP, TXC), errors (OVR, UDR, MODF). |
| Clock | `getClockFrequency()`: `pll1_q_ck` (125 MHz) for SPI1 to SPI3 with the reset mux, pclk2 for SPI4 with the reset mux. HSI, CSI and HSE for SPI4 too. 0 for PLL2, PLL3, AUDIOCLK and per_ck. |

- With `TSIZE` = 0 the transfer is endless. In full-duplex and TX-only
  mode, the master sends a frame each time the TX FIFO has data. It stops
  the clock when the TX FIFO is empty. With `TSIZE` > 0 the hardware clears
  `CSTART` and sets `EOT` after `TSIZE` frames.
- `NssMode::Soft` sets `SSM`. The internal slave select (`SSI`) is then
  inactive for a master (no mode fault) and active for a slave.
  `setNssMode(nss, mode)` takes the mode as an argument. Call it before
  `setMode(Mode::Master)`, because a master with `SSI` = 0 takes a mode fault.
  To switch to slave, call `setMode(Mode::Slave)` first.
  `configure()` does this and clears a pending `MODF`.
- `enableAlternateFunctionControl()` (`AFCNTR`) keeps SCK, MOSI and NSS at
  their idle levels while the SPI is disabled. Use it in master mode only.
- SPI1 to SPI3 take 4 to 32 data bits and have a 16-byte FIFO. SPI4 takes
  4 to 16 data bits and has an 8-byte FIFO.
- Erratum ES0621 2.17.2: a disable directly after `EOT` can cut the last
  SCK pulse. At a high PCLK/SCK ratio, wait half an SCK period between
  `waitUntilIdle()` and `disable()`.

## STM32H5 ADC

The H5 ADC is the L4/G4 type of ADC, not the G0 type. It has a sequencer
for each group, a sample time for each channel, and a deep power-down
mode. `Adc` (`families/stm32h5/Adc.h`) gives access to all functions of
the block:

| Area | Functions |
|---|---|
| Clock (static) | `enableClock()`, `reset()`, `setKernelClock()` (`RCC_CCIPR5.ADCDACSEL`), `setClockMode()` (`CKMODE`, `PRESC`), `configureClock()`, `clockHz()` |
| Common (static) | `enableVrefint()`, `enableTemperatureSensor()`, `enableVbat()`, `setDualMode()`, `dualValue()` |
| Power and calibration | `powerUp()`, `powerDown()`, `calibrate(InputMode)`, `getCalibrationFactors()`, `setCalibrationFactors()`, `enable()`, `disable()` |
| Channels | `setSamplingTime()`, `setSamplingTimePlusOne()` (`SMPPLUS`), `setInputMode()` (single-ended or differential), `setChannel0Switch()` (`ADC_OR.OP0`) |
| Regular group | `setRegularRank()` (ranks 1 to 16), `setRegularLength()`, `setResolution()`, `setAlignment()`, `setConversionMode()`, `setDiscontinuousCount()`, `setOverrunMode()`, `setDmaMode()`, `setAutoDelay()`, `setRegularTrigger()`, `setBulbSampling()`, `setSamplingTimeControlTrigger()`, `setSamplingSoftwareTrigger()`, `start()`, `stop()`, `value()` |
| Injected group | `setInjectedSequence()` (1 to 4 ranks and the trigger in one `JSQR` write), `setInjectedQueue()`, `setInjectedAuto()`, `setInjectedDiscontinuous()`, `startInjected()`, `stopInjected()`, `injectedValue()` |
| Offsets | `setOffset()` (4 units: subtract or add, saturation), `disableOffset()` |
| Oversampling | `setOversampling()` (ratio 2 to 256, shift 0 to 8, regular and injected, triggered, continued or resumed) |
| Analog watchdogs | `setWatchdog1()` (12-bit thresholds, one channel or all, filter), `setWatchdog23()` (channel set, 8-bit thresholds) |
| Events | `isSet()`, `clear()`, `enableInterrupts()`, `disableInterrupts()` with the `Adc::Event` bits |
| Helpers | `convert()` (one blocking software conversion), `readSupplyMillivolts()`, `toMillivolts()`, `supplyFromVrefint()`, `Calibration` (factory values) |

Start-up sequence:

1. `Adc::enableClock()` and `Adc::configureClock()`. The synchronous clock
   is hclk / 4 at 250 MHz, thus 62.5 MHz. The maximum ADC clock is 75 MHz
   (DS14539).
2. `powerUp()`: the ADC leaves deep power-down. The regulator then needs
   10 µs.
3. `calibrate()`: once for single-ended inputs, once more with
   `InputMode::Differential` if you use differential inputs.
4. `enable()`.

- The waits of the driver stop after `Adc::pollLimit` iterations and then
  return `false`.
- The `Channel` values are the ADC1 `VINP` numbers of the STM32H533. They
  do not follow the pin order (for example PB0 is channel 9). PA0 also
  needs `setChannel0Switch(true)`.
- `Calibration::vrefint()`, `ts1()` and `ts2()` read the factory values at
  0x08FFF810, 0x08FFF814 and 0x08FFF818 (VDDA = 3.3 V). `readSupplyMillivolts()`
  calculates VDDA = 3.3 V × `VREFINT_CAL` / raw. It sets a sample time of
  at least 5 µs for Vrefint.
- `TemperatureSensorCalibration` gives the same names as on G0, for the
  `InternalTemperatureSensor` of libArT.
- The hardware allows most writes only while no conversion runs, and some
  only while the ADC is disabled (RM0481 section 28.4.10). The driver does
  not check these rules.
- With the injected queue on (`setInjectedQueue(true)`), the injected
  group needs a hardware trigger. `setInjectedSequence()` refuses
  `TriggerEdge::Software` then and returns `false`.
- Errata ES0621 2.6.1 and 2.6.2 apply to the injected queue. Erratum 2.6.5:
  before `stopInjected()`, wait for `Event::InjectedEndOfSequence`.
- With `OverrunMode::Preserve` (the reset value), `ADC_DR` is a 3-stage
  FIFO. `convert()` sets `Overwrite` while it runs, so it never returns an
  older unread result.
- `readSupplyMillivolts()` works on ADC1 only. On ADC2, channel 17 is
  VDDCORE.

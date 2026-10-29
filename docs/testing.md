# Tests and CI

The repository has three kinds of checks:

- CI builds the library for each family.
- Test images link a complete firmware for STM32F1 and STM32H5.
- Host tests run on the build machine, with fake registers.

## CI

CI (`.gitlab-ci.yml`) builds the `system-stm32` object library standalone
for each family on each push. A change can compile for the family of one
downstream project and break another family. Only this build finds that
problem.

Each family builds in two jobs:

| Job | Standard | Check |
|---|---|---|
| `build` | C++23 | The job fails if CMake gets span-compat. Thus `hw::span` is `std::span`. |
| `build-cpp17` | C++17 | A compile with `-Wpedantic -Werror -fno-exceptions -fno-rtti` rejects C++20 constructs. Then the normal library build runs. The job fails if CMake does not use span-compat with `SPAN_COMPAT_USE_STD_SPAN=0`. |

Other jobs:

| Job | Contents |
|---|---|
| `build-exclude-startup` | STM32WL with `SYSTEM_STM32_EXCLUDE_STARTUP=ON`. |
| `build-f1-test-image`, `build-h5-test-image` | The test images, at C++23 and at C++17. |
| `host-tests` | The host tests. |

The C++17 jobs clone span-compat with the CI job token. Thus span-compat
must allow job tokens from this project.

## Test images

Enable a test image with `-D SYSTEM_STM32_BUILD_TEST_IMAGE=ON`. It is
available for `STM32_FAMILY=STM32F1` and `STM32_FAMILY=STM32H5`. Add each
new driver of these families to the test image.

| Directory | Contents |
|---|---|
| `tests/stm32f1/` | A minimal `main()`. CI links it for the F103x8 and the F103xB. CI also links `test-image-f103x8-app` with the [linker script parameters](building.md#linker-script-parameters) and checks the addresses. The image does not run on hardware. |
| `tests/stm32h5/` | CI links it for the H533xE. On a NUCLEO-H533RE, it sets 250 MHz and blinks LD2 (PA5) with `FastPin`. The IWDG (about 2 s) resets the board if the main loop stops. For the self-tests on the virtual COM port, see [STM32H5 clocks](stm32h5.md#stm32h5-clocks). |

## Host tests

`tests/host/` holds unit tests that run on the build machine, with fake
registers. They test the code that needs no hardware. Today they test
this code:

- GPIO on the `MODER` families: the `FastPin` direction switch and the
  `AFR` mapping of `GpioPin`. `FastPin` needs a constant port address, so
  the test maps a fake port at `GPIOA_BASE` with `mmap()` (Linux only).
- STM32F1 GPIO: the `CNF`/`MODE` encoding, `GpioPin` on `CRL`/`CRH`, `toggle()`,
  and the address and store order of the `FastPin` direction switch.
- USB device driver (the shared core in `src/peripherals/usbfs/`). The
  tests check these items:
  - The `COUNTn_RX` encoding.
  - The packet memory allocation and alignment.
  - The buffer descriptor table and the packet copy.
  - Double-buffered bulk OUT and the D+ pull-up.
  - The `USB_EPnR` write encoding. `CTR` bits are written as 1, `STAT` as
    `old ^ wanted` and `DTOG` as 0. The configuration bits stay the same.

  The same test (`test_usb_fs.cpp`) runs once with each family access
  layer:
  - STM32F1: 32-bit CPU stride, stub `stubs/`.
  - STM32L0: 16-bit stride and `USB_BCDR`, stub `stubs-usb-l0/`.
  - STM32H5: USB DRD with 32-bit access, `USB_CHEPnR` and VDDUSB, stub
    `stubs-usb-h5/`.
- STM32H5 GPDMA driver: the `CTR1` and `CTR2` encoding, the linked-list
  items, and the register writes of `Channel::start()` and `startList()`.
- `Uid`: the byte order of `read()` and its agreement with `word()`. The
  test cannot check the 32-bit access width.

The fake USB registers are plain memory. A register holds the last written
value. The fake does not model the toggle bits and the `rc_w0` bits of the
hardware. Thus the host tests do not test these items:

- The endpoint state after a call with more than one `USB_EPnR` write.
- The interrupt handler.

The CI job `host-tests` runs them. They need a host C++ compiler, not the
ARM toolchain:

```sh
cmake -S tests/host -B build-host
cmake --build build-host
ctest --test-dir build-host --output-on-failure
```

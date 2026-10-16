# Host unit tests for the parts of system-stm32 that do not need hardware.
# A native build, separate from the firmware build:
#   cmake -S tests/host -B build-host && cmake --build build-host && ctest --test-dir build-host
cmake_minimum_required(VERSION 3.20)
project(system-stm32-host-tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(_src ${CMAKE_CURRENT_SOURCE_DIR}/../../src)

enable_testing()

add_executable(test-gpio-f1
        test_gpio_f1.cpp
        ${_src}/families/stm32f1/GpioPin.cpp)
# The stubs come first, so <cmsis_device.h> is the host stand-in.
target_include_directories(test-gpio-f1 PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/stubs
        ${_src}
        ${_src}/families/stm32f1)
target_compile_options(test-gpio-f1 PRIVATE -Wall -Wextra -Werror -Wno-volatile)
add_test(NAME gpio-f1 COMMAND test-gpio-f1)

add_executable(test-usb-f1
        test_usb_f1.cpp
        ${_src}/families/stm32f1/UsbDevice.cpp)
target_include_directories(test-usb-f1 PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/stubs
        ${_src}
        ${_src}/families/stm32f1)
target_compile_options(test-usb-f1 PRIVATE -Wall -Wextra -Werror -Wno-volatile)
add_test(NAME usb-f1 COMMAND test-usb-f1)

add_executable(test-gpio-moder
        test_gpio_moder.cpp
        ${_src}/peripherals/GpioPin.cpp)
target_include_directories(test-gpio-moder PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/stubs-moder
        ${_src})
target_compile_options(test-gpio-moder PRIVATE -Wall -Wextra -Werror -Wno-volatile)
add_test(NAME gpio-moder COMMAND test-gpio-moder)

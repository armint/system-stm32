cmake_minimum_required(VERSION 3.20)
# Extract git revision
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/generate_version.cmake")

Include(FetchContent)

# Default to STM32L0 if not specified
set(STM32_FAMILY "STM32L0" CACHE STRING "STM32 family to build for (e.g., STM32L0, STM32C0, STM32G0, STM32WL, STM32F1)")
set(STM32_DEVICE "STM32L073xx" CACHE STRING "STM32 device define for CMSIS (e.g., STM32L073xx, STM32G071xx, STM32G0B1xx, STM32WLE5xx, STM32F103xB)")

# Use a family-specific FetchContent name so different STM32 families
# can coexist in a shared FETCHCONTENT_BASE_DIR.
string(TOLOWER ${STM32_FAMILY} _stm32_family_lower)
string(REPLACE "stm32" "" _stm32_family_suffix "${_stm32_family_lower}")
set(_cmsis_device_name "cmsis_device_${_stm32_family_suffix}")

if (STM32_FAMILY STREQUAL "STM32L0")
    FetchContent_Declare(
            ${_cmsis_device_name}
            GIT_REPOSITORY https://github.com/STMicroelectronics/cmsis_device_l0.git
            GIT_TAG v1.9.4
    )
elseif (STM32_FAMILY STREQUAL "STM32C0")
    FetchContent_Declare(
            ${_cmsis_device_name}
            GIT_REPOSITORY https://github.com/STMicroelectronics/cmsis_device_c0.git
            GIT_TAG v1.0.0
    )
elseif (STM32_FAMILY STREQUAL "STM32G0")
    FetchContent_Declare(
            ${_cmsis_device_name}
            GIT_REPOSITORY https://github.com/STMicroelectronics/cmsis_device_g0.git
            GIT_TAG v1.4.4
    )
elseif (STM32_FAMILY STREQUAL "STM32WL")
    FetchContent_Declare(
            ${_cmsis_device_name}
            GIT_REPOSITORY https://github.com/STMicroelectronics/cmsis_device_wl.git
            GIT_TAG v1.4.0
    )
elseif (STM32_FAMILY STREQUAL "STM32F1")
    FetchContent_Declare(
            ${_cmsis_device_name}
            GIT_REPOSITORY https://github.com/STMicroelectronics/cmsis_device_f1.git
            GIT_TAG v4.3.5
    )
endif()

FetchContent_GetProperties(${_cmsis_device_name})
if (NOT ${_cmsis_device_name}_POPULATED)
    FetchContent_MakeAvailable(${_cmsis_device_name})
endif ()

# Alias for the rest of this file
set(cmsis_device_SOURCE_DIR "${${_cmsis_device_name}_SOURCE_DIR}")

# STM32WL uses a Cortex-M4 application core, STM32F1 a Cortex-M3; all other
# supported families use Cortex-M0+.
if (STM32_FAMILY STREQUAL "STM32WL")
    set(_cmsis_core_variant "cm4")
elseif (STM32_FAMILY STREQUAL "STM32F1")
    set(_cmsis_core_variant "cm3")
else()
    set(_cmsis_core_variant "cm0")
endif()

# Like the device package, the core checkout gets a core-specific
# FetchContent name. A shared FETCHCONTENT_BASE_DIR would otherwise hand a
# Cortex-M0+ checkout to a Cortex-M3 or Cortex-M4 build.
set(_cmsis_core_name "cmsis_core_${_cmsis_core_variant}")

FetchContent_Declare(
        ${_cmsis_core_name}
        GIT_REPOSITORY https://github.com/STMicroelectronics/cmsis_core.git
        GIT_TAG v5.4.0_${_cmsis_core_variant}
)

FetchContent_GetProperties(${_cmsis_core_name})
if (NOT ${_cmsis_core_name}_POPULATED)
    FetchContent_MakeAvailable(${_cmsis_core_name})
endif ()

# Alias for the rest of this file
set(cmsis_core_SOURCE_DIR "${${_cmsis_core_name}_SOURCE_DIR}")

project(system-stm32
        LANGUAGES C CXX ASM
        VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_BUILD}"
)

# Provides CMAKE_INSTALL_*DIR defaults for the install() calls below. A
# consuming superproject may already have included this; the module guards
# against being processed twice.
include(GNUInstallDirs)

# Add our CMAKE modules folder
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/generate_version.cmake")

# Compiler settings
set(CMAKE_CXX_STANDARD 23)
#set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CXX_DIALECT gnu)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_C_DIALECT gnu)

if (STM32_FAMILY STREQUAL "STM32L0")
    file(GLOB_RECURSE STM32_SOURCES "${cmsis_device_SOURCE_DIR}/Source/Templates/system_stm32l0xx.c")
elseif (STM32_FAMILY STREQUAL "STM32C0")
    file(GLOB_RECURSE STM32_SOURCES "${cmsis_device_SOURCE_DIR}/Source/Templates/system_stm32c0xx.c")
elseif (STM32_FAMILY STREQUAL "STM32G0")
    file(GLOB_RECURSE STM32_SOURCES "${cmsis_device_SOURCE_DIR}/Source/Templates/system_stm32g0xx.c")
elseif (STM32_FAMILY STREQUAL "STM32WL")
    file(GLOB_RECURSE STM32_SOURCES "${cmsis_device_SOURCE_DIR}/Source/Templates/system_stm32wlxx.c")
elseif (STM32_FAMILY STREQUAL "STM32F1")
    file(GLOB_RECURSE STM32_SOURCES "${cmsis_device_SOURCE_DIR}/Source/Templates/system_stm32f1xx.c")
endif()

# Gather all project sources
file(GLOB_RECURSE PROJECT_SOURCES
        "${PROJECT_SOURCE_DIR}/src/*.c"
        "${PROJECT_SOURCE_DIR}/src/*.cpp"
)

# Exclude family-specific files from the general GLOB
file(GLOB_RECURSE ALL_FAMILY_SOURCES "${PROJECT_SOURCE_DIR}/src/families/*.c" "${PROJECT_SOURCE_DIR}/src/families/*.cpp")
if(ALL_FAMILY_SOURCES)
    list(REMOVE_ITEM PROJECT_SOURCES ${ALL_FAMILY_SOURCES})
endif()

# Re-add only the selected family sources
string(TOLOWER ${STM32_FAMILY} FAMILY_LOWER)
file(GLOB_RECURSE ACTIVE_FAMILY_SOURCES "${PROJECT_SOURCE_DIR}/src/families/${FAMILY_LOWER}/*.c" "${PROJECT_SOURCE_DIR}/src/families/${FAMILY_LOWER}/*.cpp")
if(ACTIVE_FAMILY_SOURCES)
    list(APPEND PROJECT_SOURCES ${ACTIVE_FAMILY_SOURCES})
endif()

# Exclude src/bare-metal/* from the general GLOB. Those files (startup.cpp,
# assert.c, exception-handlers.c, trace_compat.*) are only meaningful when
# nothing else provides a C/C++ runtime entry point and exception handlers —
# re-added below unless the consumer supplies its own (e.g. µOS++ IIIe).
file(GLOB_RECURSE BARE_METAL_SOURCES "${PROJECT_SOURCE_DIR}/src/bare-metal/*.c" "${PROJECT_SOURCE_DIR}/src/bare-metal/*.cpp")
if(BARE_METAL_SOURCES)
    list(REMOVE_ITEM PROJECT_SOURCES ${BARE_METAL_SOURCES})
endif()

# When the consumer provides its own startup (e.g. µOS++ IIIe), leave
# src/bare-metal/ out to avoid multiple-definition errors.
# exception-handlers.c matters even though its Handler functions are weak:
# it defines a non-weak dump_exception_stack() unconditionally, and µOS++
# defines its own non-weak dump_exception_stack() whenever the consumer
# defines TRACE — so the two land in the same link and clash as soon as
# both are present.
option(SYSTEM_STM32_EXCLUDE_STARTUP
    "Exclude src/bare-metal/ (use when an external RTOS provides startup, assert, and exception handling)" OFF)
if(NOT SYSTEM_STM32_EXCLUDE_STARTUP)
    list(APPEND PROJECT_SOURCES ${BARE_METAL_SOURCES})
endif()

# Exclude peripheral files whose register-level code is not yet ported to
# STM32WL.  Rcc.cpp, I2c.cpp, Exti.cpp, and Spi.cpp have been updated with
# WL-specific branches and are included unconditionally.
if(STM32_FAMILY STREQUAL "STM32WL")
    set(_WL_EXCLUDE
        "${PROJECT_SOURCE_DIR}/src/utils/GpioInterrupt.cpp"
    )
    foreach(_excl ${_WL_EXCLUDE})
        list(REMOVE_ITEM PROJECT_SOURCES "${_excl}")
    endforeach()
endif()

# Exclude peripheral files whose register-level code does not match the
# STM32F1 peripherals. F1 has the older I2C and RTC blocks (no I2C v2 TIMINGR,
# no RTC calendar/wakeup timer), a GPIO port with CRL/CRH instead of MODER,
# USART v1 (SR/DR instead of ISR/RDR/TDR) and EXTI IRQs per line. The GPIO,
# EXTI and USART ports come in separate changes.
if(STM32_FAMILY STREQUAL "STM32F1")
    set(_F1_EXCLUDE
        "${PROJECT_SOURCE_DIR}/src/peripherals/Exti.cpp"
        "${PROJECT_SOURCE_DIR}/src/peripherals/GpioPin.cpp"
        "${PROJECT_SOURCE_DIR}/src/peripherals/I2c.cpp"
        "${PROJECT_SOURCE_DIR}/src/peripherals/Rtc.cpp"
        "${PROJECT_SOURCE_DIR}/src/peripherals/Usart.cpp"
        "${PROJECT_SOURCE_DIR}/src/utils/GpioInterrupt.cpp"
        # The Cortex-M0+ busy loop; F1 has its own DWT version in families/stm32f1.
        "${PROJECT_SOURCE_DIR}/src/delaybusywait.cpp"
    )
    foreach(_excl ${_F1_EXCLUDE})
        list(REMOVE_ITEM PROJECT_SOURCES "${_excl}")
    endforeach()
endif()

file(GLOB_RECURSE PROJECT_HEADERS
        "src/*.h"
        "src/*.hpp")

file(GLOB PROJECT_INCLUDE_DIRS
        ${CMAKE_CURRENT_BINARY_DIR}
        src
        src/families/${FAMILY_LOWER}
)

add_library(system-stm32 OBJECT ${PROJECT_SOURCES} ${STM32_SOURCES})

# Select device and family macros for CMSIS headers so sources compile
target_compile_definitions(system-stm32 PUBLIC ${STM32_FAMILY} ${STM32_DEVICE})

# STM32WL uses SRAM1_BASE (no generic SRAM_BASE alias) and has 78 vectors
# (16 core exceptions + 62 device IRQs).
if(STM32_FAMILY STREQUAL "STM32WL")
    target_compile_definitions(system-stm32 PRIVATE
        SRAM_BASE=SRAM1_BASE
        VECTOR_TABLE_SIZE=78
    )
endif()

target_include_directories(
        system-stm32
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/families/${FAMILY_LOWER}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_include_directories(system-stm32
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Export CMSIS headers to dependents as part of the public interface
target_include_directories(system-stm32
        PUBLIC
            $<BUILD_INTERFACE:${cmsis_device_SOURCE_DIR}/Include>
            $<BUILD_INTERFACE:${cmsis_core_SOURCE_DIR}/Core/Include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/cmsis_device/Include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/cmsis_core/Core/Include>
)

target_compile_features(system-stm32 PUBLIC cxx_std_23)

# If a central build configuration target exists in the superproject,
# link it so this OBJECT library is compiled with the same flags/defines.
if (TARGET art-build-config)
    # Only affect this target's compilation in the build tree; do not appear in installed export
    target_link_libraries(system-stm32 PRIVATE $<BUILD_INTERFACE:art-build-config>)
endif ()

# Ensure the exported interface does not pull non-exported targets.
set_property(TARGET system-stm32 PROPERTY INTERFACE_LINK_LIBRARIES "")

option(SYSTEM_STM32_BUILD_TEST_IMAGE
    "Build the STM32F1 test image (STM32_FAMILY=STM32F1 only; needs an ARM toolchain)" OFF)
if(SYSTEM_STM32_BUILD_TEST_IMAGE)
    if(NOT STM32_FAMILY STREQUAL "STM32F1")
        message(FATAL_ERROR "SYSTEM_STM32_BUILD_TEST_IMAGE needs STM32_FAMILY=STM32F1")
    endif()
    add_subdirectory(tests/stm32f1)
endif()

install(TARGETS system-stm32
        EXPORT system-stm32-targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
# Memory linker scripts. A consumer of the installed package adds
# -L<prefix>/share/system-stm32/ldscripts and -T<script>.
install(DIRECTORY ldscripts/ DESTINATION ${CMAKE_INSTALL_DATADIR}/system-stm32/ldscripts)

# Install the CMSIS headers so consumers can include them from the installed package
install(DIRECTORY ${cmsis_device_SOURCE_DIR}/Include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cmsis_device/Include)
install(DIRECTORY ${cmsis_core_SOURCE_DIR}/Core/Include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cmsis_core/Core/Include)

install(EXPORT system-stm32-targets
        NAMESPACE art::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/system-stm32)

# Generate a Config file so consumers can `find_package(system-stm32)`
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/system-stm32ConfigVersion.cmake
        VERSION ${GIT_VERSION}
        COMPATIBILITY SameMajorVersion)

configure_package_config_file(
        ${CMAKE_CURRENT_LIST_DIR}/cmake/system-stm32Config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/system-stm32Config.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/system-stm32)

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/system-stm32Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/system-stm32ConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/system-stm32)

cmake_minimum_required(VERSION 3.20)
# Extract git revision
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/generate_version.cmake")

Include(FetchContent)

FetchContent_Declare(
        cmsis_device_l0
        GIT_REPOSITORY https://github.com/STMicroelectronics/cmsis_device_l0.git
        GIT_TAG v1.9.4
)

FetchContent_GetProperties(cmsis_device_l0)
if (NOT cmsis_device_l0_POPULATED)
    FetchContent_MakeAvailable(cmsis_device_l0)
endif ()

FetchContent_Declare(
        cmsis5
        GIT_REPOSITORY https://github.com/STMicroelectronics/cmsis_core.git
        GIT_TAG v5.4.0_cm0
)

FetchContent_GetProperties(cmsis5)
if (NOT cmsis5_POPULATED)
    FetchContent_MakeAvailable(cmsis5)
endif ()


project(system-stm32
        LANGUAGES C CXX ASM
        VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_BUILD}"
)


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

file(GLOB_RECURSE STM32_SOURCES
        "${cmsis_device_l0_SOURCE_DIR}/Source/Templates/system_stm32l0xx.c")

file(GLOB_RECURSE PROJECT_SOURCES
        "${PROJECT_SOURCE_DIR}/src/*.c"
        "${PROJECT_SOURCE_DIR}/src/*.cpp"
)


file(GLOB_RECURSE PROJECT_HEADERS
        "src/*.h"
        "src/*.hpp")

file(GLOB PROJECT_INCLUDE_DIRS
        ${CMAKE_CURRENT_BINARY_DIR}
        src
)

file(GLOB ADDITIONAL_INCLUDE_DIRS
        ${cmsis_device_l0_SOURCE_DIR}/Include
        ${cmsis5_SOURCE_DIR}/Core/Include
)

# Detect if we are crosscompiling, or compiling for our native system.
IF (CMAKE_CROSSCOMPILING)
    IF (NOT CMAKE_BUILD_TYPE)
        SET(CMAKE_BUILD_TYPE Release)
        MESSAGE(STATUS "CMAKE_BUILD_TYPE not set, using default: " ${CMAKE_BUILD_TYPE})
    ENDIF ()
    IF (NOT BOARD)
        SET(BOARD "NucleoL073")
        MESSAGE(STATUS "No BOARD specified, using default: " ${BOARD})
    ENDIF ()

    MESSAGE(STATUS "Building stm32-template version " ${FIRMWARE_VERSION} " (" ${CMAKE_BUILD_TYPE} ") for board: " ${BOARD})


    set(PROJECT_DEFINES )
    set(DEBUG_DEFINES DISABLE_WATCHDOG)


else ()
endif ()
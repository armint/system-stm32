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

    MESSAGE(STATUS "Building system-stm32 version " ${FIRMWARE_VERSION} " (" ${CMAKE_BUILD_TYPE} ") for board: " ${BOARD})

    file(GLOB_RECURSE PROJECT_SOURCES
            "${PROJECT_SOURCE_DIR}/src/*.c"
            "${PROJECT_SOURCE_DIR}/src/*.cpp"
    )

    file(GLOB_RECURSE PROJECT_HEADERS
            "src/*.h"
            "src/*.hpp"
    )
else ()
endif ()

add_library(system-stm32 STATIC ${PROJECT_SOURCES})

# Select device macro for CMSIS headers so sources compile
IF (CMAKE_CROSSCOMPILING)
    if (BOARD MATCHES NucleoL073)
        target_compile_definitions(system-stm32 PUBLIC STM32L073xx)
    elseif (BOARD MATCHES NucleoL053)
        target_compile_definitions(system-stm32 PUBLIC STM32L053xx)
    endif ()
ENDIF ()

target_include_directories(
        system-stm32
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)


target_include_directories(system-stm32
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Export CMSIS headers to dependents as part of the public interface
target_include_directories(system-stm32
        PUBLIC
            $<BUILD_INTERFACE:${cmsis_device_l0_SOURCE_DIR}/Include>
            $<BUILD_INTERFACE:${cmsis5_SOURCE_DIR}/Core/Include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/cmsis_device_l0/Include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/cmsis_core/Core/Include>
)

target_compile_features(system-stm32 PUBLIC cxx_std_23)

# Ensure the exported interface does not pull non-exported targets.
set_property(TARGET system-stm32 PROPERTY INTERFACE_LINK_LIBRARIES "")

install(TARGETS system-stm32
        EXPORT system-stm32-targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Install the CMSIS headers so consumers can include them from the installed package
install(DIRECTORY ${cmsis_device_l0_SOURCE_DIR}/Include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cmsis_device_l0/Include)
install(DIRECTORY ${cmsis5_SOURCE_DIR}/Core/Include/
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


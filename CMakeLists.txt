cmake_minimum_required(VERSION 3.20)
# Extract git revision
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/generate_version.cmake")

Include(FetchContent)

# Default to STM32L0 if not specified
set(STM32_FAMILY "STM32L0" CACHE STRING "STM32 family to build for (e.g., STM32L0, STM32C0, STM32G0)")

if (STM32_FAMILY STREQUAL "STM32L0")
    FetchContent_Declare(
            cmsis_device
            GIT_REPOSITORY https://github.com/STMicroelectronics/cmsis_device_l0.git
            GIT_TAG v1.9.4
    )
elseif (STM32_FAMILY STREQUAL "STM32C0")
    FetchContent_Declare(
            cmsis_device
            GIT_REPOSITORY https://github.com/STMicroelectronics/cmsis_device_c0.git
            GIT_TAG v1.0.1
    )
elseif (STM32_FAMILY STREQUAL "STM32G0")
    FetchContent_Declare(
            cmsis_device
            GIT_REPOSITORY https://github.com/STMicroelectronics/cmsis_device_g0.git
            GIT_TAG v1.4.4
    )
endif()

FetchContent_GetProperties(cmsis_device)
if (NOT cmsis_device_POPULATED)
    FetchContent_MakeAvailable(cmsis_device)
endif ()

FetchContent_Declare(
        cmsis_core
        GIT_REPOSITORY https://github.com/STMicroelectronics/cmsis_core.git
        GIT_TAG v5.4.0_cm0
)

FetchContent_GetProperties(cmsis_core)
if (NOT cmsis_core_POPULATED)
    FetchContent_MakeAvailable(cmsis_core)
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

if (STM32_FAMILY STREQUAL "STM32L0")
    file(GLOB_RECURSE STM32_SOURCES "${cmsis_device_SOURCE_DIR}/Source/Templates/system_stm32l0xx.c")
elseif (STM32_FAMILY STREQUAL "STM32C0")
    file(GLOB_RECURSE STM32_SOURCES "${cmsis_device_SOURCE_DIR}/Source/Templates/system_stm32c0xx.c")
elseif (STM32_FAMILY STREQUAL "STM32G0")
    file(GLOB_RECURSE STM32_SOURCES "${cmsis_device_SOURCE_DIR}/Source/Templates/system_stm32g0xx.c")
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

file(GLOB_RECURSE PROJECT_HEADERS
        "src/*.h"
        "src/*.hpp")

file(GLOB PROJECT_INCLUDE_DIRS
        ${CMAKE_CURRENT_BINARY_DIR}
        src
        src/families/${FAMILY_LOWER}
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
endif ()

add_library(system-stm32 OBJECT ${PROJECT_SOURCES} ${STM32_SOURCES})

# Select device macro for CMSIS headers so sources compile
target_compile_definitions(system-stm32 PUBLIC ${STM32_FAMILY})

IF (CMAKE_CROSSCOMPILING)
    if (BOARD MATCHES NucleoL073)
        target_compile_definitions(system-stm32 PUBLIC STM32L073xx)
    elseif (BOARD MATCHES NucleoL053)
        target_compile_definitions(system-stm32 PUBLIC STM32L053xx)
    elseif (BOARD MATCHES NucleoG071)
        target_compile_definitions(system-stm32 PUBLIC STM32G071xx)
    elseif (STM32_FAMILY STREQUAL "STM32G0")
        target_compile_definitions(system-stm32 PUBLIC STM32G071xx)
    endif ()
ENDIF ()

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

install(TARGETS system-stm32
        EXPORT system-stm32-targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

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

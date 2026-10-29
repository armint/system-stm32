# STM32F1 test image: links the library with a minimal main() and the linker
# scripts of this repository. Built only on request, see
# SYSTEM_STM32_BUILD_TEST_IMAGE in the top-level CMakeLists.txt.
foreach(_dev x8 xB)
    set(_name test-image-f103${_dev})
    add_executable(${_name} main.cpp)
    target_link_libraries(${_name} PRIVATE system-stm32)
    target_link_options(${_name} PRIVATE
        -nostartfiles
        --specs=nano.specs
        --specs=nosys.specs
        -Wl,--gc-sections
        -Wl,--no-warn-rwx-segments
        -Wl,-Map=${_name}.map
        -L${PROJECT_SOURCE_DIR}/ldscripts
        -Tmem-F103${_dev}.ld
        -Tlibs.ld
        -Tsections.ld)
    add_custom_command(TARGET ${_name} POST_BUILD
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${_name}>)
endforeach()

# The same image behind an 8 KB bootloader, with 1 KB reserved at the end of
# the flash and a 2 KB main stack: checks the --defsym parameters of the
# linker scripts. They must come before the -T options.
set(_name test-image-f103x8-app)
add_executable(${_name} main.cpp)
target_link_libraries(${_name} PRIVATE system-stm32)
target_link_options(${_name} PRIVATE
    -nostartfiles
    --specs=nano.specs
    --specs=nosys.specs
    -Wl,--gc-sections
    -Wl,--no-warn-rwx-segments
    -Wl,-Map=${_name}.map
    -Wl,--defsym=__flash_offset=0x2000
    -Wl,--defsym=__flash_reserved_end=0x400
    -Wl,--defsym=__main_stack_size=2048
    -Wl,--defsym=__minimum_stack_size=2048
    -L${PROJECT_SOURCE_DIR}/ldscripts
    -Tmem-F103x8.ld
    -Tlibs.ld
    -Tsections.ld)
add_custom_command(TARGET ${_name} POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${_name}>)

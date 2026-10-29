# STM32H5 test image: links the library with a minimal main() and the linker
# scripts of this repository. Built only on request, see
# SYSTEM_STM32_BUILD_TEST_IMAGE in the top-level CMakeLists.txt.
set(_name test-image-h533xE)
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
    -Tmem-H533xE.ld
    -Tlibs.ld
    -Tsections.ld)
add_custom_command(TARGET ${_name} POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${_name}>)

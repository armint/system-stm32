#pragma once

// The one span type of the public interface of this library: hw::span.
//
// std::span exists only from C++20 on. Below C++20, CMake links the
// span-compat library and defines SYSTEM_STM32_USE_SPAN_COMPAT, so hw::span
// is span_compat::span. From C++20 on, CMake links nothing and hw::span is
// std::span. Code above this library (libart-stm32, firmware) uses hw::span.
//
// All translation units of one program must see the same span type. When
// this library fetches span-compat, it sets SPAN_COMPAT_USE_STD_SPAN=OFF, so
// span_compat::span is the own span at every standard. A superproject that
// mixes standards and provides span-compat itself must also set OFF.

#if defined(SYSTEM_STM32_USE_SPAN_COMPAT)
#include <span-compat/span.h>

namespace hw {
using span_compat::dynamic_extent;
using span_compat::span;
} // namespace hw
#else
#if defined(__has_include)
#if __has_include(<version>)
#include <version>
#endif
#endif

#if !defined(__cpp_lib_span)
#error "hw::span needs C++20, or span-compat (SYSTEM_STM32_USE_SPAN_COMPAT) below C++20"
#endif

#include <span>

namespace hw {
using std::dynamic_extent;
using std::span;
} // namespace hw
#endif

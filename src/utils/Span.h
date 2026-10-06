#pragma once

// The one span type of the public interface of this library: hw::span.
//
// std::span exists only from C++20 on. Below C++20, CMake links the
// span-compat library and defines SYSTEM_STM32_USE_SPAN_COMPAT, so hw::span
// is span_compat::span. From C++20 on, CMake links nothing and hw::span is
// std::span. Code above this library (libart-stm32, firmware) uses hw::span
// and does not need to know which type it is.

#if defined(SYSTEM_STM32_USE_SPAN_COMPAT)
#include <span-compat/span.h>

namespace hw {
using span_compat::dynamic_extent;
using span_compat::span;
} // namespace hw
#else
#include <span>

namespace hw {
using std::dynamic_extent;
using std::span;
} // namespace hw
#endif

#include "trace_compat.h"

int __attribute__((weak)) trace_printf(const char *format, ...) {
   (void)format;
   return 0;
}

void __attribute__((weak)) trace_flush() {
}

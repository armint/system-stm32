#pragma once

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

int trace_printf(const char *format, ...);
void trace_flush();

#if defined(__cplusplus)
}
#endif /* __cplusplus */

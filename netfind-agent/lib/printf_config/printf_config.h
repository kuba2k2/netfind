// Copyright (c) Kuba Szczodrzyński 2026-4-10.

#pragma once

// make printf.c define wrapper functions
#define printf_	   __wrap_printf
#define sprintf_   __wrap_sprintf
#define vsprintf_  __wrap_vsprintf
#define snprintf_  __wrap_snprintf
#define vsnprintf_ __wrap_vsnprintf
#define vprintf_   __wrap_vprintf

#ifdef PRINTF_IMPLEMENTATION
extern int putchar(int);

// call stdlib's putchar() directly
static inline void putchar_(char c) {
	putchar(c);
}
#endif

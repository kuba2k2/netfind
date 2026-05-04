// Copyright (c) Kuba Szczodrzyński 2024-12-8.

#include "lt_logger.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#define COLOR_FMT			 "\e[0;30m"
#define COLOR_BLACK			 0x00
#define COLOR_RED			 0x01
#define COLOR_GREEN			 0x02
#define COLOR_YELLOW		 0x03
#define COLOR_BLUE			 0x04
#define COLOR_MAGENTA		 0x05
#define COLOR_CYAN			 0x06
#define COLOR_WHITE			 0x07
#define COLOR_BRIGHT_BLACK	 0x10
#define COLOR_BRIGHT_RED	 0x11
#define COLOR_BRIGHT_GREEN	 0x12
#define COLOR_BRIGHT_YELLOW	 0x13
#define COLOR_BRIGHT_BLUE	 0x14
#define COLOR_BRIGHT_MAGENTA 0x15
#define COLOR_BRIGHT_CYAN	 0x16
#define COLOR_BRIGHT_WHITE	 0x17

static const char levels[] = {'V', 'D', 'I', 'W', 'E', 'F'};

#if LT_LOGGER_COLOR
static const uint8_t colors[] = {
	COLOR_BRIGHT_CYAN,
	COLOR_BRIGHT_BLUE,
	COLOR_BRIGHT_GREEN,
	COLOR_BRIGHT_YELLOW,
	COLOR_BRIGHT_RED,
	COLOR_BRIGHT_MAGENTA,
};
#endif

#if LT_LOGGER_CALLER
void lt_log(const uint8_t level, const char *caller, const unsigned short line, const char *format, ...) {
#else
void lt_log(const uint8_t level, const char *format, ...) {
#endif

#if LT_LOGGER_TIMESTAMP
	struct timeval tv;
	gettimeofday(&tv, NULL);
	time_t t	 = tv.tv_sec;
	struct tm tm = *localtime(&t);
#endif

#if LT_LOGGER_TASK
	const char *thread = NULL;
#endif

#if LT_LOGGER_COLOR
	char c_bright = '0' + (colors[level] >> 4);
	char c_value  = '0' + (colors[level] & 0x7);
#endif

	printf(
	// format:
#if LT_LOGGER_COLOR
		"\x1B[%c;3%cm"
#endif
		"%c "
#if LT_LOGGER_TIMESTAMP
		"[%04d-%02d-%02d %02d:%02d:%02d.%03ld] "
#endif
#if LT_LOGGER_COLOR
		"\x1B[0m"
#endif
#if LT_LOGGER_TASK
		"[%s] "
#endif
#if LT_LOGGER_CALLER
		"%s():%hu: "
#endif
		,
	// arguments:
#if LT_LOGGER_COLOR
		c_bright, // whether text is bright
		c_value,  // text color
#endif
		levels[level]
#if LT_LOGGER_TIMESTAMP
		,
		tm.tm_year + 1900,
		tm.tm_mon + 1,
		tm.tm_mday,
		tm.tm_hour,
		tm.tm_min,
		tm.tm_sec,
		tv.tv_usec / 1000
#endif
#if LT_LOGGER_TASK
		,
		thread == NULL ? "main" : thread
#endif
#if LT_LOGGER_CALLER
		,
		caller,
		line
#endif
	);

	va_list va_args;
	va_start(va_args, format);
	vprintf(format, va_args);
	va_end(va_args);
	putchar('\n');
	fflush(stdout);
}

// Copyright (c) Kuba Szczodrzyński 2026-4-9.

#pragma once

#define nf_min(a, b)            \
	({                          \
		__typeof__(a) _a = (a); \
		__typeof__(b) _b = (b); \
		_a < _b ? _a : _b;      \
	})
#define nf_max(a, b)            \
	({                          \
		__typeof__(a) _a = (a); \
		__typeof__(b) _b = (b); \
		_a > _b ? _a : _b;      \
	})

#ifdef __GNUC__
#define PACK(__Declaration__) __Declaration__ __attribute__((__packed__))
#endif

#ifdef MSVC
#define PACK(__Declaration__) __pragma(pack(push, 1)) __Declaration__ __pragma(pack(pop))
#endif

#define STRINGIFY(x)			   #x
#define STRINGIFY_MACRO(x)		   STRINGIFY(x)
#define CONCAT_(prefix, suffix)	   prefix##suffix
#define CONCAT(prefix, suffix)	   CONCAT_(prefix, suffix)
#define UNIQ(name)				   CONCAT(name, __LINE__)
#define STRUCT_PADDING(field, len) char _padding_##field[4 - (len) % 4]
#define BUILD_BUG_ON(condition)	   ((void)sizeof(char[1 - 2 * !!(condition)]))

#define NF_WITH_MUTEX(m) \
	for (volatile int UNIQ(loop) = pthread_mutex_lock(&(m)); UNIQ(loop) == 0; pthread_mutex_unlock(&(m)), UNIQ(loop)++)

#define NF_MALLOC(ptr, size, err)                                                                     \
	do {                                                                                              \
		ptr = malloc(size);                                                                           \
		if (ptr == NULL) {                                                                            \
			LT_E("Memory allocation failed for '" #ptr "' (%llu bytes)", (unsigned long long)(size)); \
			err;                                                                                      \
		}                                                                                             \
		memset(ptr, 0, size);                                                                         \
	} while (0)

#define NF_ERR(level, err, ...)  \
	do {                         \
		LT_##level(__VA_ARGS__); \
		err;                     \
	} while (0)

// Copyright (c) Kuba Szczodrzyński 2026-4-12.

#pragma once

#include <memory.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct dcpy_t {
	uint8_t *buf;
	uint32_t len;
	bool oob;
} dcpy_t;

// dcpy.c
uint16_t dcpy_get16b(dcpy_t *src);
uint16_t dcpy_get16l(dcpy_t *src);
uint8_t dcpy_get8(dcpy_t *src);
void dcpy_get(dcpy_t *src, void *dst, int num);
void dcpy_put8(dcpy_t *dst, uint8_t value);
void dcpy_put(dcpy_t *dst, const void *src, int num);
void dcpy_skip(dcpy_t *src, int num);
void dcpy_cpy(dcpy_t *dst, dcpy_t *src, int num);

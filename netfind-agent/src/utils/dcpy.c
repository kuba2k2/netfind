// Copyright (c) Kuba Szczodrzyński 2026-4-12.

#include "dcpy.h"

uint16_t dcpy_get16b(dcpy_t *src) {
	if (src->len < 2)
		return (src->oob = true), (src->len = 0);
	uint16_t value = (*src->buf++) << 8;
	value |= *src->buf++;
	src->len -= 2;
	return value;
}

uint16_t dcpy_get16l(dcpy_t *src) {
	if (src->len < 2)
		return (src->oob = true), (src->len = 0);
	uint16_t value = *src->buf++;
	value |= (*src->buf++) << 8;
	src->len -= 2;
	return value;
}

uint8_t dcpy_get8(dcpy_t *src) {
	if (src->len < 1)
		return (src->oob = true), (src->len = 0);
	uint8_t value = *src->buf++;
	src->len -= 1;
	return value;
}

void dcpy_get(dcpy_t *src, void *dst, int num) {
	if (src->len < num) {
		src->oob = true, src->len = 0;
		return;
	}
	memcpy(dst, src->buf, num);
	src->buf += num, src->len -= num;
}

void dcpy_put8(dcpy_t *dst, uint8_t value) {
	if (dst->len < 1) {
		dst->oob = true, dst->len = 0;
		return;
	}
	*dst->buf++ = value;
	dst->len -= 1;
}

void dcpy_put(dcpy_t *dst, const void *src, int num) {
	if (dst->len < num) {
		dst->oob = true, dst->len = 0;
		return;
	}
	memcpy(dst->buf, src, num);
	dst->buf += num, dst->len -= num;
}

void dcpy_skip(dcpy_t *src, int num) {
	if (src->len < num) {
		src->oob = true, src->len = 0;
		return;
	}
	src->len -= num, src->buf += num;
}

void dcpy_cpy(dcpy_t *dst, dcpy_t *src, int num) {
	if (src->len < num) {
		src->oob = true, src->len = 0;
		return;
	}
	dcpy_put(dst, src->buf, num);
	src->buf += num, src->len -= num;
}

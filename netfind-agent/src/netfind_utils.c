// Copyright (c) Kuba Szczodrzyński 2024-12-16.

#include "netfind.h"

void hexdump(const void *buf, size_t len) {
	size_t pos = 0;
	while (pos < len) {
		// print hex offset
		printf("%06x ", (unsigned int)pos);
		// calculate current line width
		size_t lineWidth = nf_min(16, len - pos);
		// print hexadecimal representation
		for (size_t i = 0; i < lineWidth; i++) {
			if (i % 8 == 0) {
				putchar(' ');
			}
			printf("%02x ", ((const uint8_t *)buf)[pos + i]);
		}
		// print ascii representation
		putchar(' ');
		putchar('|');
		for (size_t i = 0; i < lineWidth; i++) {
			uint8_t c = ((const uint8_t *)buf)[pos + i];
			putchar((c >= 0x20 && c <= 0x7f) ? c : '.');
		}
		puts("|");
		pos += lineWidth;
	}
	fflush(stdout);
}

void *memdup(const void *src, size_t size) {
	if (!src || !size)
		return NULL;
	void *dest = malloc(size);
	if (!dest)
		return NULL;
	memcpy(dest, src, size);
	return dest;
}

int strsplit(char *str, char delim, char **parts, int max) {
	parts[0] = str;
	int i;
	const char *pos = str;
	for (i = 1; i < max && pos != NULL; i++) {
		parts[i] = (void *)strchr(pos, delim);
		if (!parts[i])
			break;
		*parts[i]++ = '\0';
		pos			= parts[i];
	}
	return i;
}

char *strlstrip(char *str, char ch) {
	while (*str == ch)
		str++;
	return str;
}

char *strrstrip(char *str, char ch) {
	if (!*str)
		return str;
	char *end = str + strlen(str) - 1;
	while (*end == ch)
		*end-- = '\0';
	return str;
}

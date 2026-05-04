// Copyright (c) Kuba Szczodrzyński 2026-4-10.

#include "netfind.h"

const mac_addr_t MAC_ADDR_ANY = {0};
const ip_addr_t IP_ADDR_ANY4  = {.length = 4, .addr = {0}};
const ip_addr_t IP_ADDR_ANY6  = {.length = 16, .addr = {0}};
const ip4_addr_t IP4_ADDR_ANY = {0};
const ip6_addr_t IP6_ADDR_ANY = {0};

const char *nf_mac2str(char *buf, const mac_addr_t mac, char sep) {
	char *out = buf;
	for (int i = 0; i < 6; i++) {
		if (i && sep)
			*buf++ = sep;
		buf += sprintf(buf, "%02x", mac[i]);
	}
	return out;
}

const char *nf_ip42str(char *buf, const ip4_addr_t ip4) {
	sprintf(buf, "%u.%u.%u.%u", ip4[0], ip4[1], ip4[2], ip4[3]);
	return buf;
}

const char *nf_ip62str(char *buf, const ip6_addr_t ip6) {
	const uint16_t *ip6w = (void *)ip6;
	// compress the IPv6 address representation
	int cmp_a = 0, cmp_b = 0;
	for (int a = 0, b = 0; b < 9; b++) {
		uint16_t word = (b < 8) ? ip6w[b] : 1;
		if (word != 0) {
			if (a != -1 && (b - a) > (cmp_b - cmp_a))
				cmp_a = a, cmp_b = b;
			a = -1;
		} else if (a == -1) {
			a = b;
		}
	}
	if (cmp_b == cmp_a + 1)
		cmp_a = cmp_b = 0;

	char *out = buf;
	for (int i = 0; i < 8; i++) {
		if (i >= cmp_a && i < cmp_b) {
			if (i == cmp_a)
				*buf++ = ':', *buf++ = ':';
			continue;
		}
		if (i && i != cmp_b)
			*buf++ = ':';
		buf += sprintf(buf, "%x", (ip6[i * 2] << 8) | (ip6[i * 2 + 1] << 0));
	}
	*buf = '\0';
	return out;
}

const char *nf_ip2str(char *buf, const ip_addr_t *ip) {
	if (ip->length == sizeof(ip4_addr_t))
		return nf_ip42str(buf, ip->ip4);
	if (ip->length == sizeof(ip6_addr_t))
		return nf_ip62str(buf, ip->ip6);
	return "(invalid addr)";
}

void nf_sa2ip(ip_addr_t *ip, const struct sockaddr *sa) {
	switch (sa->sa_family) {
		case AF_INET:
			ip->length = sizeof(ip->ip4);
			memcpy(ip->ip4, &((const struct sockaddr_in *)sa)->sin_addr, sizeof(ip->ip4));
			break;
		case AF_INET6:
			ip->length = sizeof(ip->ip6);
			memcpy(ip->ip6, &((const struct sockaddr_in6 *)sa)->sin6_addr, sizeof(ip->ip6));
			break;
	}
}

void nf_mac_cpy(mac_addr_t mac, const void *src) {
	memcpy(mac, src, sizeof(mac_addr_t));
}

void nf_ip_mask(ip_addr_t *ip, const ip_addr_t *mask) {
	for (int i = 0; i < ip->length; i++) {
		ip->addr[i] &= mask->addr[i];
	}
}

void nf_ip_cpy(ip_addr_t *dst, const ip_addr_t *src) {
	dst->length = src->length;
	memcpy(dst->addr, src->addr, src->length);
}

void nf_ip4_cpy(ip_addr_t *dst, const ip4_addr_t src) {
	dst->length = sizeof(ip4_addr_t);
	memcpy(dst->ip4, src, sizeof(ip4_addr_t));
}

void nf_ip6_cpy(ip_addr_t *dst, const ip6_addr_t src) {
	dst->length = sizeof(ip6_addr_t);
	memcpy(dst->ip6, src, sizeof(ip6_addr_t));
}

int nf_ip_cmp(const ip_addr_t *ip1, const ip_addr_t *ip2) {
	if (ip1->length != ip2->length)
		return ip1->length - ip2->length;
	return memcmp(ip1->addr, ip2->addr, ip1->length);
}

int nf_mac_cmp(const mac_addr_t mac1, const mac_addr_t mac2) {
	return memcmp(mac1, mac2, sizeof(mac_addr_t));
}

unsigned int nf_mask_cidr(const ip_addr_t *mask) {
	unsigned int cidr = 0;
	for (int i = 0; i < mask->length; i++) {
		uint8_t b = mask->addr[i];
		if (!b)
			return cidr;
		int len = 8;
		while ((b & 1) == 0) {
			len -= 1;
			b >>= 1;
		}
		cidr += len;
	}
	return cidr;
}

const char *nf_fmt(char *buf, size_t size, const char *fmt, ...) {
	va_list arg;
	va_start(arg, fmt);
	vsnprintf(buf, size - 1, fmt, arg);
	va_end(arg);
	return buf;
}

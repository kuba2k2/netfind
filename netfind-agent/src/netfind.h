// Copyright (c) Kuba Szczodrzyński 2026-4-9.

#pragma once

#include <math.h>
#include <memory.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#if WIN32
#include <io.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <errno.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#endif

#include <git_version.h>
#include <lt_logger.h>
#include <printf/printf.h>
#include <utlist.h>

#include "netfind_macros.h"
#include "netfind_types.h"
#include "utils/dcpy.h"

// netfind_types.c
extern const mac_addr_t MAC_ADDR_ANY;
extern const ip_addr_t IP_ADDR_ANY4;
extern const ip_addr_t IP_ADDR_ANY6;
extern const ip4_addr_t IP4_ADDR_ANY;
extern const ip6_addr_t IP6_ADDR_ANY;
const char *nf_mac2str(char *buf, const mac_addr_t mac, char sep);
const char *nf_ip42str(char *buf, const ip4_addr_t ip4);
const char *nf_ip62str(char *buf, const ip6_addr_t ip6);
const char *nf_ip2str(char *buf, const ip_addr_t *ip);
void nf_sa2ip(ip_addr_t *ip, const struct sockaddr *sa);
void nf_mac_cpy(mac_addr_t mac, const void *src);
void nf_ip_mask(ip_addr_t *ip, const ip_addr_t *mask);
void nf_ip_cpy(ip_addr_t *dst, const ip_addr_t *src);
void nf_ip4_cpy(ip_addr_t *dst, const ip4_addr_t src);
void nf_ip6_cpy(ip_addr_t *dst, const ip6_addr_t src);
int nf_ip_cmp(const ip_addr_t *ip1, const ip_addr_t *ip2);
int nf_mac_cmp(const mac_addr_t mac1, const mac_addr_t mac2);
unsigned int nf_mask_cidr(const ip_addr_t *mask);
const char *nf_fmt(char *buf, size_t size, const char *fmt, ...);

// netfind_utils.c
void hexdump(const void *buf, size_t len);
void *memdup(const void *src, size_t size);
int strsplit(char *str, char delim, char **parts, int max);
char *strlstrip(char *str, char ch);
char *strrstrip(char *str, char ch);

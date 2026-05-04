// Copyright (c) Kuba Szczodrzyński 2026-4-9.

#pragma once

#include <stdint.h>

typedef enum nf_err_t {
	NF_ERR_MIN = -100,
	NF_ERR_FAILED,
	NF_ERR_MALLOC,
	NF_ERR_NULL,
	NF_ERR_THREAD,
	NF_ERR_RUNNING,
	NF_ERR_CONFIG,
	NF_ERR_IGNORE,
	NF_ERR_IF_NOT_FOUND,
	NF_ERR_IF_AMBIGUOUS,
	NF_ERR_GW_NOT_FOUND,
	NF_ERR_GW_AMBIGUOUS,
	NF_ERR_OK = 0,
} nf_err_t;

typedef enum __attribute__((packed)) eth_type_t {
	ETH_TYPE_ANY	 = 0,
	ETH_TYPE_IPV4	 = 0x0800,
	ETH_TYPE_ARP	 = 0x0806,
	ETH_TYPE_802_1Q	 = 0x8100,
	ETH_TYPE_IPV6	 = 0x86DD,
	ETH_TYPE_802_1AD = 0x88A8,
} eth_type_t;

typedef uint8_t mac_addr_t[6];
typedef uint8_t ip4_addr_t[4];
typedef uint8_t ip6_addr_t[16];

typedef struct ip_addr_t {
	uint8_t length;

	union {
		ip4_addr_t ip4;
		ip6_addr_t ip6;
		uint8_t addr[16];
	};
} ip_addr_t;

typedef struct ip_net_t {
	ip_addr_t if_addr; //!< Interface IP address (192.168.0.120)
	mac_addr_t if_mac; //!< Interface MAC address
	ip_addr_t mask;	   //!< Network mask (255.255.255.0)
	ip_addr_t addr;	   //!< Network IP address (192.168.0.0)
	ip_addr_t gw_addr; //!< Gateway IP address (192.168.0.1)
	mac_addr_t gw_mac; //!< Gateway MAC address
	unsigned int cidr; //!< Subnet prefix length
	struct ip_net_t *prev, *next;
} ip_net_t;

#define IP_4B(a, i) a[i + 0], a[i + 1], a[i + 2], a[i + 3]
#define IP_ADDR4(a)                                 \
	{                                               \
		.length = sizeof(a), .ip4 = { IP_4B(a, 0) } \
	}
#define IP_ADDR6(a)                                                                         \
	{                                                                                       \
		.length = sizeof(a), .ip6 = { IP_4B(a, 0), IP_4B(a, 4), IP_4B(a, 8), IP_4B(a, 12) } \
	}

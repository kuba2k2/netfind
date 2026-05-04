// Copyright (c) Kuba Szczodrzyński 2026-4-11.

#pragma once

#include <netfind.h>

#include "ethcap_proto.h"

typedef struct ethcap_t ethcap_t;
typedef struct devdb_t devdb_t;

typedef struct ethcap_cap_t {
	devdb_t *devdb;		//!< Device database handle
	ip_net_t *networks; //!< Networks used for filtering packets
	mac_addr_t ifaddr;	//!< Captured interface's MAC address
	mac_addr_t netaddr; //!< Captured network's identifier

	const void *data;		//!< Entire captured frame (layer 1)
	unsigned int len;		//!< Entire captured length (layer 1)
	mac_addr_t src_macaddr; //!< Source MAC address (layer 2)
	mac_addr_t dst_macaddr; //!< Destination MAC address (layer 2)
	eth_type_t eth_type;	//!< EtherType (layer 2)
	ip_addr_t src_ipaddr;	//!< Source IP address (layer 3)
	ip_addr_t dst_ipaddr;	//!< Destination IP address (layer 3)
	uint8_t proto;			//!< Protocol (layer 3)
	uint16_t sport;			//!< Source port (layer 4)
	uint16_t dport;			//!< Destination port (layer 4)
} ethcap_cap_t;

// ethcap_proto.c
extern const ethcap_proto_t *ethcap_proto_l2_start;
extern const ethcap_proto_t *ethcap_proto_l2_stop;
extern const ethcap_proto_t *ethcap_proto_l3_start;
extern const ethcap_proto_t *ethcap_proto_l3_stop;
extern const ethcap_proto_t *ethcap_proto_l4_start;
extern const ethcap_proto_t *ethcap_proto_l4_stop;
extern const ethcap_proto_t *ethcap_proto_l5_start;
extern const ethcap_proto_t *ethcap_proto_l5_stop;
void ethcap_proto_init();

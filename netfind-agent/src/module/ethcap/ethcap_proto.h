// Copyright (c) Kuba Szczodrzyński 2026-4-12.

#pragma once

#include "ethcap.h"

typedef struct ethcap_cap_t ethcap_cap_t;
typedef void (*ethcap_proto_cb_t)(ethcap_cap_t *cap, const void *data, unsigned int len);

typedef struct ethcap_proto_t {
	eth_type_t eth_type; //!< EtherType (layer 2)
	uint8_t proto;		 //!< Protocol (layer 3)
	uint16_t sport;		 //!< Source port (layer 4)
	uint16_t dport;		 //!< Destination port (layer 4)
	ethcap_proto_cb_t cb;
} ethcap_proto_t;

#define ETHCAP_ADD_L2(_cb)                                                                                   \
	static const ethcap_proto_t _ethcap_proto_l2_##_cb __attribute__((section("ethcap_proto_l2"), used)) = { \
		.cb = (void *)(_cb),                                                                                 \
	};

#define ETHCAP_ADD_L3(_eth_type, _cb)                                                                        \
	static const ethcap_proto_t _ethcap_proto_l3_##_cb __attribute__((section("ethcap_proto_l3"), used)) = { \
		.eth_type = (_eth_type),                                                                             \
		.cb		  = (void *)(_cb),                                                                           \
	};

#define ETHCAP_ADD_L4(_eth_type, _proto, _cb)                                                                \
	static const ethcap_proto_t _ethcap_proto_l4_##_cb __attribute__((section("ethcap_proto_l4"), used)) = { \
		.eth_type = (_eth_type),                                                                             \
		.proto	  = (_proto),                                                                                \
		.cb		  = (void *)(_cb),                                                                           \
	}

#define ETHCAP_ADD_L5(_eth_type, _proto, _sport, _dport, _cb)                                                        \
	static const ethcap_proto_t _ethcap_proto_l5_##_cb##_sport __attribute__((section("ethcap_proto_l5"), used)) = { \
		.eth_type = (_eth_type),                                                                                     \
		.proto	  = (_proto),                                                                                        \
		.sport	  = (_sport),                                                                                        \
		.dport	  = (_dport),                                                                                        \
		.cb		  = (void *)(_cb),                                                                                   \
	}

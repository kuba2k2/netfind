// Copyright (c) Kuba Szczodrzyński 2026-4-11.

#include "../ethcap_priv.h"

typedef PACK(struct ethcap_udp_t {
	uint16_t src_port;
	uint16_t dst_port;
	uint16_t len;
	uint16_t checksum;
	uint8_t payload[];
}) ethcap_udp_t;

static void udp(ethcap_cap_t *cap, const ethcap_udp_t *udp, unsigned int len) {
	if (len < sizeof(*udp))
		return;
	// skip packets that aren't completely received
	if (len < ntohs(udp->len))
		return;

	// find the UDP payload
	const void *payload = udp->payload;
	// calculate remaining length
	len = nf_min(len, ntohs(udp->len)) - sizeof(*udp);

	// fill in capture data
	cap->sport = ntohs(udp->src_port);
	cap->dport = ntohs(udp->dst_port);

	// call registered protocol handlers
	for (const ethcap_proto_t *proto = ethcap_proto_l5_start; proto < ethcap_proto_l5_stop; proto++) {
		if (proto->eth_type && proto->eth_type != cap->eth_type)
			continue;
		if (proto->proto && proto->proto != IPPROTO_UDP)
			continue;
		if (proto->sport && proto->sport != cap->sport)
			continue;
		if (proto->dport && proto->dport != cap->dport)
			continue;
		proto->cb(cap, payload, len);
	}
}

ETHCAP_ADD_L4(
	/* eth_type= */ ETH_TYPE_ANY,
	/* proto= */ IPPROTO_UDP,
	/* cb= */ udp
);

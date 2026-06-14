// Copyright (c) Kuba Szczodrzyński 2026-4-9.

#include "../ethcap_priv.h"

typedef PACK(struct ethcap_ipv6_t {
	uint8_t hdr[4];
	uint16_t len;
	uint8_t next_hdr;
	uint8_t hop_limit;
	ip6_addr_t src;
	ip6_addr_t dst;
	uint8_t payload[];
}) ethcap_ipv6_t;

static void ipv6(ethcap_cap_t *cap, const ethcap_ipv6_t *ipv6, unsigned int len) {
	if (len < sizeof(*ipv6))
		return;
	// skip frames that aren't completely received - TODO fragmentation?
	if (len < ntohs(ipv6->len) + sizeof(*ipv6))
		return;

	// find the IPv6 payload - TODO handle extension headers
	const void *payload = ipv6->payload;
	// calculate remaining length
	len = nf_min(len - sizeof(*ipv6), ntohs(ipv6->len));

	// fill in capture data
	nf_ip6_cpy(&cap->src_ipaddr, ipv6->src);
	nf_ip6_cpy(&cap->dst_ipaddr, ipv6->dst);
	cap->proto = ipv6->next_hdr;

	// check if the source address is assigned (not ::)
	if (nf_ip_cmp(&cap->src_ipaddr, &IP_ADDR_ANY6) != 0) {
		// ignore packets from non-local IP addresses
		if (!ethcap_networks_is_cap_local(cap))
			return;

		char buf[40];
		devdb_put(
			/* devdb= */ cap->devdb,
			/* netaddr= */ cap->netaddr,
			/* devaddr= */ cap->src_macaddr,
			/* key= */ "ip.addr",
			/* value= */ nf_ip62str(buf, ipv6->src),
			/* append= */ true
		);
	}

	// call registered protocol handlers
	for (const ethcap_proto_t *proto = ethcap_proto_l4_start; proto < ethcap_proto_l4_stop; proto++) {
		if (proto->eth_type && proto->eth_type != ETH_TYPE_IPV6)
			continue;
		if (proto->proto && proto->proto != cap->proto)
			continue;
		proto->cb(cap, payload, len);
	}
}

ETHCAP_ADD_L3(
	/* eth_type= */ ETH_TYPE_IPV6,
	/* cb= */ ipv6
);

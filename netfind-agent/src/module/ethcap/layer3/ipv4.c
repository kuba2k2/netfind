// Copyright (c) Kuba Szczodrzyński 2026-4-9.

#include "../ethcap_priv.h"

typedef PACK(struct ethcap_ipv4_t {
	union {
		uint8_t header[0];
		struct {
			uint8_t hdr[2];
			uint16_t len;
			uint16_t id;
			uint16_t frag;
			uint8_t ttl;
			uint8_t proto;
			uint16_t checksum;
			ip4_addr_t src;
			ip4_addr_t dst;
		};
	};
}) ethcap_ipv4_t;

static void ipv4(ethcap_cap_t *cap, const ethcap_ipv4_t *ipv4, unsigned int len) {
	if (len < sizeof(*ipv4))
		return;
	// skip frames that aren't completely received - TODO fragmentation?
	if (len < ntohs(ipv4->len))
		return;

	// find the IPv4 payload
	uint8_t hdr_len		= (ipv4->hdr[0] & 0xF) * 4;
	const void *payload = ipv4->header + hdr_len;
	// calculate remaining length
	len = nf_min(len, ntohs(ipv4->len)) - hdr_len;

	// fill in capture data
	nf_ip4_cpy(&cap->src_ipaddr, ipv4->src);
	nf_ip4_cpy(&cap->dst_ipaddr, ipv4->dst);
	cap->proto = ipv4->proto;

	// check if the source address is assigned (not 0.0.0.0)
	if (nf_ip_cmp(&cap->src_ipaddr, &IP_ADDR_ANY4) != 0) {
		// ignore packets from non-local IP addresses
		if (!ethcap_networks_is_cap_local(cap))
			return;

		// avoid storing autoconfig IP addresses (169.254.x.x)
		if (ipv4->src[0] != 169 || ipv4->src[1] != 254) {
			char buf[20];
			devdb_put(
				/* devdb= */ cap->devdb,
				/* netaddr= */ cap->netaddr,
				/* devaddr= */ cap->src_macaddr,
				/* key= */ "ip.addr",
				/* value= */ nf_ip42str(buf, ipv4->src),
				/* append= */ true
			);
		}
	}

	// call registered protocol handlers
	for (const ethcap_proto_t *proto = ethcap_proto_l4_start; proto < ethcap_proto_l4_stop; proto++) {
		if (proto->eth_type && proto->eth_type != ETH_TYPE_IPV4)
			continue;
		if (proto->proto && proto->proto != cap->proto)
			continue;
		proto->cb(cap, payload, len);
	}
}

ETHCAP_ADD_L3(
	/* eth_type= */ ETH_TYPE_IPV4,
	/* cb= */ ipv4
);

// Copyright (c) Kuba Szczodrzyński 2026-4-9.

#include "../ethcap_priv.h"

typedef PACK(struct ethcap_arp_t {
	uint16_t hw_type;
	eth_type_t proto_type : 16;
	uint8_t hw_size;
	uint8_t proto_size;
	uint16_t opcode;
	struct {
		mac_addr_t src_macaddr;
		ip4_addr_t src_ipaddr;
		mac_addr_t dst_macaddr;
		ip4_addr_t dst_ipaddr;
	} ipv4;
}) ethcap_arp_t;

static void arp(ethcap_cap_t *cap, const ethcap_arp_t *arp, unsigned int len) {
	char buf[20];
	if (len < sizeof(*arp))
		return;
	// only handle Ethernet IPv4 packets - there is no ARP in IPv6
	if (ntohs(arp->hw_type) != 1 || arp->hw_size != 6)
		return;
	if (ntohs(arp->proto_type) != ETH_TYPE_IPV4 || arp->proto_size != 4)
		return;
	// ignore packets with no known sender IP address
	if (arp->ipv4.src_ipaddr[0] + arp->ipv4.src_ipaddr[1] + arp->ipv4.src_ipaddr[2] + arp->ipv4.src_ipaddr[3] == 0)
		return;
	// ignore autoconfig IP addresses
	if (arp->ipv4.src_ipaddr[0] == 169 && arp->ipv4.src_ipaddr[1] == 254)
		return;

	devdb_put(
		/* devdb= */ cap->devdb,
		/* netaddr= */ cap->netaddr,
		/* devaddr= */ cap->src_macaddr,
		/* key= */ "ip.addr",
		/* value= */ nf_ip42str(buf, arp->ipv4.src_ipaddr)
	);
}

ETHCAP_ADD_L3(
	/* eth_type= */ ETH_TYPE_ARP,
	/* cb= */ arp
);

// Copyright (c) Kuba Szczodrzyński 2026-4-12.

#include "../ethcap.h"

#include "devdb/devdb.h"

typedef PACK(struct ethcap_eth_t {
	mac_addr_t dst;
	mac_addr_t src;
	union {
		eth_type_t type : 16; //!< EtherType
		uint16_t len;		  //!< 802.3 Length
	};
	uint8_t payload[];
}) ethcap_eth_t;

typedef PACK(struct ethcap_eth_vlan_t {
	mac_addr_t dst;
	mac_addr_t src;
	uint16_t qtag;		   //!< VLAN tag
	eth_type_t qtype : 16; //!< EtherType (802.1Q/802.1ad)
	eth_type_t type : 16;  //!< EtherType
	uint8_t payload[];
}) ethcap_eth_vlan_t;

static void eth(ethcap_cap_t *cap, const ethcap_eth_t *eth, unsigned int len) {
	if (len < sizeof(*eth))
		return;
	// skip IEEE 802.3 Ethernet frames - only care about Ethernet II
	if (ntohs(eth->type) < ETH_TYPE_IPV4)
		return;

	// find the Ethernet payload
	const void *payload = eth->payload;
	len -= sizeof(ethcap_eth_t);

	// handle 802.1Q and 802.1ad VLAN-tagged packets
	eth_type_t eth_type		= ntohs(eth->type);
	ethcap_eth_vlan_t *vlan = (void *)eth;
	if (eth_type == ETH_TYPE_802_1Q || eth_type == ETH_TYPE_802_1AD) {
		eth_type = ntohs(vlan->type);
		payload	 = vlan->payload;
		len -= sizeof(ethcap_eth_vlan_t) - sizeof(ethcap_eth_t);
	}

	// fill in capture data
	nf_mac_cpy(cap->src_macaddr, eth->src);
	nf_mac_cpy(cap->dst_macaddr, eth->dst);
	cap->eth_type = eth_type;

	char buf[20];
	devdb_put(
		/* devdb= */ cap->devdb,
		/* netaddr= */ cap->netaddr,
		/* devaddr= */ eth->src,
		/* key= */ "eth.addr",
		/* value= */ nf_mac2str(buf, eth->src, ':')
	);

	// call registered protocol handlers
	for (const ethcap_proto_t *proto = ethcap_proto_l3_start; proto < ethcap_proto_l3_stop; proto++) {
		if (proto->eth_type && proto->eth_type != eth_type)
			continue;
		proto->cb(cap, payload, len);
	}
}

ETHCAP_ADD_L2(eth);

// Copyright (c) Kuba Szczodrzyński 2026-4-10.

#include "ethcap_priv.h"

static void ethcap_packet_handler(ethcap_t *ethcap, const struct pcap_pkthdr *hdr, const void *data);

void *ethcap_thread(ethcap_t *ethcap) {
	LT_D("Starting ethcap thread");

	const nf_cfg_choice_t *netif = nf_cfg_option_get_single(nf_cfg_option_find(ethcap->mod.cfg, "netif"));
	if (!netif || !netif->data)
		return NULL;

	// assign networks from the 'netif' choice
	ethcap->networks = netif->data;
	// copy the interface MAC address
	nf_mac_cpy(ethcap->ifaddr, ethcap->networks->if_mac);
	// copy the network identifier (gateway MAC)
	nf_mac_cpy(ethcap->netaddr, ethcap->networks->gw_mac);

	LT_I("Opening interface '%s'", netif->key);
	pcap_t *pcap = pcap_open_live(
		/* device= */ netif->key,
		/* snaplen= */ 65536,
		/* promisc= */ 1,
		/* to_ms= */ 100,
		/* errbuf= */ ethcap->errbuf
	);
	if (!pcap) {
		LT_E("Failed to open network interface '%s'; errbuf=%s", netif->key, ethcap->errbuf);
		return NULL;
	}

	char buf[256];
	LT_I(
		"Starting capture on interface %s with networks: %s",
		nf_mac2str(buf, ethcap->networks->if_mac, ':'),
		ethcap_networks_describe(ethcap->networks, buf + 20, sizeof(buf) - 20 - 1)
	);

	pcap_loop(pcap, /* cnt= */ 0, /* callback= */ (pcap_handler)ethcap_packet_handler, /* user= */ (void *)ethcap);
	pcap_close(pcap);

	LT_D("Stopping ethcap thread");
	return NULL;
}

static void ethcap_packet_handler(ethcap_t *ethcap, const struct pcap_pkthdr *hdr, const void *data) {
	ethcap_cap_t cap = {
		.devdb	  = ethcap->devdb,
		.networks = ethcap->networks,
		.data	  = data,
		.len	  = hdr->caplen,
	};
	nf_mac_cpy(cap.ifaddr, ethcap->ifaddr);
	nf_mac_cpy(cap.netaddr, ethcap->netaddr);

	// call registered protocol handlers
	for (const ethcap_proto_t *proto = ethcap_proto_l2_start; proto < ethcap_proto_l2_stop; proto++) {
		proto->cb(&cap, data, hdr->caplen);
	}
}

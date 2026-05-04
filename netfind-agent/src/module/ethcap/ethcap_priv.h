// Copyright (c) Kuba Szczodrzyński 2026-4-9.

#pragma once

#include "ethcap.h"

#include <pcap/pcap.h>

#include "devdb/devdb.h"
#include "module/module.h"

typedef struct ethcap_t {
	module_t mod;
	devdb_t *devdb;
	char addrbuf[256];
	char errbuf[PCAP_ERRBUF_SIZE];

	pcap_t *handle;		//!< pcap capture handle
	ip_net_t *networks; //!< Networks used for filtering packets (in mod->opt->choices->data)
	mac_addr_t ifaddr;	//!< Captured interface's address
	mac_addr_t netaddr; //!< Captured network's address
} ethcap_t;

// ethcap.c
ethcap_t *ethcap_init(devdb_t *devdb);
void ethcap_free(ethcap_t *ethcap);
nf_err_t ethcap_start(ethcap_t *ethcap);
nf_err_t ethcap_stop(ethcap_t *ethcap);
nf_err_t ethcap_option_update(ethcap_t *ethcap, nf_cfg_option_t *opt);
nf_err_t ethcap_action_call(ethcap_t *ethcap, nf_action_t *call);

// ethcap_network.c
nf_err_t ethcap_network_identify(ip_net_t *net, const struct sockaddr *addr_sa, const struct sockaddr *mask_sa);
nf_err_t ethcap_networks_copy_mac(ip_net_t *networks);
const char *ethcap_networks_describe(const ip_net_t *networks, char *out, size_t len);
ip_net_t *ethcap_networks_identify_pcap(const pcap_if_t *dev);
bool ethcap_networks_is_cap_local(const ethcap_cap_t *cap);

// ethcap_thread.c
void *ethcap_thread(ethcap_t *ethcap);

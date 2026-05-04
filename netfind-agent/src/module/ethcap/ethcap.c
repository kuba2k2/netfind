// Copyright (c) Kuba Szczodrzyński 2026-4-9.

#include "ethcap_priv.h"

module_ops_t ethcap_ops = {
	.name		   = "ethcap",
	.init		   = (module_init_t)ethcap_init,
	.free		   = (module_free_t)ethcap_free,
	.start		   = (module_start_t)ethcap_start,
	.stop		   = (module_stop_t)ethcap_stop,
	.option_update = (module_option_update_t)ethcap_option_update,
	.action_call   = (module_action_call_t)ethcap_action_call,
};

ethcap_t *ethcap_init(devdb_t *devdb) {
	ethcap_proto_init();

	ethcap_t *ethcap;
	NF_MALLOC(ethcap, sizeof(*ethcap), return NULL);
	ethcap->mod.ops = &ethcap_ops;
	ethcap->devdb	= devdb;

	nf_cfg_option_t *opt = NULL;
	nf_action_t *act	 = NULL;

	pcap_if_t *devs = NULL;
	if (pcap_findalldevs(&devs, ethcap->errbuf) != 0)
		NF_ERR(E, goto err, "Error in pcap_findalldevs(): %s", ethcap->errbuf);

	opt = nf_cfg_option_make(
		CFG_TYPE_SINGLE_CHOICE,
		"netif",
		"Capture interface",
		"Choose a network interface to capture packets on"
	);
	if (opt) {
		DL_APPEND(ethcap->mod.cfg, opt);
		pcap_if_t *dev = NULL;
		LL_FOREACH(devs, dev) {
			ip_net_t *networks = ethcap_networks_identify_pcap(dev);
			if (!networks)
				continue;
			char buf[256];
			nf_cfg_choice_t *choice = nf_cfg_choice_make(
				dev->name,
				dev->description ? dev->description : dev->name,
				ethcap_networks_describe(networks, buf, sizeof(buf) - 1)
			);
			if (choice) {
				choice->data = networks;
				DL_APPEND(opt->choices, choice);
			}
		}
	}
	pcap_freealldevs(devs);

	act = nf_action_make("scan-arp", "Scan IP range (ARP)", "Find devices in the chosen IP address range by ARP scan");
	if (act) {
		DL_APPEND(ethcap->mod.act, act);
		opt = nf_cfg_option_make(CFG_TYPE_TEXT, "(ipaddr)ipaddr-first", "First IP address", NULL);
		if (opt)
			DL_APPEND(act->options, opt);
		opt = nf_cfg_option_make(CFG_TYPE_TEXT, "(ipaddr)ipaddr-last", "Last IP address", NULL);
		if (opt)
			DL_APPEND(act->options, opt);
	}

	act = nf_action_make("scan-mdns", "Scan mDNS service", "Find devices by scanning for mDNS services");
	if (act) {
		DL_APPEND(ethcap->mod.act, act);
		opt = nf_cfg_option_make(CFG_TYPE_TEXT, "(mdns)service-name", "Service name", NULL);
		if (opt) {
			nf_cfg_option_set_text(opt, "_services._dns-sd._udp.local");
			DL_APPEND(act->options, opt);
		}
	}

	act = nf_action_make(
		"scan-ports",
		"Scan device ports/services",
		"Check device's open ports and identify running services"
	);
	if (act) {
		DL_APPEND(ethcap->mod.act, act);
		opt = nf_cfg_option_make_dev_ip();
		if (opt)
			DL_APPEND(act->options, opt);
	}

	act = nf_action_make(
		"reverse-dns",
		"Get device hostname (reverse DNS)",
		"Perform a DNS request for the device's local IP address"
	);
	if (act) {
		DL_APPEND(ethcap->mod.act, act);
		opt = nf_cfg_option_make_dev_ip();
		if (opt)
			DL_APPEND(act->options, opt);
	}

	act = nf_action_make("ping", "Ping device", "Send ICMP-ping to the device");
	if (act) {
		DL_APPEND(ethcap->mod.act, act);
		opt = nf_cfg_option_make_dev_ip();
		if (opt)
			DL_APPEND(act->options, opt);
	}

	return ethcap;

err:
	ethcap_free(ethcap);
	return NULL;
}

void ethcap_free(ethcap_t *ethcap) {
	if (!ethcap)
		return;

	ethcap_stop(ethcap);

	// get 'netif' option and free all networks from all choices
	nf_cfg_option_t *netif_opt = nf_cfg_option_find(ethcap->mod.cfg, "netif");
	if (netif_opt) {
		nf_cfg_choice_t *choice, *ctmp;
		DL_FOREACH_SAFE(netif_opt->choices, choice, ctmp) {
			ip_net_t *networks = choice->data;
			ip_net_t *net, *ntmp;
			DL_FOREACH_SAFE(networks, net, ntmp) {
				DL_DELETE(networks, net);
				free(net);
			}
		}
	}

	// delete and free all options
	nf_cfg_option_t *opt, *otmp;
	DL_FOREACH_SAFE(ethcap->mod.cfg, opt, otmp) {
		DL_DELETE(ethcap->mod.cfg, opt);
		nf_cfg_option_free(opt);
	}

	// delete and free all actions
	nf_action_t *act, *atmp;
	DL_FOREACH_SAFE(ethcap->mod.act, act, atmp) {
		DL_DELETE(ethcap->mod.act, act);
		nf_action_free(act);
	}

	free(ethcap);
}

nf_err_t ethcap_start(ethcap_t *ethcap) {
	int err = 0;
	if ((err = ethcap_stop(ethcap)))
		return err;

	const nf_cfg_choice_t *netif = nf_cfg_option_get_single(nf_cfg_option_find(ethcap->mod.cfg, "netif"));
	if (!netif || !netif->data)
		return NF_ERR_CONFIG;

	if ((err = pthread_create(&ethcap->mod.thread, NULL, (void *)ethcap_thread, ethcap))) {
		LT_E("Starting ethcap thread failed; err=%d", err);
		return NF_ERR_THREAD;
	}

	return NF_ERR_OK;
}

nf_err_t ethcap_stop(ethcap_t *ethcap) {
	if (ethcap->mod.thread == 0)
		return NF_ERR_OK;

	int err;
	if ((err = pthread_cancel(ethcap->mod.thread)) && errno != ESRCH) {
		LT_E("Stopping ethcap thread failed; err=%d", err);
		return NF_ERR_THREAD;
	}

	ip_net_t *net = NULL, *tmp;
	DL_FOREACH_SAFE(ethcap->networks, net, tmp) {
		DL_DELETE(ethcap->networks, net);
		free(net);
	}

	ethcap->mod.thread = 0;
	return NF_ERR_OK;
}

nf_err_t ethcap_option_update(ethcap_t *ethcap, nf_cfg_option_t *opt) {}

nf_err_t ethcap_action_call(ethcap_t *ethcap, nf_action_t *call) {}

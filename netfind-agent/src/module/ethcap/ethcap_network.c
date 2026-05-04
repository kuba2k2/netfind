// Copyright (c) Kuba Szczodrzyński 2026-4-11.

#include "ethcap_priv.h"

static nf_err_t ethcap_network_initialize(
	ip_net_t *net,
	const struct sockaddr *addr_sa,
	const struct sockaddr *mask_sa
) {
	if (!net || !addr_sa || !mask_sa)
		return NF_ERR_NULL;

	// convert sockaddr* to ip_net_t*
	nf_sa2ip(&net->if_addr, addr_sa);
	nf_sa2ip(&net->mask, mask_sa);
	// ignore addresses with /32 or /128 mask
	if (net->mask.addr[net->mask.length - 1] == 0xFF)
		return NF_ERR_IGNORE;
	// find out the network address
	nf_sa2ip(&net->addr, addr_sa);
	nf_ip_mask(&net->addr, &net->mask);
	net->cidr = nf_mask_cidr(&net->mask);
	// clear the gateway IP and MAC addresses
	net->gw_addr.length = 0;
	memset(net->gw_mac, 0, sizeof(net->gw_mac));

	return NF_ERR_OK;
}

static nf_err_t ethcap_networks_copy_mac(ip_net_t *networks) {
	if (!networks)
		return NF_ERR_NULL;

	// find a *single* interface & gateway MAC addresses
	mac_addr_t if_mac = {0}, gw_mac = {0};
	bool if_found = false, gw_found = false;

	ip_net_t *net = NULL;
	DL_FOREACH(networks, net) {
		// skip networks with no detected interface MAC
		if (nf_mac_cmp(net->if_mac, MAC_ADDR_ANY) == 0)
			continue;
		if (!if_found) {
			// first address found - copy it
			nf_mac_cpy(if_mac, net->if_mac);
			if_found = true;
		} else if (nf_mac_cmp(if_mac, net->if_mac) != 0)
			// a different address was found - fail
			return NF_ERR_IF_AMBIGUOUS;

		// same for gateway MAC
		if (nf_mac_cmp(net->gw_mac, MAC_ADDR_ANY) == 0)
			continue;
		if (!gw_found) {
			nf_mac_cpy(gw_mac, net->gw_mac);
			gw_found = true;
		} else if (nf_mac_cmp(gw_mac, net->gw_mac) != 0)
			return NF_ERR_GW_AMBIGUOUS;
	}
	if (!if_found)
		return NF_ERR_IF_NOT_FOUND;
	if (!gw_found)
		return NF_ERR_GW_NOT_FOUND;

	// copy the addresses to other networks
	DL_FOREACH(networks, net) {
		if (nf_mac_cmp(net->if_mac, MAC_ADDR_ANY) == 0)
			nf_mac_cpy(net->if_mac, if_mac);
		if (nf_mac_cmp(net->gw_mac, MAC_ADDR_ANY) == 0)
			nf_mac_cpy(net->gw_mac, gw_mac);
	}
	return NF_ERR_OK;
}

ip_net_t *ethcap_networks_identify(const pcap_if_t *dev) {
	const char *dev_name = dev->description ? dev->description : dev->name;
	if (!dev->addresses) {
		LT_D("Adapter '%s' - no addresses found, ignoring", dev_name);
		return NULL;
	}

	ip_net_t *networks		= NULL;
	const pcap_addr_t *addr = NULL;
	LL_FOREACH(dev->addresses, addr) {
		ip_net_t *net;
		NF_MALLOC(net, sizeof(*net), return NULL);
		nf_err_t err;
		if ((err = ethcap_network_initialize(net, addr->addr, addr->netmask))) {
			free(net);
			continue;
		}
		if ((err = ethcap_network_fill_addrs(dev->name, net))) {
			free(net);
			continue;
		}
		DL_APPEND(networks, net);
	}

	// copy the gateway MAC to all detected networks
	char buf[20];
	switch (ethcap_networks_copy_mac(networks)) {
		case NF_ERR_OK:
			LT_D("Adapter '%s' - found gateway MAC address: %s", dev_name, nf_mac2str(buf, networks->gw_mac, ':'));
			return networks;
		case NF_ERR_IF_NOT_FOUND:
			LT_W("Adapter '%s' - no interface MAC detected, ignoring", dev_name);
			break;
		case NF_ERR_IF_AMBIGUOUS:
			LT_E("Adapter '%s' - found multiple interface MAC addresses, ignoring", dev_name);
			break;
		case NF_ERR_GW_NOT_FOUND:
			LT_W("Adapter '%s' - no gateway MAC detected, ignoring", dev_name);
			break;
		case NF_ERR_GW_AMBIGUOUS:
			LT_E("Adapter '%s' - found multiple gateway MAC addresses, ignoring", dev_name);
			break;
		default:
			LT_E("Adapter '%s' - failed to find MAC addresses, ignoring", dev_name);
			break;
	}

	ip_net_t *net, *tmp;
	DL_FOREACH_SAFE(networks, net, tmp) {
		DL_DELETE(networks, net);
		free(net);
	}
	return NULL;
}

const char *ethcap_networks_describe(const ip_net_t *networks, char *out, size_t len) {
	if (!networks)
		return NULL;

	char buf[40];
	char *pos = out;
	*pos	  = '\0';

	const ip_net_t *net = NULL;
	DL_FOREACH(networks, net) {
		if (*out && len >= 2) {
			*pos++ = ',';
			*pos++ = ' ';
			len -= 2;
		}
		size_t chars = snprintf(
			pos,
			len,
			"%s/%u",
			net->gw_addr.length ? nf_ip2str(buf, &net->gw_addr) : nf_ip2str(buf, &net->addr),
			net->cidr
		);
		pos += chars;
		len -= chars;
	}

	snprintf(pos, len, " (ID: %s)", nf_mac2str(buf, networks->gw_mac, '\0'));
	return out;
}

bool ethcap_networks_is_cap_local(const ethcap_cap_t *cap) {
	if (!cap || !cap->networks)
		return false;

	// go through each connected network
	const ip_net_t *net = NULL;
	DL_FOREACH(cap->networks, net) {
		// only consider networks with the same address family
		if (net->addr.length != cap->src_ipaddr.length)
			continue;

		// apply the subnet mask
		ip_addr_t src_masked;
		nf_ip_cpy(&src_masked, &cap->src_ipaddr);
		nf_ip_mask(&src_masked, &net->mask);

		// accept if the IP address belongs to the subnet
		if (nf_ip_cmp(&src_masked, &net->addr) == 0)
			return true;

		// accept if the packet is not unicast (gateway -> local interface)
		if (nf_mac_cmp(cap->src_macaddr, net->gw_mac) != 0)
			return true;
		if (nf_mac_cmp(cap->dst_macaddr, net->if_mac) != 0)
			return true;
	}
	// otherwise reject
	return false;
}

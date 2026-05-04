// Copyright (c) Kuba Szczodrzyński 2026-4-11.

#include "ethcap_priv.h"

#if WIN32
#include <iphlpapi.h>
#endif

nf_err_t ethcap_network_identify(ip_net_t *net, const struct sockaddr *addr_sa, const struct sockaddr *mask_sa) {
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

#if WIN32
	unsigned long adapter_index = 0;
	{
		// list available adapters
		unsigned long gaa_flags = GAA_FLAG_INCLUDE_GATEWAYS | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST |
								  GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME;
		unsigned long addrs_len = 64 * 1024;
		IP_ADAPTER_ADDRESSES *adapters;
		NF_MALLOC(adapters, addrs_len, return NF_ERR_MALLOC);
		unsigned long err;
		if ((err = GetAdaptersAddresses(addr_sa->sa_family, gaa_flags, NULL, adapters, &addrs_len))) {
			LT_E("GetAdaptersAddresses failed; err=%ld", err);
			free(adapters);
			return NF_ERR_FAILED;
		}
		// find the adapter by its unicast address
		IP_ADAPTER_ADDRESSES *adapter = NULL;
		LL_FOREACH2(adapters, adapter, Next) {
			// find a matching address
			IP_ADAPTER_UNICAST_ADDRESS *unicast = NULL;
			LL_FOREACH2(adapter->FirstUnicastAddress, unicast, Next) {
				ip_addr_t unicast_ip = {0};
				nf_sa2ip(&unicast_ip, unicast->Address.lpSockaddr);
				if (nf_ip_cmp(&net->if_addr, &unicast_ip) == 0)
					break;
			}
			// if none of this adapter's addresses match, check the next one
			if (!unicast)
				continue;
			// store the adapter's interface index for later
			adapter_index = adapter->IfIndex;
			char buf[40];
			LT_V(
				"Matched interface address %s to adapter %s - '%ls' (index %lu)",
				nf_ip2str(buf, &net->if_addr),
				adapter->AdapterName,
				adapter->Description,
				adapter->IfIndex
			);
			// copy the MAC address, if available
			if (adapter->PhysicalAddressLength == sizeof(net->gw_mac))
				nf_mac_cpy(net->if_mac, adapter->PhysicalAddress);
			// copy the first available gateway address
			if (adapter->FirstGatewayAddress)
				nf_sa2ip(&net->gw_addr, adapter->FirstGatewayAddress->Address.lpSockaddr);
			break;
		}
		free(adapters);
		// ignore this network altogether if the adapter wasn't found
		if (!adapter)
			return NF_ERR_IGNORE;
	}
#endif

	// return early if there's no gateway IP address
	if (net->gw_addr.length == 0)
		return NF_ERR_OK;

#if WIN32
	{
		// get the IP neighbor table (ARP table)
		MIB_IPNET_TABLE2 *table;
		unsigned long err;
		if ((err = GetIpNetTable2(addr_sa->sa_family, &table))) {
			LT_E("GetIpNetTable2 failed; err=%ld", err);
			return NF_ERR_FAILED;
		}
		for (MIB_IPNET_ROW2 *row = table->Table; row < table->Table + table->NumEntries; row++) {
			// ignore rows for other interfaces
			if (row->InterfaceIndex != adapter_index)
				continue;
			// ignore non-MAC addresses
			if (row->PhysicalAddressLength != sizeof(net->gw_mac))
				continue;
			ip_addr_t addr = {0};
			nf_sa2ip(&addr, (const struct sockaddr *)&row->Address);
			// look for entries which match the gateway address
			if (nf_ip_cmp(&addr, &net->gw_addr) != 0)
				continue;
			// copy the gateway's MAC address
			nf_mac_cpy(net->gw_mac, row->PhysicalAddress);
			break;
		}
		FreeMibTable(table);
	}
#endif

	return NF_ERR_OK;
}

nf_err_t ethcap_networks_copy_mac(ip_net_t *networks) {
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

ip_net_t *ethcap_networks_identify_pcap(const pcap_if_t *dev) {
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
		nf_err_t err = ethcap_network_identify(net, addr->addr, addr->netmask);
		if (err) {
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

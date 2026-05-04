// Copyright (c) Kuba Szczodrzyński 2026-4-11.

#if WIN32

#include "ethcap_priv.h"

#include <iphlpapi.h>

nf_err_t ethcap_network_fill_addrs(const char *name, ip_net_t *net) {
	unsigned long adapter_index = 0;

	unsigned long family = net->if_addr.length == sizeof(ip6_addr_t) ? AF_INET6 : AF_INET;

	// find the adapter's MAC address and default gateway
	{
		// list available adapters
		unsigned long gaa_flags = GAA_FLAG_INCLUDE_GATEWAYS | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST |
								  GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME;
		unsigned long addrs_len = 64 * 1024;
		IP_ADAPTER_ADDRESSES *adapters;
		NF_MALLOC(adapters, addrs_len, return NF_ERR_MALLOC);
		unsigned long err;
		if ((err = GetAdaptersAddresses(family, gaa_flags, NULL, adapters, &addrs_len))) {
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

	// return early if there's no gateway IP address
	if (net->gw_addr.length == 0)
		return NF_ERR_OK;

	// find an ARP table entry for the default gateway
	{
		// get the IP neighbor table (ARP table)
		MIB_IPNET_TABLE2 *table;
		unsigned long err;
		if ((err = GetIpNetTable2(family, &table))) {
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

	return NF_ERR_OK;
}

#endif

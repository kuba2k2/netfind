// Copyright (c) Kuba Szczodrzyński 2026-5-4.

#if linux

#include "ethcap_priv.h"

#include <linux/if.h>
#include <sys/ioctl.h>

static const char *net_route = "/proc/net/route";
static const char *net_arp	 = "/proc/net/arp";

nf_err_t ethcap_network_fill_addrs(const char *name, ip_net_t *net) {
	// for now, ignore any non-IPv4 networks
	if (net->if_addr.length != sizeof(ip4_addr_t))
		return NF_ERR_OK;

	char gw_addr_str[20];

	// find the interface MAC address
	{
		int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
		if (sock == -1) {
			LT_E("socket open failed; errno=%d", errno);
			return NF_ERR_FAILED;
		}

		struct ifreq s;
		strcpy(s.ifr_name, name);
		if (ioctl(sock, SIOCGIFHWADDR, &s) != 0) {
			LT_E("socket ioctl failed; errno=%d", errno);
			close(sock);
			return NF_ERR_FAILED;
		}
		close(sock);

		nf_mac_cpy(net->if_mac, s.ifr_addr.sa_data);
	}

	// find the default route (gateway IP address)
	{
		// open /proc/net/route
		FILE *f = fopen(net_route, "r");
		if (!f) {
			LT_E("%s open failed; errno=%d", net_route, errno);
			return NF_ERR_FAILED;
		}
		// skip header
		char line[256];
		fgets(line, sizeof(line), f);

		// read each line
		while (fgets(line, sizeof(line), f)) {
			char interface[IFNAMSIZ];
			unsigned long dest_addr, gw_addr;
			sscanf(line, "%s %lx %lx", interface, &dest_addr, &gw_addr);

			// ignore if the interface name doesn't match or destination is not 0.0.0.0
			if (strcmp(interface, name) != 0 || dest_addr != 0)
				continue;

			// store the gateway IP address
			struct sockaddr_in sa;
			sa.sin_family	   = AF_INET;
			sa.sin_addr.s_addr = gw_addr;
			nf_sa2ip(&net->gw_addr, (const struct sockaddr *)&sa);
			break;
		}
		fclose(f);
	}

	// return early if there's no gateway IP address
	if (net->gw_addr.length == 0)
		return NF_ERR_OK;
	nf_ip2str(gw_addr_str, &net->gw_addr);

	// find an ARP table entry for the default gateway
	{
		// open /proc/net/route
		FILE *f = fopen(net_arp, "r");
		if (!f) {
			LT_E("%s open failed; errno=%d", net_arp, errno);
			return NF_ERR_FAILED;
		}
		// skip header
		char line[256];
		fgets(line, sizeof(line), f);

		// read each line
		while (fgets(line, sizeof(line), f)) {
			char ip_addr[20], hw_addr[20], interface[IFNAMSIZ];
			unsigned long flags;
			sscanf(line, "%s 0x%*x 0x%lx %s %*s %s", ip_addr, &flags, hw_addr, interface);

			// ignore if the IP address is different or interface name doesn't match
			if (strcmp(ip_addr, gw_addr_str) != 0 || strcmp(interface, name) != 0)
				continue;

			// store the gateway MAC address
			sscanf(
				hw_addr,
				"%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
				&net->gw_mac[0],
				&net->gw_mac[1],
				&net->gw_mac[2],
				&net->gw_mac[3],
				&net->gw_mac[4],
				&net->gw_mac[5]
			);
			break;
		}
		fclose(f);
	}

	return NF_ERR_OK;
}

#endif

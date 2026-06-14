// Copyright (c) Kuba Szczodrzyński 2026-4-11.

#include "../ethcap_priv.h"

typedef enum __attribute__((packed)) dhcp_opt_t {
	DHCP_SUBNET_MASK			 = 1,
	DHCP_ROUTER					 = 3,
	DHCP_DNS_SERVERS			 = 6,
	DHCP_HOST_NAME				 = 12,
	DHCP_DOMAIN_NAME			 = 15,
	DHCP_NTP_SERVERS_ADDRESSES	 = 42,
	DHCP_REQUESTED_IP_ADDRESS	 = 50,
	DHCP_IP_ADDRESS_LEASE_TIME	 = 51,
	DHCP_MESSAGE_TYPE			 = 53,
	DHCP_SERVER_IDENTIFIER		 = 54,
	DHCP_PARAMETER_REQUEST_LIST	 = 55,
	DHCP_VENDOR_CLASS_IDENTIFIER = 60,
	DHCP_FQDN					 = 81,
} dhcp_opt_t;

typedef PACK(struct ethcap_dhcp_t {
	uint8_t type;
	uint8_t hw_type;
	uint8_t hw_len;
	uint8_t hops;
	uint32_t id;
	uint16_t secs;
	uint16_t flags;
	ip4_addr_t ip_client;
	ip4_addr_t ip_your;
	ip4_addr_t ip_server;
	ip4_addr_t ip_relay;
	mac_addr_t mac_addr;
	uint8_t addr_padding[10];
	char server_hostname[64];
	char boot_file[128];
	uint32_t cookie;
	uint8_t options[];
}) ethcap_dhcp_t;

static void devdb_put_ip_addrs(
	ethcap_cap_t *cap,
	const ethcap_dhcp_t *dhcp,
	const char *key,
	const uint8_t *opt,
	uint8_t opt_len
) {
	char buf[20];
	for (int i = 0; i < opt_len && i + 4 <= opt_len; i += 4) {
		devdb_put(
			/* devdb= */ cap->devdb,
			/* netaddr= */ cap->netaddr,
			/* devaddr= */ dhcp->mac_addr,
			/* key= */ key,
			/* value= */ nf_ip42str(buf, &opt[i]),
			/* append= */ true
		);
	}
}

static void devdb_put_string(
	ethcap_cap_t *cap,
	const ethcap_dhcp_t *dhcp,
	const char *key,
	const uint8_t *opt,
	uint8_t opt_len
) {
	char buf[255];
	memcpy(buf, opt, opt_len);
	buf[opt_len] = '\0';
	devdb_put(
		/* devdb= */ cap->devdb,
		/* netaddr= */ cap->netaddr,
		/* devaddr= */ dhcp->mac_addr,
		/* key= */ key,
		/* value= */ buf,
		/* append= */ true
	);
}

static void dhcp(ethcap_cap_t *cap, const ethcap_dhcp_t *dhcp, unsigned int len) {
	if (len < sizeof(*dhcp))
		return;
	// only handle valid DHCP packets
	if (dhcp->cookie != 0x63538263)
		return;

	len -= sizeof(*dhcp);

	char buf[64];
	if (cap->sport == 67) {
		devdb_put(
			/* devdb= */ cap->devdb,
			/* netaddr= */ cap->netaddr,
			/* devaddr= */ cap->src_macaddr,
			/* key= */ "ip.udp.67.protocol",
			/* value= */ "dhcp-server",
			/* append= */ true
		);
	} else {
		devdb_put(
			/* devdb= */ cap->devdb,
			/* netaddr= */ cap->netaddr,
			/* devaddr= */ cap->src_macaddr,
			/* key= */ "ip.udp.68.protocol",
			/* value= */ "dhcp",
			/* append= */ true
		);
	}

	// go through DHCP options
	dhcp_opt_t opt_type;
	uint8_t opt_len;
	for (const uint8_t *opt = dhcp->options;
		 (opt_type = *opt++) != 0xFF && (opt_len = *opt++, len -= 2) && (len >= opt_len);
		 opt += opt_len, len -= opt_len) {
		// parse supported options
		switch (opt_type) {
			case DHCP_SUBNET_MASK:
				devdb_put_ip_addrs(cap, dhcp, "ip.udp.68.dhcp.subnet", opt, opt_len);
				break;

			case DHCP_ROUTER:
				devdb_put_ip_addrs(cap, dhcp, "ip.udp.68.dhcp.router", opt, opt_len);
				break;

			case DHCP_DNS_SERVERS:
				devdb_put_ip_addrs(cap, dhcp, "ip.udp.68.dhcp.dns", opt, opt_len);
				break;

			case DHCP_HOST_NAME:
				devdb_put_string(cap, dhcp, "ip.udp.68.dhcp.hostname", opt, opt_len);
				break;

			case DHCP_DOMAIN_NAME:
				devdb_put_string(cap, dhcp, "ip.udp.68.dhcp.domain", opt, opt_len);
				break;

			case DHCP_NTP_SERVERS_ADDRESSES:
				devdb_put_ip_addrs(cap, dhcp, "ip.udp.68.dhcp.ntp", opt, opt_len);
				break;

			case DHCP_REQUESTED_IP_ADDRESS:
				devdb_put_ip_addrs(cap, dhcp, "ip.udp.68.dhcp.ipaddr", opt, opt_len);
				break;

			case DHCP_IP_ADDRESS_LEASE_TIME:
				devdb_put(
					/* devdb= */ cap->devdb,
					/* netaddr= */ cap->netaddr,
					/* devaddr= */ dhcp->mac_addr,
					/* key= */ "ip.udp.68.dhcp.lease-time",
					/* value= */ nf_fmt(buf, sizeof(buf), "%lu", ntohl(*(uint32_t *)opt)),
					/* append= */ true
				);
				break;

			case DHCP_MESSAGE_TYPE:
				// abort if message is NAK
				if (*opt == 6)
					return;
				break;

			case DHCP_SERVER_IDENTIFIER:
				devdb_put_ip_addrs(cap, dhcp, "ip.udp.68.dhcp.server", opt, opt_len);
				break;

			case DHCP_PARAMETER_REQUEST_LIST:
				break;

			case DHCP_VENDOR_CLASS_IDENTIFIER:
				devdb_put_string(cap, dhcp, "ip.udp.68.dhcp.vendor", opt, opt_len);
				break;

			case DHCP_FQDN:
				opt += 3, opt_len -= 3;
				devdb_put_string(cap, dhcp, "ip.udp.68.dhcp.fqdn", opt, opt_len);
				break;
		}
	}
}

ETHCAP_ADD_L5(
	/* eth_type= */ ETH_TYPE_ANY,
	/* proto= */ IPPROTO_UDP,
	/* sport= */ 67,
	/* dport= */ 68,
	/* cb= */ dhcp
);
ETHCAP_ADD_L5(
	/* eth_type= */ ETH_TYPE_ANY,
	/* proto= */ IPPROTO_UDP,
	/* sport= */ 68,
	/* dport= */ 67,
	/* cb= */ dhcp
);

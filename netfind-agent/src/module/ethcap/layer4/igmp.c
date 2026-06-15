// Copyright (c) Kuba Szczodrzyński 2026-4-11.

#include "../ethcap_priv.h"

typedef enum __attribute__((packed)) igmp_type_t {
	IGMP_QUERY	  = 0x11,
	IGMPv1_REPORT = 0x12,
	IGMPv2_REPORT = 0x16,
	IGMPv3_REPORT = 0x22,
	IGMP_LEAVE	  = 0x17,
} igmp_type_t;

typedef PACK(struct ethcap_igmp_t {
	igmp_type_t type;
	uint8_t max_resp;
	uint16_t checksum;
	ip4_addr_t group_addr;
}) ethcap_igmp_t;

static void igmp(ethcap_cap_t *cap, const ethcap_igmp_t *igmp, unsigned int len) {
	if (len < sizeof(*igmp))
		return;
	// only handle membership report messages (except IGMPv3, as the format is different)
	if (igmp->type != IGMPv1_REPORT && igmp->type != IGMPv2_REPORT)
		return;

	char buf[20];
	devdb_put(
		/* devdb= */ cap->devdb,
		/* netaddr= */ cap->netaddr,
		/* devaddr= */ cap->src_macaddr,
		/* key= */ "ip/igmp",
		/* value= */ nf_ip42str(buf, igmp->group_addr),
		/* append= */ true
	);
}

ETHCAP_ADD_L4(
	/* eth_type= */ ETH_TYPE_ANY,
	/* proto= */ IPPROTO_IGMP,
	/* cb= */ igmp
);

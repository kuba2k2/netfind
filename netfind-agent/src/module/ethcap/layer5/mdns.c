// Copyright (c) Kuba Szczodrzyński 2026-4-11.

#include "../ethcap_priv.h"

typedef enum __attribute__((packed)) mdns_type_t {
	MDNS_A	  = 1,
	MDNS_PTR  = 12,
	MDNS_TXT  = 16,
	MDNS_AAAA = 28,
	MDNS_SRV  = 33,
} mdns_type_t;

typedef PACK(struct ethcap_mdns_t {
	uint8_t packet[0];
	uint16_t id;
	uint16_t flags;
	uint16_t queries;
	uint16_t answers;
	uint16_t auth_rr;
	uint16_t add_rr;
	uint8_t data[];
}) ethcap_mdns_t;

typedef PACK(struct ethcap_mdns_rr_t {
	uint16_t type;
	uint16_t flags;
	uint32_t ttl;
	uint16_t len;
	uint8_t data[];
}) ethcap_mdns_rr_t;

static void mdns_skip_name(dcpy_t *src);
static void mdns_read_name(const dcpy_t *pkt, dcpy_t *src, dcpy_t *dst);
static void mdns_split_name(char *name, char **instance, char **service, char **protocol, char **suffix);

static void devdb_put_service(ethcap_cap_t *cap, const char *service, const char *key, const char *value);
static void devdb_put_a(ethcap_cap_t *cap, const dcpy_t *pkt, char *name, dcpy_t *src);
static void devdb_put_aaaa(ethcap_cap_t *cap, const dcpy_t *pkt, char *name, dcpy_t *src);
static void devdb_put_ptr(ethcap_cap_t *cap, const dcpy_t *pkt, char *name, dcpy_t *src);
static void devdb_put_srv(ethcap_cap_t *cap, const dcpy_t *pkt, char *name, dcpy_t *src);
static void devdb_put_txt(ethcap_cap_t *cap, const dcpy_t *pkt, char *name, dcpy_t *src);

static void mdns(ethcap_cap_t *cap, const ethcap_mdns_t *mdns, unsigned int len) {
	if (len < sizeof(*mdns))
		return;
	// only handle mDNS responses
	if (!(mdns->flags & 0x80))
		return;

	devdb_put(
		/* devdb= */ cap->devdb,
		/* netaddr= */ cap->netaddr,
		/* devaddr= */ cap->src_macaddr,
		/* key= */ "ip.udp.5353.protocol",
		/* value= */ "mdns"
	);

	const dcpy_t pkt = {
		.buf = (void *)mdns->packet,
		.len = len,
	};
	dcpy_t src = {
		.buf = (void *)mdns->data,
		.len = len - sizeof(*mdns),
	};

	// skip all queries
	for (int i = 0; i < ntohs(mdns->queries); i++) {
		// skip name, type and flags
		mdns_skip_name(&src);
		dcpy_skip(&src, 4);
		if (src.oob)
			return;
	}

	// go through all RRs
	int total_rr = ntohs(mdns->answers) + ntohs(mdns->auth_rr) + ntohs(mdns->add_rr);
	for (int i = 0; i < total_rr; i++) {
		// read record name
		char name[128];
		dcpy_t name_dst = {.buf = (void *)name, .len = sizeof(name)};
		mdns_read_name(&pkt, &src, &name_dst);

		// read record header
		ethcap_mdns_rr_t rr;
		dcpy_get(&src, &rr, sizeof(rr));
		uint16_t rr_len = ntohs(rr.len);
		if (src.oob)
			return;

		// if name buffer overflows, skip the entire record
		if (name_dst.oob) {
			dcpy_skip(&src, rr_len);
			if (src.oob)
				return;
			continue;
		}

		// create a dcpy_t for just the record
		dcpy_t rr_src = {.buf = src.buf, .len = nf_min(rr_len, src.len)};
		switch ((mdns_type_t)ntohs(rr.type)) {
			case MDNS_A:
				devdb_put_a(cap, &pkt, name, &rr_src);
				break;
			case MDNS_PTR:
				devdb_put_ptr(cap, &pkt, name, &rr_src);
				break;
			case MDNS_TXT:
				devdb_put_txt(cap, &pkt, name, &rr_src);
				break;
			case MDNS_AAAA:
				devdb_put_aaaa(cap, &pkt, name, &rr_src);
				break;
			case MDNS_SRV:
				devdb_put_srv(cap, &pkt, name, &rr_src);
				break;
		}

		// advance past the processed record
		dcpy_skip(&src, rr_len);
		if (src.oob)
			return;
	}
}

static void mdns_skip_name(dcpy_t *src) {
	for (;;) {
		// check length or pointer, abort if buffer empty
		uint8_t len = dcpy_get8(src);
		// succeed if no more labels found
		if (len == 0)
			return;
		// skip name pointer
		if (len & 0xC0)
			return dcpy_skip(src, 1);
		// skip name directly
		dcpy_skip(src, len);
	}
}

static void mdns_read_name(const dcpy_t *pkt, dcpy_t *src, dcpy_t *dst) {
	for (int i = 0;; i++) {
		// check length or pointer, abort if buffer empty
		uint8_t len = dcpy_get8(src);
		// succeed if no more labels found
		if (len == 0)
			return dcpy_put8(dst, '\0');
		// add a label separator (.)
		if (i)
			dcpy_put8(dst, '.');
		// read name pointer
		if (len & 0xC0) {
			uint16_t ptr = (len & 0x3F) << 8;
			ptr |= dcpy_get8(src);
			// recurse to read the label being pointed to
			dcpy_t header_src = {.buf = pkt->buf + ptr, .len = pkt->len - ptr};
			return mdns_read_name(pkt, &header_src, dst);
		}
		// read name directly
		dcpy_cpy(dst, src, len);
	}
}

static void mdns_split_name(char *name, char **instance, char **service, char **protocol, char **suffix) {
	// split 'name' by '.' into at most 5 parts
	char *parts[5] = {NULL};
	int num		   = strsplit(name, '.', parts, 5);

	// valid mDNS names have at least 3 parts
	if (num < 3)
		return;
	// second to last part must be either "_tcp" or "_udp"
	if (strcmp(parts[num - 2], "_tcp") != 0 && strcmp(parts[num - 2], "_udp") != 0)
		return;

	// assign each part to an output pointer
	if (suffix)
		*suffix = parts[num - 1];
	num--;
	if (protocol)
		*protocol = parts[num - 1];
	num--;
	if (service)
		*service = parts[num - 1];
	num--;
	if (num && instance)
		*instance = parts[num - 1];
}

static void devdb_put_service(ethcap_cap_t *cap, const char *service, const char *key, const char *value) {
	char buf[128];
	devdb_put(
		/* devdb= */ cap->devdb,
		/* netaddr= */ cap->netaddr,
		/* devaddr= */ cap->src_macaddr,
		/* key= */ nf_fmt(buf, sizeof(buf), "ip.udp.5353.mdns.[%s].%s", service, key),
		/* value= */ value
	);
}

static void devdb_put_a(ethcap_cap_t *cap, const dcpy_t *pkt, char *name, dcpy_t *src) {
	ip4_addr_t ipaddr;
	dcpy_get(src, ipaddr, sizeof(ipaddr));
	if (src->oob)
		return;

	char buf[20 + 128];
	devdb_put(
		/* devdb= */ cap->devdb,
		/* netaddr= */ cap->netaddr,
		/* devaddr= */ cap->src_macaddr,
		/* key= */ nf_fmt(buf + 20, sizeof(buf) - 20, "ip.udp.5353.mdns.host.[%s]", name),
		/* value= */ nf_ip42str(buf, ipaddr)
	);
}

static void devdb_put_aaaa(ethcap_cap_t *cap, const dcpy_t *pkt, char *name, dcpy_t *src) {
	ip6_addr_t ipaddr;
	dcpy_get(src, ipaddr, sizeof(ipaddr));
	if (src->oob)
		return;

	char buf[40 + 128];
	devdb_put(
		/* devdb= */ cap->devdb,
		/* netaddr= */ cap->netaddr,
		/* devaddr= */ cap->src_macaddr,
		/* key= */ nf_fmt(buf + 40, sizeof(buf) - 40, "ip.udp.5353.mdns.host.[%s]", name),
		/* value= */ nf_ip62str(buf, ipaddr)
	);
}

static void devdb_put_ptr(ethcap_cap_t *cap, const dcpy_t *pkt, char *name, dcpy_t *src) {
	// read the PTR target
	char ptr[256]  = {0};
	dcpy_t ptr_dst = {.buf = (void *)ptr, .len = sizeof(ptr)};
	mdns_read_name(pkt, src, &ptr_dst);
	if (src->oob)
		return;

	// split the PTR target, return if not a valid mDNS label
	char *instance = NULL, *service = NULL, *protocol = NULL;
	mdns_split_name(ptr, &instance, &service, &protocol, NULL);
	if (!service)
		return;

	// put protocol and instance from PTR target
	if (protocol)
		devdb_put_service(cap, service, "protocol", protocol);
	if (instance)
		devdb_put_service(cap, service, "name", instance);
}

static void devdb_put_srv(ethcap_cap_t *cap, const dcpy_t *pkt, char *name, dcpy_t *src) {
	// parse the record name, return if not a valid mDNS label
	char *instance = NULL, *service = NULL, *protocol = NULL;
	mdns_split_name(name, &instance, &service, &protocol, NULL);
	if (!service)
		return;

	// skip priority and weight
	dcpy_skip(src, 4);
	// read port number
	uint16_t port = dcpy_get16b(src);
	// read the SRV target
	char srv[256]  = {0};
	dcpy_t srv_dst = {.buf = (void *)srv, .len = sizeof(srv)};
	mdns_read_name(pkt, src, &srv_dst);
	if (src->oob)
		return;

	// put protocol and instance from record name
	if (protocol)
		devdb_put_service(cap, service, "protocol", protocol);
	if (instance)
		devdb_put_service(cap, service, "name", instance);

	// put SRV target
	char buf[128];
	devdb_put_service(cap, service, "srv", nf_fmt(buf, sizeof(buf), "%s:%u", srv, port));
}

static void devdb_put_txt(ethcap_cap_t *cap, const dcpy_t *pkt, char *name, dcpy_t *src) {
	// parse the record name, return if not a valid mDNS label
	char *instance = NULL, *service = NULL, *protocol = NULL;
	mdns_split_name(name, &instance, &service, &protocol, NULL);
	if (!service)
		return;

	// put protocol and instance from record name
	if (protocol)
		devdb_put_service(cap, service, "protocol", protocol);
	if (instance)
		devdb_put_service(cap, service, "name", instance);

	// loop through every TXT key=value pair
	while (src->len) {
		uint8_t len = dcpy_get8(src);
		if (!len)
			continue;
		// read the KV text
		char txt[256];
		dcpy_get(src, txt, len);
		txt[len] = '\0';
		if (src->oob)
			return;
		// split key=value into two parts
		char *kv[2] = {NULL};
		if (strsplit(txt, '=', kv, 2) != 2)
			continue;
		// put TXT key and value
		char buf[128];
		devdb_put_service(cap, service, nf_fmt(buf, sizeof(buf), "txt.[%s]", kv[0]), kv[1]);
	}
}

ETHCAP_ADD_L5(
	/* eth_type= */ ETH_TYPE_ANY,
	/* proto= */ IPPROTO_UDP,
	/* sport= */ 5353,
	/* dport= */ 5353,
	/* cb= */ mdns
);

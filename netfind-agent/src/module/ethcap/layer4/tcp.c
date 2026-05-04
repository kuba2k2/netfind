// Copyright (c) Kuba Szczodrzyński 2026-4-11.

#include "../ethcap_priv.h"

static void tcp(ethcap_cap_t *cap, const void *tcp, unsigned int len) {}

ETHCAP_ADD_L4(
	/* eth_type= */ ETH_TYPE_ANY,
	/* proto= */ IPPROTO_TCP,
	/* cb= */ tcp
);

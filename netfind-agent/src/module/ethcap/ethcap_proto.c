// Copyright (c) Kuba Szczodrzyński 2026-4-12.

#include "ethcap_priv.h"

extern const ethcap_proto_t __start_ethcap_proto_l2[]; // NOLINT(*-reserved-identifier)
extern const ethcap_proto_t __stop_ethcap_proto_l2[];  // NOLINT(*-reserved-identifier)
extern const ethcap_proto_t __start_ethcap_proto_l3[]; // NOLINT(*-reserved-identifier)
extern const ethcap_proto_t __stop_ethcap_proto_l3[];  // NOLINT(*-reserved-identifier)
extern const ethcap_proto_t __start_ethcap_proto_l4[]; // NOLINT(*-reserved-identifier)
extern const ethcap_proto_t __stop_ethcap_proto_l4[];  // NOLINT(*-reserved-identifier)
extern const ethcap_proto_t __start_ethcap_proto_l5[]; // NOLINT(*-reserved-identifier)
extern const ethcap_proto_t __stop_ethcap_proto_l5[];  // NOLINT(*-reserved-identifier)

const ethcap_proto_t *ethcap_proto_l2_start;
const ethcap_proto_t *ethcap_proto_l2_stop;
const ethcap_proto_t *ethcap_proto_l3_start;
const ethcap_proto_t *ethcap_proto_l3_stop;
const ethcap_proto_t *ethcap_proto_l4_start;
const ethcap_proto_t *ethcap_proto_l4_stop;
const ethcap_proto_t *ethcap_proto_l5_start;
const ethcap_proto_t *ethcap_proto_l5_stop;

void ethcap_proto_init() {
	// initialize protocol handler lists using section start/stop symbols
	ethcap_proto_l2_start = __start_ethcap_proto_l2;
	ethcap_proto_l2_stop  = __stop_ethcap_proto_l2;
	ethcap_proto_l3_start = __start_ethcap_proto_l3;
	ethcap_proto_l3_stop  = __stop_ethcap_proto_l3;
	ethcap_proto_l4_start = __start_ethcap_proto_l4;
	ethcap_proto_l4_stop  = __stop_ethcap_proto_l4;
	ethcap_proto_l5_start = __start_ethcap_proto_l5;
	ethcap_proto_l5_stop  = __stop_ethcap_proto_l5;
}

// Copyright (c) Kuba Szczodrzyński 2026-4-6.

#include "netfind.h"

#include "conn/conn.h"
#include "devdb/devdb.h"
#include "module/module.h"

int main(int argc, const char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "usage: %s url [token]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	LT_I("Starting Netfind Agent " GIT_VERSION);

	conn_t *conn = conn_init(/* url= */ argv[1], /* token= */ argc >= 3 ? argv[2] : NULL);
	if (!conn)
		return 1;

	devdb_t *devdb = devdb_init(conn);
	if (!devdb)
		return 1;

	module_t *mod = module_init(devdb, /* ops= */ &ethcap_ops);
	if (!mod)
		return 1;

	nf_err_t err;
	if ((err = module_start(mod)))
		return err;

	while (1) {
		usleep(1000 * 1000);
	}
}

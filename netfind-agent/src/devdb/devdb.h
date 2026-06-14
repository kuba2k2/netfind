// Copyright (c) Kuba Szczodrzyński 2026-4-9.

#pragma once

#include <netfind.h>

typedef struct devdb_t devdb_t;
typedef struct conn_t conn_t;

// devdb.c
devdb_t *devdb_init(conn_t *conn);
void devdb_free(devdb_t *devdb);
void devdb_put(
	devdb_t *devdb,
	const mac_addr_t netaddr,
	const mac_addr_t devaddr,
	const char *key,
	const char *value,
	bool append
);

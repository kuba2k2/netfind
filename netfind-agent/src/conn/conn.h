// Copyright (c) Kuba Szczodrzyński 2026-5-22.

#pragma once

#include <netfind.h>

typedef struct conn_t conn_t;
typedef struct conn_msg_t conn_msg_t;
typedef bool (*conn_cb_func_t)(void *param, const conn_msg_t *msg);

typedef enum conn_msg_type_t {
	CONN_MSG_PUB   = 0,
	CONN_MSG_SUB   = 1,
	CONN_MSG_GET   = 2,
	CONN_MSG_DEL   = 3,
	CONN_MSG_UNSUB = 4,
	CONN_MSG_LWT   = 5,
} conn_msg_type_t;

typedef enum conn_pub_mode_t {
	CONN_PUB_SEND	 = 0,
	CONN_PUB_RETAIN	 = 1,
	CONN_PUB_PERSIST = 2,
	CONN_PUB_APPEND	 = 3
} conn_pub_mode_t;

// conn.c
conn_t *conn_init(const char *url, const char *token);
void conn_free(conn_t *conn);

// conn_msg.c
void conn_msg_add(
	conn_t *conn,
	conn_msg_type_t type,
	const char *topic,
	const char *value,
	uint64_t created_at,
	uint64_t updated_at,
	conn_pub_mode_t mode
);
void conn_pub(
	conn_t *conn,
	const char *topic,
	const char *value,
	uint64_t created_at,
	uint64_t updated_at,
	conn_pub_mode_t mode
);
void conn_get(conn_t *conn, const char *topic);
void conn_del(conn_t *conn, const char *topic);
void conn_lwt(conn_t *conn, const char *topic, const char *value);
void conn_sub(conn_t *conn, const char *topic, conn_cb_func_t func, void *param);
void conn_unsub(conn_t *conn, const char *topic, conn_cb_func_t func);
void conn_msgs_dispatch(conn_t *conn, const conn_msg_t *msgs);

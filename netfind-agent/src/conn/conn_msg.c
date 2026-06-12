// Copyright (c) Kuba Szczodrzyński 2026-5-24.

#include "conn_priv.h"

void conn_msg_add(
	conn_t *conn,
	conn_msg_type_t type,
	const char *key,
	const char *value,
	time_t value_at,
	conn_pub_mode_t mode
) {
	conn_msg_t *msg = calloc(1, sizeof(*msg));
	if (!msg)
		return;
	msg->type	  = type;
	msg->key	  = strdup(key);
	msg->value	  = value ? strdup(value) : NULL;
	msg->value_at = value_at;
	msg->mode	  = mode;
	DL_APPEND(conn->send_msgs, msg);
}

void conn_msg_free(conn_msg_t *msg) {
	if (!msg)
		return;
	free(msg->key);
	free(msg->value);
	free(msg);
}

void conn_msgs_free(conn_msg_t *msgs) {
	if (!msgs)
		return;
	conn_msg_t *msg = NULL, *tmp = NULL;
	DL_FOREACH_SAFE(msgs, msg, tmp) {
		conn_msg_free(msg);
	}
}

void conn_pub(conn_t *conn, const char *key, const char *value, time_t value_at, conn_pub_mode_t mode) {
	if (!key)
		return;
	conn_msg_add(conn, CONN_MSG_PUB, key, value, value_at, mode);
}

void conn_get(conn_t *conn, const char *key) {
	if (!key)
		return;
	conn_msg_add(conn, CONN_MSG_GET, key, NULL, 0, 0);
}

void conn_del(conn_t *conn, const char *key) {
	if (!key)
		return;
	conn_msg_add(conn, CONN_MSG_DEL, key, NULL, 0, 0);
}

void conn_lwt(conn_t *conn, const char *key, const char *value) {
	if (!key || !value)
		return;
	conn_msg_add(conn, CONN_MSG_LWT, key, value, 0, 0);
}

void conn_sub(conn_t *conn, const char *key, conn_cb_func_t func, void *param) {
	if (!key || !func)
		return;
	conn_cb_t *cb = calloc(1, sizeof(*cb));
	if (!cb)
		return;
	cb->key	  = strdup(key);
	cb->func  = func;
	cb->param = param;
	DL_APPEND(conn->recv_cbs, cb);

	conn_msg_add(conn, CONN_MSG_SUB, key, NULL, 0, 0);
}

void conn_unsub(conn_t *conn, const char *key, conn_cb_func_t func) {
	if (!key || !func)
		return;
	conn_cb_t *cb = NULL, *tmp = NULL;
	DL_FOREACH_SAFE(conn->recv_cbs, cb, tmp) {
		if (strcmp(cb->key, key) != 0 || cb->func != func)
			continue;
		DL_DELETE(conn->recv_cbs, cb);
		free(cb->key);
		free(cb);
		break;
	}

	if (cb != NULL)
		conn_msg_add(conn, CONN_MSG_UNSUB, key, NULL, 0, 0);
}

void conn_msgs_dispatch(conn_t *conn, const conn_msg_t *msgs) {
	const conn_msg_t *msg = NULL;
	DL_FOREACH(msgs, msg) {
		LT_F("conn / %s / %s", msg->key, msg->value);
	}
}

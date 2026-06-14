// Copyright (c) Kuba Szczodrzyński 2026-6-14.

#include "conn_priv.h"

void conn_cb_free(conn_cb_t *cb) {
	if (!cb)
		return;
	free(cb->topic);
	free(cb);
}

void conn_cbs_free(conn_cb_t *cbs) {
	if (!cbs)
		return;
	conn_cb_t *cb = NULL, *tmp = NULL;
	DL_FOREACH_SAFE(cbs, cb, tmp) {
		conn_cb_free(cb);
	}
}

void conn_msg_free(conn_msg_t *msg) {
	if (!msg)
		return;
	free(msg->topic);
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

// Copyright (c) Kuba Szczodrzyński 2026-5-22.

#pragma once

#include "conn.h"

#include <curl/curl.h>
#include <pb_decode.h>
#include <pb_encode.h>

typedef struct conn_msg_t {
	conn_msg_type_t type;
	char *topic;
	char *value;
	uint64_t created_at;
	uint64_t updated_at;
	conn_pub_mode_t mode;
	struct conn_msg_t *prev, *next;
} conn_msg_t;

typedef struct conn_cb_t {
	char *topic;
	conn_cb_func_t func;
	void *param;
	struct conn_cb_t *prev, *next;
} conn_cb_t;

typedef struct conn_t {
	char *url;
	char *token;

	pthread_t thread;

	conn_msg_t *send_msgs;
	pthread_mutex_t send_msgs_mutex;

	conn_cb_t *recv_cbs;
	pthread_mutex_t recv_cbs_mutex;
} conn_t;

// conn.c
void *conn_thread(conn_t *conn);

// conn_pb_stream.c
pb_istream_t pb_istream_from_curl(CURL *curl);

// conn_pb_msg.c
bool pb_encode_field_string(pb_ostream_t *stream, const pb_field_iter_t *field, void *const *arg);
bool pb_decode_field_string(pb_istream_t *stream, const pb_field_t *field, void **arg);
bool pb_encode_Message(pb_ostream_t *stream, const conn_msg_t *msg, unsigned int flags);
bool pb_decode_Message(pb_istream_t *stream, conn_msg_t *msg, unsigned int flags);
bool pb_encode_field_messages(pb_ostream_t *stream, const pb_field_iter_t *field, void *const *arg);
bool pb_decode_field_messages(pb_istream_t *stream, const pb_field_iter_t *field, void **arg);
bool pb_encode_MessageList(pb_ostream_t *stream, const conn_msg_t *msgs, unsigned int flags);
bool pb_decode_MessageList(pb_istream_t *stream, conn_msg_t **msgs, unsigned int flags);

// conn_util.c
void conn_cb_free(conn_cb_t *cb);
void conn_cbs_free(conn_cb_t *cbs);
void conn_msg_free(conn_msg_t *msg);
void conn_msgs_free(conn_msg_t *msgs);

// conn_ws.c
CURLcode conn_ws_send(conn_t *conn, CURL *curl);
CURLcode conn_ws_recv(conn_t *conn, CURL *curl);

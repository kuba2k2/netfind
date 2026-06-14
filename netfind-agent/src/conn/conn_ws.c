// Copyright (c) Kuba Szczodrzyński 2026-6-10.

#include "conn_priv.h"

CURLcode conn_ws_send(conn_t *conn, CURL *curl) {
	CURLcode err = CURLE_OK;

	// wait for messages from the send queue
	conn_msg_t *msgs = NULL;
	NF_WITH_MUTEX(conn->send_msgs_mutex) {
		msgs = conn->send_msgs;
		if (msgs == NULL)
			// no messages, break out of mutex loop
			continue;
		// take all enqueued messages
		conn->send_msgs = NULL;
	}
	if (msgs == NULL)
		return CURLE_AGAIN;

	// build a Protobuf output stream for sizing
	pb_ostream_t size_stream = PB_OSTREAM_SIZING;
	// call pb_encode() to get message encoded length
	bool ret = pb_encode_MessageList(&size_stream, msgs, 0);
	if (!ret)
		goto out;

	// allocate data for the encoded message
	uint8_t *buf;
	size_t buf_len = size_stream.bytes_written;
	NF_MALLOC(buf, buf_len, goto out);

	// build a Protobuf output stream with the new buffer
	pb_ostream_t stream = pb_ostream_from_buffer(buf, buf_len);
	// call pb_encode() again to actually encode the message
	ret = pb_encode_MessageList(&stream, msgs, 0);
	if (!ret)
		goto out;

	// count messages for logging purposes
	conn_msg_t *msg = NULL;
	int msg_count	= 0;
	DL_COUNT(msgs, msg, msg_count);
	LT_I("Sending %d message(s), total length %llu", msg_count, buf_len);

	// send the entire message
	do {
		err = curl_ws_send(
			/* curl= */ curl,
			/* buffer_arg= */ buf,
			/* buflen= */ buf_len,
			/* sent= */ NULL,
			/* fragsize= */ 0,
			/* flags= */ CURLWS_BINARY
		);
	} while (err == CURLE_AGAIN);

out:
	// show an error if encoding failed
	if (!ret)
		LT_W("Message encode failed");
	// show an error if sending failed
	if (err != CURLE_OK) {
		LT_E("WebSocket send failed; err=%s", curl_easy_strerror(err));
		// put the failed messages back on the queue
		NF_WITH_MUTEX(conn->send_msgs_mutex) {
			DL_CONCAT(conn->send_msgs, msgs);
		}
	} else {
		// free all messages
		conn_msg_free(msgs);
	}
	return err;
}

CURLcode conn_ws_recv(conn_t *conn, CURL *curl) {
	// wait for frames from the WebSocket
	size_t recv;
	const struct curl_ws_frame *meta;
	CURLcode err = curl_ws_recv(
		/* curl= */ curl,
		/* buffer_arg= */ NULL,
		/* buflen= */ 0,
		/* recv= */ &recv,
		/* metap= */ &meta
	);

	// quietly accept E_AGAIN
	if (err == CURLE_AGAIN)
		return err;
	// report any other errors
	if (err != CURLE_OK)
		NF_ERR(E, return err, "WebSocket recv failed; err=%s", curl_easy_strerror(err));
	// ignore everything that's not a binary frame
	if (!(meta->flags & CURLWS_BINARY))
		return CURLE_AGAIN;

	// build a Protobuf input stream
	pb_istream_t stream = pb_istream_from_curl(curl);
	conn_msg_t *msgs	= NULL;
	// call pb_decode() to receive a message list
	bool ret = pb_decode_MessageList(&stream, &msgs, 0);
	if (!ret)
		NF_ERR(W, return CURLE_AGAIN, "Message decode failed");

	// skip whatever's left of the frame
	if (stream.bytes_left)
		pb_read(&stream, NULL, SIZE_MAX);

	// dispatch received messages to subscribers
	if (msgs)
		conn_msgs_dispatch(conn, msgs);
	// finally free all received messages
	conn_msgs_free(msgs);

	return CURLE_OK;
}

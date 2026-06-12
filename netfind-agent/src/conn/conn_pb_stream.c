// Copyright (c) Kuba Szczodrzyński 2026-5-24.

#include "conn_priv.h"

static bool read_callback(pb_istream_t *stream, uint8_t *buf, size_t count) {
	// copy EOF state from substreams
	if (!stream->state) {
		stream->bytes_left = 0;
		return false;
	}

	size_t recv;
	const struct curl_ws_frame *meta;
	while (count) {
		int err = curl_ws_recv(
			/* curl= */ stream->state,
			/* buffer_arg= */ buf,
			/* buflen= */ count,
			/* recv= */ &recv,
			/* metap= */ &meta
		);

		// quietly accept E_AGAIN
		if (err == CURLE_AGAIN)
			continue;
		// report any other errors
		if (err != CURLE_OK)
			NF_ERR(E, return false, "WebSocket recv failed in callback; err=%s", curl_easy_strerror(err));
		// fail if something like this ever happens
		if (recv > count)
			NF_ERR(E, return false, "WebSocket recv more than count; recv=%llu, count=%llu", recv, count);
		// ignore everything that's not a binary frame
		if (!(meta->flags & CURLWS_BINARY))
			continue;

		// decrement remaining count
		count -= recv;

		// report EOF if there are no more bytes in a final frame
		if (meta->bytesleft == 0 && !(meta->flags & CURLWS_CONT)) {
			stream->bytes_left = 0;
			stream->state	   = NULL;
			break;
		}
	}
	return count == 0;
}

pb_istream_t pb_istream_from_curl(CURL *curl) {
	pb_istream_t stream = {
		&read_callback,
		curl,
		SIZE_MAX,
	};
	return stream;
}

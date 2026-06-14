// Copyright (c) Kuba Szczodrzyński 2026-5-22.

#include "conn_priv.h"

conn_t *conn_init(const char *url, const char *token) {
	if (!url || !token)
		return NULL;

	int err;
	conn_t *conn;
	NF_MALLOC(conn, sizeof(*conn), return NULL);

	conn->url	= strdup(url);
	conn->token = strdup(token);

	if ((err = curl_global_init(CURL_GLOBAL_ALL)))
		NF_ERR(E, goto err, "curl init failed; err=%s", curl_easy_strerror(err));

	if ((err = pthread_mutex_init(&conn->send_msgs_mutex, NULL)))
		NF_ERR(E, goto err, "Messages mutex init failed; err=%d", err);

	if ((err = pthread_mutex_init(&conn->recv_cbs_mutex, NULL)))
		NF_ERR(E, goto err, "Callbacks mutex init failed; err=%d", err);

	if ((err = pthread_create(&conn->thread, NULL, (void *)conn_thread, conn)))
		NF_ERR(E, goto err, "Connection thread failed; err=%d", err);

	return conn;

err:
	conn_free(conn);
	return NULL;
}

void conn_free(conn_t *conn) {
	if (!conn)
		return;
	if (conn->thread) {
		pthread_cancel(conn->thread);
		pthread_join(conn->thread, NULL);
	}

	if (conn->recv_cbs_mutex)
		pthread_mutex_destroy(&conn->recv_cbs_mutex);
	if (conn->send_msgs_mutex)
		pthread_mutex_destroy(&conn->send_msgs_mutex);

	curl_global_cleanup();
	conn_msgs_free(conn->send_msgs);
	conn_cbs_free(conn->recv_cbs);
	free(conn->url);
	free(conn->token);
	free(conn);
}

void *conn_thread(conn_t *conn) {
	while (1) {
		CURL *curl = curl_easy_init();
		if (!curl)
			NF_ERR(E, break, "curl easy init failed");

		// set options and connect
		curl_easy_setopt(curl, CURLOPT_URL, conn->url);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 2L);
		LT_I("curl connecting...");
		int err = curl_easy_perform(curl);
		LT_I("curl connected");

		// loop while the connection is active
		while (err == CURLE_OK || err == CURLE_AGAIN) {
			err = conn_ws_send(conn, curl);
			if (err == CURLE_AGAIN)
				usleep(10 * 1000);
			else if (err != CURLE_OK)
				break;

			err = conn_ws_recv(conn, curl);
			if (err == CURLE_AGAIN)
				usleep(10 * 1000);
			else if (err != CURLE_OK)
				break;
		}

		// close the connection
		curl_ws_send(curl, "", 0, NULL, 0, CURLWS_CLOSE);
		curl_easy_cleanup(curl);

		if (err != CURLE_GOT_NOTHING)
			LT_E("curl connection failed; err=%s", curl_easy_strerror(err));
		else
			LT_I("curl connection closed");

		LT_W("Retrying in 5 seconds...");
		usleep(5000 * 1000);
	}
}

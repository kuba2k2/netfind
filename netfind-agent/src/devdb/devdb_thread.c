// Copyright (c) Kuba Szczodrzyński 2026-6-14.

#include "devdb_priv.h"

static struct timespec *set_timespec(struct timespec *ts, uint64_t millis) {
	clock_gettime(CLOCK_REALTIME, ts);
	ts->tv_sec += millis / 1000L;
	ts->tv_nsec += (millis % 1000L) * 1000L * 1000L;
	return ts;
}

void *devdb_thread(devdb_t *devdb) {
	struct timespec ts;

	while (1) {
		// reset the semaphore before waiting
		while (sem_trywait(&devdb->records_sem) == 0) {}

		// wait for a signal, but still loop periodically
		int ret = sem_timedwait(&devdb->records_sem, set_timespec(&ts, DEVDB_DB_CHECK_INTERVAL));
		if (ret != 0 && errno != ETIMEDOUT)
			NF_ERR(E, return NULL, "Semaphore wait failed; errno=%d", errno);

		// if the semaphore was signalled, throttle it with MIN_SEND interval
		uint64_t start = millis();
		while (sem_timedwait(&devdb->records_sem, set_timespec(&ts, DEVDB_MIN_SEND_INTERVAL)) == 0) {
			// ...but only until the interval elapses twice
			// this safeguards against records sent continuously being throttled forever
			if ((millis() - start) >= DEVDB_MIN_SEND_INTERVAL * 2)
				break;
		}

		// lock the records list
		NF_WITH_MUTEX(devdb->records_mutex) {
			uint64_t now = millis();

			// check the records database for anything new to send
			devdb_record_t *record = NULL;
			DL_FOREACH(devdb->records, record) {
				if ((now - record->sent_at) < DEVDB_MAX_SEND_INTERVAL)
					// record already sent recently
					continue;
				if (record->sent_at > record->updated_at)
					// record not updated since last sent
					continue;

				// record should be sent - build a message topic
				char topic[512 + 1];
				char buf[40];
				snprintf(
					/* stream= */ topic,
					/* n= */ sizeof(topic),
					/* format= */ "/network/%s/device/%s/%s",
					nf_mac2str(buf, record->netaddr, '\0'),
					nf_mac2str(buf + 20, record->devaddr, '\0'),
					record->key
				);

				// finally, publish the message
				conn_pub(
					/* conn= */ devdb->conn,
					/* topic= */ topic,
					/* value= */ record->value,
					/* created_at= */ record->created_at,
					/* updated_at= */ record->updated_at,
					/* mode= */ record->append ? CONN_PUB_APPEND : CONN_PUB_PERSIST
				);

				// reset the last sent time
				record->sent_at = now;
			}
		}

		// why not?
		usleep(10 * 1000);
	}
}

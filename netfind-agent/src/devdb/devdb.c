// Copyright (c) Kuba Szczodrzyński 2026-6-14.

#include "devdb_priv.h"

devdb_t *devdb_init(conn_t *conn) {
	if (!conn)
		return NULL;

	int err;
	devdb_t *devdb;
	NF_MALLOC(devdb, sizeof(*devdb), return NULL);

	devdb->conn = conn;

	if ((err = sem_init(&devdb->records_sem, 0, 0)))
		NF_ERR(E, goto err, "Semaphore init failed; err=%d", err);

	if ((err = pthread_mutex_init(&devdb->records_mutex, NULL)))
		NF_ERR(E, goto err, "Mutex init failed; err=%d", err);
	devdb->records_mutex_init = true;

	if ((err = pthread_create(&devdb->thread, NULL, (void *)devdb_thread, devdb)))
		NF_ERR(E, goto err, "Devdb thread failed; err=%d", err);

	return devdb;

err:
	devdb_free(devdb);
	return NULL;
}

void devdb_free(devdb_t *devdb) {
	if (!devdb)
		return;
	if (devdb->thread) {
		pthread_cancel(devdb->thread);
		pthread_join(devdb->thread, NULL);
	}

	if (devdb->records_mutex_init)
		pthread_mutex_destroy(&devdb->records_mutex);
	if (devdb->records_sem)
		sem_destroy(&devdb->records_sem);

	devdb_record_t *record, *tmp;
	DL_FOREACH_SAFE(devdb->records, record, tmp) {
		DL_DELETE(devdb->records, record);
		devdb_record_free(record);
	}

	free(devdb);
}

devdb_record_t *devdb_record_init() {
	devdb_record_t *record;
	NF_MALLOC(record, sizeof(*record), return NULL);
	return record;
}

void devdb_record_free(devdb_record_t *record) {
	if (!record)
		return;
	free(record->key);
	free(record->value);
	free(record);
}

void devdb_put(
	devdb_t *devdb,
	const mac_addr_t netaddr,
	const mac_addr_t devaddr,
	const char *key,
	const char *value,
	bool append
) {
	if (!key || !value)
		return;

	// fetch the current timestamp, in milliseconds
	uint64_t now   = millis();
	bool do_signal = false;

	// lock the records list
	NF_WITH_MUTEX(devdb->records_mutex) {
		// find a record with a matching addresses and key (and perhaps value)
		devdb_record_t *record = NULL;
		DL_FOREACH(devdb->records, record) {
			// compare 'netaddr', 'devaddr' and 'key'
			if (nf_mac_cmp(netaddr, record->netaddr) != 0)
				continue;
			if (nf_mac_cmp(devaddr, record->devaddr) != 0)
				continue;
			if (!record->key || strcmp(key, record->key) != 0)
				continue;

			// if append=false, take any matching record
			if (!append)
				break;
			// otherwise additionally compare 'value'
			if (record->value && strcmp(value, record->value) == 0)
				break;
		}

		if (record == NULL) {
			// create a new record if nothing was found
			record = devdb_record_init();
			if (!record)
				continue;
			nf_mac_cpy(record->netaddr, netaddr);
			nf_mac_cpy(record->devaddr, devaddr);
			record->key		   = strdup(key);
			record->created_at = now;
			// append to the records list
			DL_APPEND(devdb->records, record);
		} else if (!append) {
			// for existing non-append records, reset the last send time
			// as soon as the value changes
			if (record->value && strcmp(value, record->value) != 0)
				record->sent_at = 0;
		}

		// update the value
		free(record->value);
		record->value = strdup(value);
		// update other fields
		record->append	   = append;
		record->updated_at = now;

		// check if the record's last send time is older than the threshold
		if ((now - record->sent_at) > DEVDB_MAX_SEND_INTERVAL)
			do_signal = true;
	}

	// signal the devdb thread that a record was updated
	if (do_signal)
		sem_post(&devdb->records_sem);
}

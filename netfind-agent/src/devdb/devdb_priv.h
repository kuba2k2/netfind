// Copyright (c) Kuba Szczodrzyński 2026-4-9.

#pragma once

#include "devdb.h"

#include "conn/conn.h"

#define DEVDB_MIN_SEND_INTERVAL (120)
#define DEVDB_DB_CHECK_INTERVAL (2000)
#define DEVDB_MAX_SEND_INTERVAL (20000)

typedef struct devdb_record_t {
	mac_addr_t netaddr;	 //!< Network address
	mac_addr_t devaddr;	 //!< Device address
	char *key;			 //!< Record key
	char *value;		 //!< Record value
	bool append;		 //!< Create a new record instead of replacing
	uint64_t created_at; //!< Record first created time
	uint64_t updated_at; //!< Record last updated time
	uint64_t sent_at;	 //!< Time of last sent update

	struct devdb_record_t *prev, *next;
} devdb_record_t;

typedef struct devdb_t {
	conn_t *conn;
	pthread_t thread;

	devdb_record_t *records;	   //!< Records database
	pthread_mutex_t records_mutex; //!< Mutex for locking records
	sem_t records_sem;			   //!< Semaphore for signalling record changes
} devdb_t;

// devdb.c
devdb_record_t *devdb_record_init();
void devdb_record_free(devdb_record_t *record);

// devdb_thread.c
void *devdb_thread(devdb_t *devdb);

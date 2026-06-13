/*
 * Copyright (c) Kuba Szczodrzyński 2026-6-13.
 */

DROP TABLE IF EXISTS client;
DROP TABLE IF EXISTS topic;

CREATE TABLE client (
	id    INTEGER PRIMARY KEY AUTOINCREMENT,
	name  TEXT NOT NULL,
	token TEXT NOT NULL
);

CREATE TABLE topic (
	id         INTEGER PRIMARY KEY AUTOINCREMENT,
	topic      TEXT    NOT NULL,
	value      TEXT,
	created_at INTEGER NOT NULL,
	updated_at INTEGER NOT NULL,
	mode       INTEGER NOT NULL,
	client_id  INTEGER NOT NULL,
	FOREIGN KEY (client_id) REFERENCES client (id)
);
CREATE UNIQUE INDEX topic_value ON topic (topic, value);

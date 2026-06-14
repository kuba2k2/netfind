#  Copyright (c) Kuba Szczodrzyński 2026-6-13.

import sqlite3
from datetime import datetime
from logging import warning
from time import time

import click
from flask import Flask, current_app, g

from .proto import MessageType, NetfindMessage, Pattern, PublishMode

LAST_CHECKPOINT = [0.0]


class Database:
    db: sqlite3.Connection

    def __init__(self):
        self.db = sqlite3.connect(
            database=current_app.config["DATABASE"],
            timeout=10.0,
            detect_types=sqlite3.PARSE_DECLTYPES,
        )
        self.db.execute("PRAGMA journal_mode = WAL")
        self.db.row_factory = sqlite3.Row

    def recreate(self) -> None:
        with current_app.open_resource("db.sql") as f:
            self.db.executescript(f.read().decode("utf-8"))

    def wal_checkpoint(self) -> None:
        if (time() - LAST_CHECKPOINT[0]) < 10.0:
            return
        LAST_CHECKPOINT[0] = time()
        try:
            self.db.execute("PRAGMA wal_checkpoint(RESTART)")
        except sqlite3.OperationalError as e:
            warning(f"WAL checkpoint failed: {e}")

    def close(self) -> None:
        self.wal_checkpoint()
        self.db.close()

    def commit(self) -> None:
        self.db.commit()

    def rollback(self) -> None:
        self.db.rollback()

    def get(self, topic_or_pattern: str | Pattern) -> list[NetfindMessage]:
        query_topic = topic_or_pattern
        if isinstance(topic_or_pattern, Pattern):
            query_topic = query_topic.replace("/+/", "/%/")
            query_topic = query_topic.replace("/#", "/%")
        # fetch all matching rows
        rows = self.db.execute(
            "SELECT * FROM topic WHERE topic LIKE ?",
            (query_topic,),
        ).fetchall()
        # map to NetfindMessage
        return [
            NetfindMessage(
                type=MessageType.PUB,
                topic=row["topic"],
                value=row["value"],
                created_at=row["created_at"],
                updated_at=row["updated_at"],
                mode=PublishMode(row["mode"]),
            )
            for row in rows
        ]

    def store(self, msg: NetfindMessage) -> None:
        self.db.execute(
            "INSERT INTO topic (topic, value, created_at, updated_at, mode, client_id) "
            "VALUES (?, ?, ?, ?, ?, 1) "
            "ON CONFLICT (topic, value) DO "
            "UPDATE SET updated_at = ?, mode = ?",
            (
                msg.topic,
                msg.value,
                msg.created_at,
                msg.updated_at,
                msg.mode,
                msg.updated_at,
                msg.mode,
            ),
        )

    def delete_topic(self, topic: str) -> None:
        self.db.execute(
            "DELETE FROM topic WHERE topic = ?",
            (topic,),
        )

    def delete_value(self, topic: str, value: str) -> None:
        self.db.execute(
            "DELETE FROM topic WHERE topic = ? AND value = ?",
            (topic, value),
        )

    def delete_other(self, topic: str, value: str) -> None:
        self.db.execute(
            "DELETE FROM topic WHERE topic = ? AND value <> ?",
            (topic, value),
        )


def get_db() -> Database:
    if "db" not in g:
        g.db = Database()
    return g.db


def close_db(e=None):
    db = g.pop("db", None)
    if db is not None:
        db.close()


def app_init_db(app: Flask):
    app.teardown_appcontext(close_db)
    app.cli.add_command(init_db_command)


@click.command("init-db")
def init_db_command():
    db = get_db()
    db.recreate()
    click.echo("Database initialized")


sqlite3.register_converter(
    "timestamp",
    lambda v: datetime.fromisoformat(v.decode()),
)

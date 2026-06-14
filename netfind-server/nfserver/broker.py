#  Copyright (c) Kuba Szczodrzyński 2026-6-13.

from collections import defaultdict
from logging import info
from threading import Lock

from .db import get_db
from .proto import (
    Connection,
    MessageType,
    NetfindMessage,
    Pattern,
    PublishMode,
    SendQueue,
)


class Broker:
    msgs: dict[str, NetfindMessage]  # {topic: message}
    conn: set[Connection]  # {...clients}
    sub: dict[Pattern, set[Connection]]  # {pattern: {...subscribers}}
    lwt: dict[Connection, tuple[str, str]]  # {client: (topic, value)}
    msgs_lock: Lock
    conn_lock: Lock
    sub_lock: Lock
    lwt_lock: Lock

    def __init__(self):
        self.msgs = {}
        self.conn = set()
        self.sub = defaultdict(set)
        self.lwt = {}
        self.msgs_lock = Lock()
        self.conn_lock = Lock()
        self.sub_lock = Lock()
        self.lwt_lock = Lock()

    def add_connection(self, conn: Connection) -> None:
        # add the connection
        with self.conn_lock:
            self.conn.add(conn)

    def del_connection(self, conn: Connection) -> None:
        # discard the connection
        with self.conn_lock:
            self.conn.discard(conn)
        # discard all subscriptions
        with self.sub_lock:
            for _, subs in self.sub.items():
                subs.discard(conn)
        # check if there is an LWT set
        with self.lwt_lock:
            lwt = self.lwt.pop(conn, None)
        if not lwt:
            return
        # publish LWT as a RETAIN message
        msg = NetfindMessage(
            type=MessageType.PUB,
            topic=lwt[0],
            value=lwt[1],
            created_at=NetfindMessage.now(),
            updated_at=NetfindMessage.now(),
            mode=PublishMode.RETAIN,
        )
        info(f"Publishing LWT of {conn} as {msg.topic} = '{msg.value}'")
        self.accept_pub_del(conn=None, msgs=[msg])

    def get_subs(self, topic_or_pattern: str | Pattern) -> set[Connection]:
        out = set()
        # read subscribers that are currently connected
        with self.sub_lock:
            for pattern, subs in self.sub.items():
                if pattern.matches(topic_or_pattern):
                    out |= subs
        return out

    def accept_pub_del(
        self,
        conn: Connection | None,
        msgs: list[NetfindMessage],
    ) -> None:
        queue = SendQueue()
        # process PUB/DEL messages, collect everything to dispatch to subscribers
        msgs = self.save_msgs(msgs)
        for msg in msgs:
            topic_or_pattern = (
                msg.topic if msg.type == MessageType.PUB else Pattern(msg.topic)
            )
            for sub_conn in self.get_subs(topic_or_pattern):
                queue.append(sub_conn, msg)
        # avoid sending messages back to the sender
        queue.discard(conn)
        queue.send()

    def accept_get_sub_unsub(
        self,
        conn: Connection,
        msgs: list[NetfindMessage],
    ) -> None:
        queue = SendQueue()
        # process GET/SUB/UNSUB messages, collect everything to send to 'conn'
        for msg in msgs:
            pattern = Pattern(msg.topic)
            if msg.type == MessageType.GET:
                queue.extend(conn, self.load_msgs(pattern))
            elif msg.type == MessageType.SUB:
                info(f"Subscribing {conn} to {pattern}")
                with self.sub_lock:
                    self.sub[pattern].add(conn)
                queue.extend(conn, self.load_msgs(pattern))
            elif msg.type == MessageType.UNSUB:
                info(f"Unsubscribing {conn} from {pattern}")
                with self.sub_lock:
                    self.sub[pattern].discard(conn)
        queue.send()

    def accept_lwt(
        self,
        conn: Connection,
        msgs: list[NetfindMessage],
    ) -> None:
        # process LWT messages (should only ever be one)
        for msg in msgs:
            info(f"Setting LWT of {conn} to {msg.topic} = '{msg.value}'")
            with self.lwt_lock:
                self.lwt[conn] = (msg.topic, msg.value)

    def load_msgs(self, pattern: Pattern) -> list[NetfindMessage]:
        db = get_db()
        out = []

        # read messages with RETAIN mode
        with self.msgs_lock:
            for topic, msg in self.msgs.items():
                if pattern.matches(topic):
                    out.append(msg)

        # read messages with PERSIST/APPEND mode
        for msg in db.get(pattern):
            if pattern.matches(msg.topic):
                out.append(msg)
        return out

    def save_msgs(self, msgs: list[NetfindMessage]) -> list[NetfindMessage]:
        db = get_db()

        for msg in msgs:
            match msg.type, msg.mode:
                case MessageType.PUB, PublishMode.SEND:
                    # do nothing, just send the message
                    pass

                case MessageType.PUB, PublishMode.RETAIN:
                    # store in retain cache
                    with self.msgs_lock:
                        # set created_at from the cache, if present
                        if prev_msg := self.msgs.get(msg.topic):
                            msg.created_at = prev_msg.created_at
                        # save to retain cache
                        self.msgs[msg.topic] = msg

                case MessageType.PUB, _:
                    # delete from retain cache
                    with self.msgs_lock:
                        self.msgs.pop(msg.topic, None)
                    # retrieve all persisted messages with this topic
                    prev_msgs = db.get(msg.topic)
                    match msg.mode:
                        case PublishMode.PERSIST:
                            # get 'created_at' from this topic's oldest timestamp
                            if prev_msgs:
                                msg.created_at = min(m.created_at for m in prev_msgs)
                            # delete all messages with this topic but a different value
                            db.delete_other(msg.topic, msg.value)
                        case PublishMode.APPEND:
                            # get 'created_at' only if this specific value was already saved
                            for prev_msg in prev_msgs:
                                if msg.value == prev_msg.value:
                                    msg.created_at = prev_msg.created_at
                                    break
                    # insert into the database, or update 'updated_at' and 'mode' if already exists
                    db.store(msg)

                case MessageType.DEL, _:
                    pattern = Pattern(msg.topic)
                    # delete topics matching the pattern from retain cache
                    with self.msgs_lock:
                        for topic in list(self.msgs.keys()):
                            if not pattern.matches(topic):
                                continue
                            self.msgs.pop(topic, None)
                    # delete messages from the persistent database
                    for db_msg in db.get(pattern):
                        if pattern.matches(db_msg.topic):
                            if msg.value is not None:
                                db.delete_value(db_msg.topic, msg.value)
                            else:
                                db.delete_topic(db_msg.topic)

        db.commit()
        return msgs


broker = Broker()

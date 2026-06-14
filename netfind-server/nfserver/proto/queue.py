#  Copyright (c) Kuba Szczodrzyński 2026-6-13.

from collections import defaultdict

from .connection import Connection
from .model import NetfindMessage
from .util import reduce_msgs


class SendQueue:
    msgs: dict[Connection, list[NetfindMessage]]

    def __init__(self):
        self.msgs = defaultdict(list)

    def append(self, conn: Connection, msg: NetfindMessage) -> None:
        self.msgs[conn].append(msg)

    def extend(self, conn: Connection, msgs: list[NetfindMessage]) -> None:
        self.msgs[conn].extend(msgs)

    def discard(self, conn: Connection) -> None:
        self.msgs.pop(conn, None)

    def send(self) -> None:
        for conn, msgs in self.msgs.items():
            msgs = reduce_msgs(msgs)
            conn.send(msgs)

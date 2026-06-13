#  Copyright (c) Kuba Szczodrzyński 2026-6-13.

from collections import defaultdict

from simple_websocket import Server

from .msg_util import reduce_msgs
from .proto import NetfindMessage, NetfindMessageList


class SendQueue:
    msgs: dict[Server, list[NetfindMessage]]

    def __init__(self):
        self.msgs = defaultdict(list)

    def append(self, conn: Server, msg: NetfindMessage) -> None:
        self.msgs[conn].append(msg)

    def extend(self, conn: Server, msgs: list[NetfindMessage]) -> None:
        self.msgs[conn].extend(msgs)

    def discard(self, conn: Server) -> None:
        self.msgs.pop(conn, None)

    def send(self) -> None:
        for conn, msgs in self.msgs.items():
            msgs = reduce_msgs(msgs)
            message_list = NetfindMessageList(messages=list(msgs))
            data = message_list.dumps()
            conn.send(data)

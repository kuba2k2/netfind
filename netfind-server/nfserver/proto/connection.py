#  Copyright (c) Kuba Szczodrzyński 2026-6-14.

from socket import socket

from simple_websocket import Server

from .model import NetfindMessage, NetfindMessageList


class Connection:
    def __init__(self, ws: Server, sock: socket, user_agent: str):
        self.ws = ws
        self.name = f"{sock.getpeername()[0]}:{sock.getpeername()[1]}"
        if user_agent:
            self.name = f"{self.name} ({user_agent})"

    def recv(self, timeout: float = None) -> list[NetfindMessage]:
        # receive a WebSocket frame
        frame = self.ws.receive(timeout=timeout)
        if not isinstance(frame, bytes):
            return []
        # decode received frame into a list of messages
        msgs = NetfindMessageList.loads(frame).messages
        # filter out invalid messages
        return [msg for msg in msgs if msg.is_valid]

    def send(self, msgs: list[NetfindMessage]) -> None:
        message_list = NetfindMessageList(messages=list(msgs))
        data = message_list.dumps()
        self.ws.send(data)

    def __str__(self) -> str:
        return self.name

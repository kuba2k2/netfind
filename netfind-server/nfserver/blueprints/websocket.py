#  Copyright (c) Kuba Szczodrzyński 2026-6-13.

from logging import exception, info

from flask import Blueprint, g, request
from flask_sock import Sock
from simple_websocket import ConnectionClosed, Server

from nfserver.broker import broker
from nfserver.db import get_db
from nfserver.proto import Connection, MessageType, reduce_msgs

bp = Blueprint("websocket", __name__)
sock = Sock()


@sock.route("/ws", bp=bp)
def root(ws: Server):
    conn = Connection(ws, ws.sock, request.user_agent.string)
    info(f"Connection opened: {conn}")
    broker.add_connection(conn)

    db = get_db()
    try:
        while True:
            # in every loop, restart the write-ahead-log (throttled internally in 'db')
            db.wal_checkpoint()

            # receive messages with a 10-second timeout
            msgs = conn.recv(timeout=10.0)
            if not msgs:
                continue

            # reduce identical messages
            msgs = reduce_msgs(msgs)

            # fill in client name in message topics
            for msg in msgs:
                msg.topic = msg.topic.replace("/@", f"/{g.client['name']}")

            # group messages by their type
            msgs_pub_del = [
                m for m in msgs if m.type in (MessageType.PUB, MessageType.DEL)
            ]
            msgs_get_sub_unsub = [
                m
                for m in msgs
                if m.type in (MessageType.GET, MessageType.SUB, MessageType.UNSUB)
            ]
            msgs_lwt = [m for m in msgs if m.type in (MessageType.LWT,)]

            # pass all messages to the broker
            if msgs_pub_del:
                broker.accept_pub_del(conn, msgs_pub_del)
            if msgs_get_sub_unsub:
                broker.accept_get_sub_unsub(conn, msgs_get_sub_unsub)
            if msgs_lwt:
                broker.accept_lwt(conn, msgs_lwt)

    except ConnectionClosed:
        pass
    except Exception as e:
        exception("WebSocket connection failed", exc_info=e)
    finally:
        info(f"Connection closed: {conn}")
        broker.del_connection(conn)

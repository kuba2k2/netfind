#  Copyright (c) Kuba Szczodrzyński 2026-6-13.

from logging import exception, info

from flask import Blueprint, request
from flask_sock import Sock
from simple_websocket import ConnectionClosed, Server

from nfserver.broker import broker
from nfserver.proto import Connection, MessageType, reduce_msgs

bp = Blueprint("websocket", __name__)
sock = Sock()


@sock.route("/ws", bp=bp)
def root(ws: Server):
    conn = Connection(ws, ws.sock, request.user_agent.string)
    info(f"Connection opened: {conn}")
    broker.add_connection(conn)

    try:
        while True:
            msgs = conn.recv()
            if not msgs:
                continue

            # reduce identical messages
            msgs = reduce_msgs(msgs)
            # group messages by their type
            msgs_pub_del = filter(
                lambda m: m.type in (MessageType.PUB, MessageType.DEL),
                msgs,
            )
            msgs_get_sub_unsub = filter(
                lambda m: m.type
                in (MessageType.GET, MessageType.SUB, MessageType.UNSUB),
                msgs,
            )
            msgs_lwt = filter(
                lambda m: m.type in (MessageType.LWT,),
                msgs,
            )
            # pass all messages to the broker
            broker.accept_pub_del(conn, list(msgs_pub_del))
            broker.accept_get_sub_unsub(conn, list(msgs_get_sub_unsub))
            broker.accept_lwt(conn, list(msgs_lwt))

    except ConnectionClosed:
        pass
    except Exception as e:
        exception("WebSocket connection failed", exc_info=e)
    finally:
        info(f"Connection closed: {conn}")
        broker.del_connection(conn)

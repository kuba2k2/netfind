#  Copyright (c) Kuba Szczodrzyński 2026-6-13.

from logging import exception, info

from flask import Blueprint, request
from flask_sock import Sock
from simple_websocket import ConnectionClosed, Server

from nfserver.broker import broker
from nfserver.msg_util import reduce_msgs
from nfserver.proto import MessageType, NetfindMessageList

bp = Blueprint("websocket", __name__)
sock = Sock()


@sock.route("/ws", bp=bp)
def root(conn: Server):
    info(f"Connection opened: {request.remote_addr}, {request.user_agent}")
    broker.add_connection(conn)

    try:
        while True:
            frame = conn.receive()
            if not isinstance(frame, bytes):
                continue

            # decode received frame into a list of messages
            msgs = NetfindMessageList.loads(frame).messages
            # filter out invalid messages
            msgs = [msg for msg in msgs if msg.is_valid]
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
        info(f"Connection closed: {request.remote_addr}, {request.user_agent}")
        broker.del_connection(conn)

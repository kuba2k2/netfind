#  Copyright (c) Kuba Szczodrzyński 2026-6-13.

import re
from itertools import groupby
from typing import Iterator, Union

from .proto import MessageType, NetfindMessage


class Pattern(str):
    regex: str = None

    def init_regex(self) -> None:
        if self.regex:
            return
        self.regex = f"^{self}$"
        self.regex = self.regex.replace("/+/", "/[^/]+?/")
        self.regex = self.regex.replace("/#", "/.+?")

    def matches(self, other: Union[str, "Pattern"]) -> bool:
        self.init_regex()
        if isinstance(other, Pattern):
            # TODO
            return False
        return bool(re.match(self.regex, other))


def get_group_key(msg: NetfindMessage) -> tuple[MessageType, str, str | None]:
    match msg.type:
        case MessageType.PUB | MessageType.LWT:
            return msg.type, msg.topic, msg.value
        case _:  # SUB, GET, DEL, UNSUB
            return msg.type, msg.topic, None


def reduce_msgs(msgs: list[NetfindMessage]) -> list[NetfindMessage]:
    out = []
    msgs = sorted(msgs, key=get_group_key)
    for key, group in groupby(msgs, get_group_key):
        msg_type: MessageType = key[0]
        group: Iterator[NetfindMessage]
        # take the first available message
        msg = next(group)
        # for PUB messages, merge their created/updated timestamps and modes
        if msg_type == MessageType.PUB:
            for other in group:
                msg.created_at = min(msg.created_at, other.created_at)
                msg.updated_at = max(msg.updated_at, other.updated_at)
                msg.mode = max(msg.mode, other.mode)
        # append to the resulting list
        out.append(msg)
    return out

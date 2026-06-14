#  Copyright (c) Kuba Szczodrzyński 2026-6-13.

from dataclasses import dataclass, field
from datetime import datetime
from enum import IntEnum
from typing import Annotated

from pure_protobuf.annotations import Field
from pure_protobuf.message import BaseMessage


class MessageType(IntEnum):
    PUB = 0
    SUB = 1
    GET = 2
    DEL = 3
    UNSUB = 4
    LWT = 5


class PublishMode(IntEnum):
    SEND = 0
    RETAIN = 1
    PERSIST = 2
    APPEND = 3


@dataclass
class NetfindMessage(BaseMessage):
    type: Annotated[MessageType, Field(1)]
    topic: Annotated[str, Field(2)]
    value: Annotated[str | None, Field(3)] = None
    created_at: Annotated[int | None, Field(4)] = None
    updated_at: Annotated[int | None, Field(5)] = None
    mode: Annotated[PublishMode | None, Field(6)] = None

    @staticmethod
    def now() -> int:
        return int(datetime.now().timestamp() * 1000.0)

    @property
    def is_valid(self) -> bool:
        # topics must start with "/" and not end with "/"
        if not self.topic.startswith("/"):
            return False
        if self.topic.endswith("/"):
            return False

        # this is not a valid wildcard
        if "#/" in self.topic:
            return False

        if self.type in (MessageType.PUB, MessageType.LWT):
            # PUB and LWT must have a value
            if self.value is None:
                return False
            # PUB and LWT topics can't have wildcards
            if "/#" in self.topic:
                return False
            if "/+" in self.topic:
                return False
        else:
            # only DEL *may* have a value
            if self.value is not None and self.type != MessageType.DEL:
                return False
            # there can be at most 1 multi-level wildcard
            if (cnt := self.topic.count("/#")) > 1:
                return False
            # the multi-level wildcard, if present, must be the last segment
            if cnt == 1 and not self.topic.endswith("/#"):
                return False

        if self.type == MessageType.PUB:
            # PUB must have created_at, updated_at, mode
            if self.created_at is None:
                return False
            if self.updated_at is None:
                return False
            if self.mode is None:
                return False

        return True


@dataclass
class NetfindMessageList(BaseMessage):
    messages: Annotated[list[NetfindMessage], Field(1)] = field(default_factory=list)

#  Copyright (c) Kuba Szczodrzyński 2026-6-13.

import re
from typing import Union


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

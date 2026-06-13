#  Copyright (c) Kuba Szczodrzyński 2026-6-13.

from logging import error, info

from flask import Blueprint, abort, request

from nfserver.db import get_db

bp = Blueprint("auth", __name__)


@bp.before_app_request
def check_auth():
    if (
        not request.authorization
        or request.authorization.type != "bearer"
        or not request.authorization.token
    ):
        error(f"No authentication from {request.remote_addr}")
        abort(401)

    db = get_db()
    token = request.authorization.token

    client = db.db.execute(
        "SELECT * FROM client WHERE token = ?",
        (token,),
    ).fetchone()

    if client is None:
        error(f"Bad authentication from {request.remote_addr} with token '{token}'")
        abort(401)

    info(f"Authenticated {request.remote_addr} as '{client['name']}'")

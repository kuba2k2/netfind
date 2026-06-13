#  Copyright (c) Kuba Szczodrzyński 2026-6-13.

import logging
from logging import Logger
from pathlib import Path

from flask import Flask


def create_app():
    from .blueprints import auth, websocket
    from .db import app_init_db

    # create the Flask app
    app = Flask(__name__, instance_relative_config=True)
    Path(app.instance_path).mkdir(parents=True, exist_ok=True)

    Logger.root = app.logger
    logging.root = app.logger

    # load app configuration
    app.config.from_mapping(
        SECRET_KEY="dev",
        DATABASE=Path(app.instance_path) / "netfind-server.db",
    )
    app.config.from_pyfile("config.py", silent=True)

    # add database to app and register blueprints
    app_init_db(app)
    app.register_blueprint(auth.bp)
    app.register_blueprint(websocket.bp)

    return app

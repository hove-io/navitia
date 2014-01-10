#!/usr/bin/env python
# coding=utf-8
from jormungandr import app
from jormungandr import i_manager
import signal
from werkzeug.serving import run_simple
import sys
from flask_script import Manager


def kill_thread(signal, frame):
    i_manager.stop()
    sys.exit(0)

manager = Manager(app)

if __name__ == '__main__':
    signal.signal(signal.SIGINT, kill_thread)
    signal.signal(signal.SIGTERM, kill_thread)
    app.config['DEBUG'] = True
    manager.run()

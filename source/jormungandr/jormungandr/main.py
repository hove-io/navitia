#!/usr/bin/env python
# coding=utf-8
from jormungandr import app
from jormungandr.instance_manager import InstanceManager
import signal
from werkzeug.serving import run_simple
import sys

def kill_thread(signal, frame):
    InstanceManager().stop()
    sys.exit(0)


if __name__ == '__main__':
    signal.signal(signal.SIGINT, kill_thread)
    signal.signal(signal.SIGTERM, kill_thread)
    InstanceManager().initialisation()
    app.config['DEBUG'] = True
    run_simple('localhost', 5000, app, use_evalex=True)

else:
    InstanceManager().initialisation()

#!/usr/bin/env python
# coding=utf-8
import sys
import signal
import os
from conf import base_url
from instance_manager import NavitiaManager, RegionNotFound
from flask import Flask, url_for
from flask.ext.restful import Api
from interfaces.v0_routing import v0_routing
from interfaces.v1_routing import v1_routing
from interfaces.documentation import v0_documentation, v1_documentation
import os
from werkzeug.serving import run_simple

app = Flask(__name__)
app.debug = True
app.config.update(PROPAGATE_EXCEPTIONS=True)
api = Api(app)

v1_routing(api)
v0_routing(api)
v0_documentation(api)
v1_documentation(api)
@app.errorhandler(RegionNotFound)
def region_not_found(error):
    return {"error" : error.value}, 404


def kill_thread(signal, frame):
    NavitiaManager().stop()
    sys.exit(0)
config_file = 'Jormungandr.ini' if not os.environ.has_key('JORMUNGANDR_CONFIG_FILE')\
                                else os.environ['JORMUNGANDR_CONFIG_FILE']
if __name__ == '__main__':
    signal.signal(signal.SIGINT, kill_thread)
    signal.signal(signal.SIGTERM, kill_thread)
    NavitiaManager().set_config_file(config_file)
    NavitiaManager().initialisation()
    run_simple('localhost', 5000, app,
               use_debugger=True, use_evalex=True)

else:
    NavitiaManager().set_config_file(config_file)
    NavitiaManager().initialisation()

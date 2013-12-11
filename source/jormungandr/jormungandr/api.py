#!/usr/bin/env python
# coding=utf-8
import sys
import signal
import os
from jormungandr.instance_manager import InstanceManager
from flask import url_for, make_response
from flask.ext.restful.representations import json
from jormungandr.interfaces.v0_routing import v0_routing
from jormungandr.interfaces.v1_routing import v1_routing
from jormungandr.interfaces.documentation import v0_documentation, v1_documentation
import os
from werkzeug.serving import run_simple
from flask import request
import dict2xml
from jormungandr.app import app, api


#@api.representation("application/xml")
# def output_xml(data, code, headers=None):
#    """Makes a Flask response with a XML encoded body"""
#    data_xml = dict2xml.dict2xml({'response' :data})
#    resp = make_response(data_xml, code)
#    resp.headers.extend(headers or {})
#    return resp


@api.representation("text/jsonp")
@api.representation("application/jsonp")
def output_jsonp(data, code, headers=None):
    resp = json.output_json(data, code, headers)
    callback = request.args.get('callback', False)
    if callback:
        resp.data = str(callback) + '(' + resp.data + ')'
    return resp


v1_routing(api)
v0_routing(api)
v0_documentation(api)
v1_documentation(api)


def kill_thread(signal, frame):
    InstanceManager().stop()
    sys.exit(0)
config_file = 'Jormungandr.ini' if 'JORMUNGANDR_CONFIG_FILE' not in os.environ\
    else os.environ['JORMUNGANDR_CONFIG_FILE']
if __name__ == '__main__':
    signal.signal(signal.SIGINT, kill_thread)
    signal.signal(signal.SIGTERM, kill_thread)
    InstanceManager().initialisation()
    run_simple('localhost', 5000, app,
               use_evalex=True)

else:
    InstanceManager().initialisation()

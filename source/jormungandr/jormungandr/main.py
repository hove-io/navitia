#!/usr/bin/env python
# coding=utf-8
import sys
import signal
import os
from jormungandr.instance_manager import InstanceManager
from flask import url_for, make_response
from flask.ext.restful.representations import json
import jormungandr.interfaces.v0_routing as v0_routing
import jormungandr.interfaces.v1_routing as v1_routing
import jormungandr.interfaces.documentation as documentation
from jormungandr.index import index
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

index(api)
v1_routing.v1_routing(api)
v0_routing.v0_routing(api)
documentation.v0_documentation(api)
documentation.v1_documentation(api)


def kill_thread(signal, frame):
    InstanceManager().stop()
    sys.exit(0)


if __name__ == '__main__':
    signal.signal(signal.SIGINT, kill_thread)
    signal.signal(signal.SIGTERM, kill_thread)
    InstanceManager().initialisation()
    app.config['DEBUG'] = True
    run_simple('localhost', 5000, app,
               use_evalex=True)

else:
    InstanceManager().initialisation()

#!/usr/bin/env python
# coding=utf-8
from flask_restful.representations import json
from jormungandr.interfaces.v0_routing import v0_routing
from jormungandr.interfaces.v1_routing import v1_routing
from jormungandr.interfaces.documentation import v0_documentation, \
    v1_documentation
from flask import request
from jormungandr import rest_api
from jormungandr.index import index


@rest_api.representation("text/jsonp")
@rest_api.representation("application/jsonp")
def output_jsonp(data, code, headers=None):
    resp = json.output_json(data, code, headers)
    callback = request.args.get('callback', False)
    if callback:
        resp.data = str(callback) + '(' + resp.data + ')'
    return resp

index(rest_api)
v1_routing(rest_api)
v0_routing(rest_api)
v0_documentation(rest_api)
v1_documentation(rest_api)

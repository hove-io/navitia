#!/usr/bin/env python
# coding=utf-8
import sys
import signal
import os
from werkzeug.wrappers import Request, Response
from werkzeug.wsgi import responder
from werkzeug.routing import Map, Rule
from wsgiref.simple_server import make_server

from validate import *
from swagger import api_doc
from instance_manager import NavitiaManager
from find_extrem_datetimes import extremes

from apis import *
from renderers import render
from universals import *
from protobuf_to_dict import protobuf_to_dict

import request_pb2, type_pb2

def on_summary_doc(request) :
    return render(api_doc(Apis().apis_all), 'json',  request.args.get('callback'))

def on_doc(request, api):
    return render(api_doc(Apis().apis_all, api), 'json', request.args.get('callback'))

def on_index(request, version = None, region = None ):
    path = os.path.join(os.path.dirname(__file__), 'static','lost.html')
    file = open(path, 'r')
    return Response(file.read(), mimetype='text/html;charset=utf-8')


def on_regions(request, version, format):
    response = {'regions': []}
    for region in NavitiaManager().instances.keys() : 
        req = request_pb2.Request()
        req.requested_api = type_pb2.METADATAS
        try:
            resp = NavitiaManager().send_and_receive(req, region)
            resp_dict = protobuf_to_dict(resp) 
            if 'metadatas' in resp_dict.keys():
                resp_dict['metadatas']['region_id'] = region                
                response['regions'].append(resp_dict['metadatas'])
        except DeadSocketException :
            response['regions'].append({"region_id" : region, "status" : "not running"})
        except RegionNotFound:
            response['regions'].append({"region_id" : region, "status" : "not found"})

    return render(response, format,  request.args.get('callback'))

url_map = Map([
    Rule('/', endpoint=on_index),
    Rule('/<version>/', endpoint=on_index),
    Rule('/<version>/regions.<format>', endpoint = on_regions),
    Rule('/<version>/journeys.<format>', endpoint = on_universal_journeys("journeys")),
    Rule('/<version>/isochrone.<format>', endpoint = on_universal_journeys("isochrone")),
    Rule('/<version>/places_nearby.<format>', endpoint = on_universal_places_nearby),
    Rule('/<version>/<region>/', endpoint = on_index),
    Rule('/<version>/<region>/<api>.<format>', endpoint = NavitiaManager().dispatch),
    Rule('/doc.json', endpoint = on_summary_doc),
    Rule('/doc.json/<api>', endpoint = on_doc)
    ])


def kill_thread(signal, frame):
    print "Got signal !"
    NavitiaManager().stop()
    print "stoped"
    sys.exit(0)

@responder
def application(environ, start_response):
    request = Request(environ)
    urls = url_map.bind_to_environ(environ)
    return urls.dispatch(lambda fun, v: fun(request, **v),
            catch_http_exceptions=True)

signal.signal(signal.SIGINT, kill_thread)
signal.signal(signal.SIGTERM, kill_thread)

if __name__ == '__main__':
    NavitiaManager().initialisation('Jörmungandr.ini')
    v = validate_apis(Apis().apis_all)
    if not(v.valid):
        for apiname, details in v.details.iteritems():
            if len(details) > 0:
                print "Error in api : " + apiname
                for error in details : 
                    print "\t"+error
    httpd = make_server('', 8088, application)
    print "Serving on port 8088..."
    httpd.serve_forever()

else:
    NavitiaManager().initialisation()


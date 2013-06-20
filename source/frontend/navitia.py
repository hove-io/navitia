#!/usr/bin/env python
# coding=utf-8
import sys
import signal
import os
from conf import base_url
from werkzeug.wrappers import Request, Response
from werkzeug.wsgi import responder
from werkzeug.routing import Map, Rule, Submount
from wsgiref.simple_server import make_server

from validate import *
from swagger import api_doc
from instance_manager import NavitiaManager
from find_extrem_datetimes import extremes

from apis import *
from renderers import render
from universals import *
from protobuf_to_dict import protobuf_to_dict
import interfaces.input_v1
import interfaces.input_v0

import request_pb2, type_pb2


def on_summary_doc(request) :
    return render(api_doc(Apis().apis_all), 'json',  request.args.get('callback'))

def on_doc(request, api):
    return render(api_doc(Apis().apis_all, api), 'json', request.args.get('callback'))

def on_index(request):
    res = {
        'api_versions': [
        {
            'id': 'v0',
            'links':[{'href':'http://doc.navitia.io', 'rel':'doc'}],
            'title': 'Current stable API version'
        },
        {
            'id': 'v1',
            'title': 'Dev version'
        }],
        'links' : [
            {"href" : base_url + '/{api_versions.id }', "rel":"navitia.api_versions"},
            {"href" : 'http://www.navitia.io', 'rel':'about'}
            ]
            
        }
    return render(res, 'json', request.args.get('callback'))

v0_rules = [
    Rule('/', endpoint=interfaces.input_v0.on_index),
    Rule('/regions.<format>', endpoint = interfaces.input_v0.on_regions),
    Rule('/journeys.<format>', endpoint = interfaces.input_v0.on_universal_journeys("journeys")),
    Rule('/isochrone.<format>', endpoint = interfaces.input_v0.on_universal_journeys("isochrone")),
    Rule('/places_nearby.<format>', endpoint = interfaces.input_v0.on_universal_places_nearby),
    Rule('/<region>/', endpoint = interfaces.input_v0.on_index),
    Rule('/<region>/<api>.<format>', endpoint =  interfaces.input_v0.on_api),
    ]

v1_rules = [
    Rule('/', endpoint=interfaces.input_v1.index),
    Rule('/coverage', endpoint=interfaces.input_v1.coverage),
    Rule('/coverage/', endpoint=interfaces.input_v1.coverage),
    Rule('/coverage/<path:uri>', endpoint=interfaces.input_v1.uri),
    Rule('/coverage/<path:uri>/places', endpoint=interfaces.input_v1.places),
    Rule('/coverage/<path:uri1>/places_nearby', endpoint=interfaces.input_v1.nearby),
    Rule('/coverage/<path:uri1>/places_nearby/<path:uri2>', endpoint=interfaces.input_v1.nearby),
    Rule('/coverage/<path:uri1>/journeys', endpoint=interfaces.input_v1.journeys),
#Rule('/coverage/<path:uri1>/route_schedules', endpoint=interfaces.input_v1.route_schedules),
#    Rule('/coverage/<path:uri1>/stop_schedules', endpoint=interfaces.input_v1.stop_schedules),
    Rule('/coverage/<path:uri1>/departures', endpoint=interfaces.input_v1.departures),
    Rule('/coverage/<path:uri1>/arrivals', endpoint=interfaces.input_v1.arrivals),
    Rule('/journeys', endpoint=interfaces.input_v1.journeys),
    Rule('/coord/<lon>;<lat>', endpoint=interfaces.input_v1.coord)
    ]

url_map = Map([
    Rule('/', endpoint=on_index),
    Rule('/doc.json', endpoint = on_summary_doc),
    Rule('/doc.json/<api>', endpoint = on_doc),
    Submount('/v0', v0_rules),
    Submount('/v1', v1_rules)
    ]
    )


def kill_thread(signal, frame):
#print "Got signal !"
    NavitiaManager().stop()
#print "stoped"
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
#    print "Serving on port 8088..."
    httpd.serve_forever()

else:
    NavitiaManager().initialisation()


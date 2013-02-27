# coding=utf-8
import type_pb2
import json
import dict2xml
import copy
import re
import sys
import signal
from protobuf_to_dict import protobuf_to_dict
from werkzeug.wrappers import Request, Response
from werkzeug.wsgi import responder
from werkzeug.routing import Map, Rule
from wsgiref.simple_server import make_server

from validate import *
from swagger import api_doc
from instance_manager import NavitiaManager
from find_extrem_datetimes import extremes

from apis import *
from apis_functions import *
from renderers import render
from universals import *



def on_summary_doc(request) :
    return render(api_doc(Apis().apis_all), 'json',  request.args.get('callback'))

def on_doc(request, api):
    return render(api_doc(Apis().apis_all, api), 'json', request.args.get('callback'))

url_map = Map([
    Rule('/', endpoint=on_index),
    Rule('/<version>/', endpoint=on_index),
    Rule('/<version>/regions.<format>', endpoint = on_regions),
    Rule('/<version>/journeys.<format>', endpoint = on_universal_journeys("journeys")),
    Rule('/<version>/isochrone.<format>', endpoint = on_universal_journeys("isochrone")),
    Rule('/<version>/proximity_list.<format>', endpoint = on_universal_proximity_list),
    Rule('/<version>/<region>/', endpoint = on_index),
    Rule('/<version>/<region>/<api>.<format>', endpoint = Apis().on_api),
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
    NavitiaManager('Jörmungandr.ini')
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
    NavitiaManager()


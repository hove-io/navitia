# coding=utf-8
import zmq
import type_pb2
import json
import dict2xml
import time
from protobuf_to_dict import protobuf_to_dict
from werkzeug.wrappers import Request, Response
from werkzeug.wsgi import responder
from werkzeug.routing import Map, Rule

# Prepare our context and sockets
context = zmq.Context()
socket = context.socket(zmq.REQ)
socket.connect("ipc:///tmp/diediedie")

def render_output(pb_resp, format, request):
    if format == 'json':
        json_str = json.dumps(protobuf_to_dict(pb_resp, enum_as_labels=True), ensure_ascii=False)
        callback = request.args.get('callback', '')
        if callback == '':
            return Response(json_str, mimetype='application/json')
        else:
            return Response(callback + '(' + json_str + ')', mimetype='application/json')
    elif format == 'txt':
        return Response(json.dumps(protobuf_to_dict(pb_resp, enum_as_labels=True), ensure_ascii=False, indent=4),
                mimetype='text/plain')
    elif format == 'xml':
        return Response('<?xml version="1.0" encoding="UTF-8"?>\n'+dict2xml.dict2xml(protobuf_to_dict(pb_resp, enum_as_labels=True), wrap="Response"),
                mimetype='application/xml')
    elif format == 'pb':
        return Response(pb_resp.SerializeToString(), mimetype='application/octet-stream')
    else:
        return Response("Unknown file format format. Please choose .json, .txt, .xml or .pb", mimetype='text/plain', status=404)


def send_and_receive(request):
    socket.send(request.SerializeToString())
    pb = socket.recv()
    print "Taille du protobuf : ", len(pb)
    resp = type_pb2.Response()
    resp.ParseFromString(pb)
    return resp

def on_index(request, version = None ):
    return Response('Hello from the index')

def on_status(request, format):
    req = type_pb2.Request()
    req.requested_api = type_pb2.STATUS
    resp = send_and_receive(req)
    return render_output(resp, format, request)

def on_load(request, format):
    req = type_pb2.Request()
    req.requested_api = type_pb2.LOAD
    resp = send_and_receive(req)
    return render_output(resp, format, request)

pb_type = {
        'stop_area': type_pb2.STOPAREA,
        'stop_point': type_pb2.STOPPOINT,
        'city': type_pb2.CITY,
        'address': type_pb2.ADDRESS
        }

def on_first_letter(request, version, format):
    req = type_pb2.Request()
    req.requested_api = type_pb2.FIRSTLETTER
    req.first_letter.name = request.args['name']
    

    types = request.args.getlist('filter[]')
    if len(types) == 0 :
        types = ['stop_area', 'address', 'city']
    for type in types:
        if type in pb_type:
            req.first_letter.types.append(pb_type[type])
    resp = send_and_receive(req)
    print resp.firstletter.items[0].object.address.name
    return render_output(resp, format, request)



def stop_times(request, version, format, departure_filter, arrival_filter, api):
    req = type_pb2.req()
    req.requested_api = api
    req.next_departure.departure_filter = departure_filter
    req.next_departure.arrival_filter = arrival_filter

    req.next_departure.datetime = request.args.get("next_departure")
    req.next_departure.max_datetime = request.args.get("max_datetime")
    req.next_departure.nb_departures = request.args.get("nb_departures", 10, type=int)
    req.next_departure.depth = request.args.get("depth", 1, type=int)
    req.next_departure.depth = request.args.get("wheelchair", False, type=bool)
    req.line_schedule.changetime = request.args.get("changetime", "00T00")
    resp = send_and_receive(req)
    return render_output(resp, format, request)

def on_line_schedule(request, version, format):
    return stop_times(request, version, format, request.args.get("filter", ""), "", type_pb2.LINE_SCHEDULE)

def on_next_arrivals(request, version, format):
    return stop_times(request, version, format, request.args.get("filter", ""), "", type_pb2.NEXT_DEPARTURES)

def on_next_departures(request, version, format):
    return stop_times(request, version, format, "", request.args.get("filter", ""), type_pb2.NEXT_ARRIVALS)

def on_stops_schedule(request, version, format):
    return stop_times(request, version, format, "", request.args.get("departure_filter", ""), request.args.get("arrvial_filter", ""),type_pb2.STOPS_SCHEDULE)

def on_departure_board(request, version, format):
    return stop_times(request, version, format, request.args.get("filter", ""), "", type_pb2.DEPARTURE_BOARD)

def on_proximity_list(request, version, format):
    req = type_pb2.Request()
    req.requested_api = type_pb2.PROXIMITYLIST
    req.proximity_list.coord.lon = request.args.get("lon", type=float)
    req.proximity_list.coord.lat = request.args.get("lat", type=float)
    req.proximity_list.distance = request.args.get("distance", 500, type=int)
    types = request.args.getlist('filter[]')
    if len(types) == 0 :
        types = ['stop_area', 'address', 'city']
    for type in types:
        if type in pb_type:
            req.proximity_list.types.append(pb_type[type])
    resp = send_and_receive(req)
    return render_output(resp, format, request)

def journeys(requested_type, request, version, format):
#    req.params = "origin=coord:2.1301409667968625:48.802045523752106&destination=coord:2.3818232910156234:48.86202003509158&datetime=20120615T143200&format=pb" 
    req = type_pb2.Request()
    req.requested_api = requested_type

    req.journeys.origin = request.args.get("origin")
    req.journeys.destination = request.args.get("destination", "")
    req.journeys.datetime.append(request.args.get("datetime"))
    req.journeys.clockwise = request.args.get("clockwise", True, type=bool)
    #req.journeys.forbiddenline += request.args.getlist('forbiddenline[]')
    #req.journeys.forbiddenmode += request.args.getlist('forbiddenmode[]')
    #req.journeys.forbiddenroute += request.args.getlist('forbiddenroute[]')
    req.journeys.walking_speed = request.args.get('walking_speed', 1.3, type=float)
    req.journeys.walking_distance = request.args.get('walking_distance', 1000, type=int)
    req.journeys.wheelchair = request.args.get('wheelchair', False, type=float)
    resp = send_and_receive(req)
    return render_output(resp, format, request)

def on_journeys(requested_type):
    return lambda request, version, format: journeys(requested_type, request, version, format)

def ptref(requested_type, request, version, format):
    req = type_pb2.Request()
    req.requested_api = type_pb2.PTREFERENTIAL

    req.ptref.requested_type = requested_type
    req.ptref.filter = request.args.get("filter", "")
    req.ptref.depth = request.args.get("depth", 1, type=int)
    resp = send_and_receive(req)
    return render_output(resp, format, request)

def on_ptref(requested_type):
    return lambda request, version, format: ptref(requested_type, request, version, format)

url_map = Map([
    Rule('/', endpoint=on_index),
    Rule('/<version>/', endpoint=on_index),
    Rule('/load.<format>', endpoint = on_load),
    Rule('/status.<format>', endpoint = on_status),
    Rule('/<version>/first_letter.<format>', endpoint = on_first_letter),
    Rule('/<version>/line_schedule.<format>', endpoint = on_line_schedule),
    Rule('/<version>/next_departures.<format>', endpoint = on_next_departures),
    Rule('/<version>/next_arrivals.<format>', endpoint = on_next_arrivals),
    Rule('/<version>/stops_schedule.<format>', endpoint = on_stops_schedule),
    Rule('/<version>/departure_board.<format>', endpoint = on_departure_board),
    Rule('/<version>/proximity_list.<format>', endpoint = on_proximity_list),
    Rule('/<version>/journeys.<format>', endpoint = on_journeys(type_pb2.PLANNER)),
    Rule('/<version>/stop_areas.<format>', endpoint = on_ptref(type_pb2.STOPAREA)),
    Rule('/<version>/stop_points.<format>', endpoint = on_ptref(type_pb2.STOPPOINT)),
    Rule('/<version>/lines.<format>', endpoint = on_ptref(type_pb2.LINE)),
    Rule('/<version>/routes.<format>', endpoint = on_ptref(type_pb2.ROUTE)),
    Rule('/<version>/networks.<format>', endpoint = on_ptref(type_pb2.NETWORK)),
    Rule('/<version>/modes.<format>', endpoint = on_ptref(type_pb2.MODE)),
    Rule('/<version>/mode_types.<format>', endpoint = on_ptref(type_pb2.MODETYPE)),
    Rule('/<version>/connections.<format>', endpoint = on_ptref(type_pb2.CONNECTION)),
    Rule('/<version>/route_points.<format>', endpoint = on_ptref(type_pb2.ROUTEPOINT)),
    Rule('/<version>/companies.<format>', endpoint = on_ptref(type_pb2.COMPANY)),
    Rule('/<version>/isochrone.<format>', endpoint = on_journeys(type_pb2.ISOCHRONE)),
    ])


@responder
def application(environ, start_response):
    request = Request(environ)
    urls = url_map.bind_to_environ(environ)
    return urls.dispatch(lambda fun, v: fun(request, **v),
            catch_http_exceptions=True)

from gevent.pywsgi import WSGIServer
if __name__ == '__main__':
    print 'Serving on 8088...'
    WSGIServer(('', 8088), application).serve_forever()


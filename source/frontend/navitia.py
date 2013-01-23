# coding=utf-8
import type_pb2
import json
import dict2xml
import time
from protobuf_to_dict import protobuf_to_dict
from werkzeug.wrappers import Request, Response
from werkzeug.wsgi import responder
from werkzeug.routing import Map, Rule

from validate import *
from swagger import api_doc
import instance_manager

instances = instance_manager.NavitiaManager(default_zmq_socket='ipc:///tmp/diediedie')

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
    socket = instances.socket_of_key(None)
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
    for key, val in request.args.iteritems() : 
        for a in request.args.getlist(key) :
            print key+"=>"+a
    print "agrs size : %d "%len(request.args)
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


scheduleArguments = {
        "filter" : Argument("Filter to have the times you want", str, True,
                            False),
        "datetime" : Argument("The date from which you want the times",
                              datetime, True, False),
        "change_datetime" : Argument("The date where you want to cut the request",
                                  datetime, False, False),        
        "wheelchair" : Argument("true if you want the times to have accessibility", boolean, False, False, "0")
        }
stopsScheduleArguments = scheduleArguments
del stopsScheduleArguments["filter"]
stopsScheduleArguments["departure_filter"] = Argument("The filter of your departure point", str,
                                                      True, False)
stopsScheduleArguments["arrival_filter"] = Argument("The filter of your arrival point", str,
                                                      True, False)

nextTimesArguments = scheduleArguments
nextTimesArguments["nb_departures"] = Argument("The maximum number of departures", int,False, False)

ptrefArguments = {
        "filter" : Argument("Conditions to filter the returned objects", str,
                            False, False),
        "depth" : Argument("Maximum depth on objects", int, False, False, 1)
        }
journeyArguments = {
        "origin" : Argument("Departure Point", str, True, False),
        "destination" : Argument("Destination Point" , str, True, False),
        "clockwise" : Argument("1 if you want to have a journey that starts after datetime, 0 if you a journey that arrives before datetime", False, False, 1),
        "forbiddenline" : Argument("Forbidden lines identified by their external codes", False, True),
        "forbiddenmode" : Argument("Forbidden modes identified by their external codes", False, True),
        "forbiddenroute" : Argument("Forbidden routes identified by their external codes", False, True),
        "walking_speed" : Argument("Walking speed in m/s", float, False, False, 1.38),
        "walking_distance" : Argument("Maximum walking distance in meters", int,
                                      False, False, "1000"),
        "wheelchair" : Argument("Does the journey has to be accessible ?",
                                boolean, False, False, "false")
        }

apis = {
        "first_letter" : {"endpoint" : on_first_letter, "arguments" : {"name" : Argument("The data to search", str, True, False ),
                                                                       "filter" : Argument("The type of datas you want in return", str, False, False)}},
        "next_departures" : {"endpoint" : on_next_departures, "arguments" :
                             nextTimesArguments},
        "next_arrivals" : {"endpoint" : on_next_arrivals, "arguments" :
                            nextTimesArguments},
        "line_schedule" : {"endpoint" : on_line_schedule, "arguments" :
                           scheduleArguments},
        "stops_schedule" : {"endpoint" : on_stops_schedule, "arguments" :
                            stopsScheduleArguments},
        "departure_board" : {"endpoint" : on_departure_board,
                             "arguments":scheduleArguments},
        "stop_areas" : {"endpoint" : on_ptref(type_pb2.STOPAREA), "arguments" :
                        ptrefArguments},
        "stop_points" : {"endpoint" : on_ptref(type_pb2.STOPPOINT), "arguments" :
                        ptrefArguments},
        "lines" : {"endpoint" : on_ptref(type_pb2.LINE), "arguments" :
                        ptrefArguments},
        "routes" : {"endpoint" : on_ptref(type_pb2.ROUTE), "arguments" :
                        ptrefArguments},
        "networks" : {"endpoint" : on_ptref(type_pb2.NETWORK), "arguments" :
                        ptrefArguments},
        "modes" : {"endpoint" : on_ptref(type_pb2.MODE), "arguments" :
                        ptrefArguments},
        "mode_types" : {"endpoint" : on_ptref(type_pb2.MODETYPE), "arguments" :
                        ptrefArguments},
        "connections" : {"endpoint" : on_ptref(type_pb2.CONNECTION), "arguments" :
                        ptrefArguments},
        "route_points" : {"endpoint" : on_ptref(type_pb2.ROUTEPOINT), "arguments" :
                        ptrefArguments},
        "companies" : {"endpoint" : on_ptref(type_pb2.COMPANY), "arguments" :
                        ptrefArguments},
        "journeys" : {"endpoint" :  on_journeys(type_pb2.PLANNER), "arguments" :
                      journeyArguments},
        "isochrone" : {"endpoint" : on_journeys(type_pb2.ISOCHRONE), "arguments" : journeyArguments}
        
        }

def on_api(request, version, api, format):
    if(api in apis):
         v = validate_arguments(request, apis[api]["arguments"])
         if(v.valid):
            return apis[api]["endpoint"](request, version, format)
         else:
             print "requete non valide"
             print v.details
    else:
        print api +" non trouve ! "


def on_summary_doc(request) : 
    return api_doc(apis)

def on_doc(request, api):
    return api_doc(apis, api)

url_map = Map([
    Rule('/', endpoint=on_index),
    Rule('/<version>/', endpoint=on_index),
    Rule('/load.<format>', endpoint = on_load),
    Rule('/status.<format>', endpoint = on_status),
    Rule('/<version>/<api>.<format>', endpoint = on_api),
    Rule('/doc.json', endpoint = on_summary_doc),
    Rule('/doc.json/<api>', endpoint = on_doc)
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


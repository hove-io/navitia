# coding=utf-8
import type_pb2
import json
import dict2xml
import time
from protobuf_to_dict import protobuf_to_dict
from werkzeug.wrappers import Request, Response
from werkzeug.wsgi import responder
from werkzeug.routing import Map, Rule
from wsgiref.simple_server import make_server

from validate import *
from swagger import api_doc
import instance_manager

instances = instance_manager.NavitiaManager('Jörmungandr.ini')

def render(dico, format, request):
    if format == 'json':
        json_str = json.dumps(dico, ensure_ascii=False)
        callback = request.args.get('callback', '')
        if callback == '':
            result = Response(json_str, mimetype='application/json')
        else:
            result = Response(callback + '(' + json_str + ')', mimetype='application/json')
    elif format == 'txt':
        result = Response(json.dumps(dico, ensure_ascii=False, indent=4), mimetype='text/plain')
    elif format == 'xml':
        result = Response('<?xml version="1.0" encoding="UTF-8"?>\n'+ dict2xml.dict2xml(dico, wrap="Response"), mimetype='application/xml')
    elif format == 'pb':
        result = Response('Protocol buffer not supported for this request', status=404)
    else:
        result = Response("Unknown file format format. Please choose .json, .txt, .xml or .pb", mimetype='text/plain', status=404)
    result.headers.add('Access-Control-Allow-Origin', '*')
    return result


def render_from_protobuf(pb_resp, format, request):
    if format == 'pb':
        return Response(pb_resp.SerializeToString(), mimetype='application/octet-stream')
    else:
        render(protobuf_to_dict(pb_resp, enum_as_labels=True), format, request)


def send_and_receive(request, region = None):
    socket = instances.socket_of_key(region)
    socket.send(request.SerializeToString())
    pb = socket.recv()
    resp = type_pb2.Response()
    resp.ParseFromString(pb)
    return resp

def on_index(request, version = None ):
    return Response('Hello from the index')

def on_regions(request, version, format):
    return render(instances.regions(), format, request)


def on_status(request, region, format):
    req = type_pb2.Request()
    req.requested_api = type_pb2.STATUS
    resp = send_and_receive(req, region)
    return render_from_protobuf(resp, format, request)

def on_load(request, region, format):
    req = type_pb2.Request()
    req.requested_api = type_pb2.LOAD
    resp = send_and_receive(req, region)
    return render_from_protobuf(resp, format, request)

pb_type = {
        'stop_area': type_pb2.STOPAREA,
        'stop_point': type_pb2.STOPPOINT,
        'city': type_pb2.CITY,
        'address': type_pb2.ADDRESS
        }

def on_first_letter(request, version, region, format):
    req = type_pb2.Request()
    req.requested_api = type_pb2.FIRSTLETTER
    req.first_letter.name = request.args['name']
    

    types = request.args.getlist('filter[]')
    if len(types) == 0 :
        types = ['stop_area', 'address', 'city']
    for type in types:
        if type in pb_type:
            req.first_letter.types.append(pb_type[type])
    resp = send_and_receive(req, region)
    return render_from_protobuf(resp, format, request)


def stop_times(request, version, region, format, departure_filter, arrival_filter, api):
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
    resp = send_and_receive(req, region)
    return render_from_protobuf(resp, format, request)

def on_line_schedule(request, version, region, format):
    return stop_times(request, version, region, format, request.args.get("filter", ""), "", type_pb2.LINE_SCHEDULE)

def on_next_arrivals(request, version, region, format):
    return stop_times(request, version, region, format, request.args.get("filter", ""), "", type_pb2.NEXT_DEPARTURES)

def on_next_departures(request, version, region, format):
    return stop_times(request, version, region, format, "", request.args.get("filter", ""), type_pb2.NEXT_ARRIVALS)

def on_stops_schedule(request, version, region, format):
    return stop_times(request, version, region, format, "", request.args.get("departure_filter", ""), request.args.get("arrvial_filter", ""),type_pb2.STOPS_SCHEDULE)

def on_departure_board(request, version, region, format):
    return stop_times(request, version, region, format, request.args.get("filter", ""), "", type_pb2.DEPARTURE_BOARD)

def on_proximity_list(request, version, region, format):
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
    resp = send_and_receive(req, region)
    return render_from_protobuf(resp, format, request)

def journeys(requested_type, request, version, region, format):
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
    resp = send_and_receive(req, region)
    return render_from_protobuf(resp, format, request)

def on_journeys(requested_type):
    return lambda request, version, region, format: journeys(requested_type, request, version, region, format)

def ptref(requested_type, request, version, region, format):
    req = type_pb2.Request()
    req.requested_api = type_pb2.PTREFERENTIAL

    req.ptref.requested_type = requested_type
    req.ptref.filter = request.args.get("filter", "")
    req.ptref.depth = request.args.get("depth", 1, type=int)
    resp = send_and_receive(req, region)
    return render_from_protobuf(resp, format, request)

def on_ptref(requested_type):
    return lambda request, version, region, format: ptref(requested_type, request, version, region, format)


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
                                                                       "filter" : Argument("The type of datas you want in return", str, False, False)},
                          "description" : "Retrieves the objects which contains in their name the \"name\""},
        "next_departures" : {"endpoint" : on_next_departures, "arguments" :
                             nextTimesArguments, 
                             "description" : "Retrieves the departures after datetime at the stop points filtered with filter"},
        "next_arrivals" : {"endpoint" : on_next_arrivals, "arguments" :
                            nextTimesArguments,
                           "description" : "Retrieves the departures after datetime at the stop points filtered with filter"},
        "line_schedule" : {"endpoint" : on_line_schedule, "arguments" :
                           scheduleArguments,
                           "description" : "Retrieves the schedule of line at the day datetime"},
        "stops_schedule" : {"endpoint" : on_stops_schedule, "arguments" :
                            stopsScheduleArguments,
                            "description" : "Retrieves the schedule for 2 stops points"},
        "departure_board" : {"endpoint" : on_departure_board,
                             "arguments":scheduleArguments,
                             "description" : "Give all the departures of filter at datetime"},
        "stop_areas" : {"endpoint" : on_ptref(type_pb2.STOPAREA), "arguments" :
                        ptrefArguments,
                        "description" : "Retrieves all the stop areas filtered with filter"},
        "stop_points" : {"endpoint" : on_ptref(type_pb2.STOPPOINT), "arguments" :
                        ptrefArguments,
                        "description" : "Retrieves all the stop points filtered with filter"},
        "lines" : {"endpoint" : on_ptref(type_pb2.LINE), "arguments" :
                        ptrefArguments,
                        "description" : "Retrieves all the stop_areas filtered with filter"},
        "routes" : {"endpoint" : on_ptref(type_pb2.ROUTE), "arguments" :
                        ptrefArguments},
        "networks" : {"endpoint" : on_ptref(type_pb2.NETWORK), "arguments" :
                        ptrefArguments,
                        "description" : "Retrieves all the networks filtered with filter"},
        "modes" : {"endpoint" : on_ptref(type_pb2.MODE), "arguments" :
                        ptrefArguments,
                        "description" : "Retrieves all the modes filtered with filter"},
        "mode_types" : {"endpoint" : on_ptref(type_pb2.MODETYPE), "arguments" :
                        ptrefArguments,
                        "description" : "Retrieves all the mode types filtered with filter"},
        "connections" : {"endpoint" : on_ptref(type_pb2.CONNECTION), "arguments" :
                        ptrefArguments,
                        "description" : "Retrieves all the connections points filtered with filter"},
        "route_points" : {"endpoint" : on_ptref(type_pb2.ROUTEPOINT), "arguments" :
                        ptrefArguments,
                        "description" : "Retrieves all the route points filtered with filter"},
        "companies" : {"endpoint" : on_ptref(type_pb2.COMPANY), "arguments" :
                        ptrefArguments,
                        "description" : "Retrieves all the companies filtered with filter"},
        "journeys" : {"endpoint" :  on_journeys(type_pb2.PLANNER), "arguments" :
                      journeyArguments,
                      "description" : "Computes and retrieves a journey"},
        "isochrone" : {"endpoint" : on_journeys(type_pb2.ISOCHRONE), "arguments" : journeyArguments,
                       "description" : "Computes and retrieves an isochrone"}
        
        }

def on_api(request, version, region, api, format):
    if version != "v0":
        return Response("Unknown version: " + version, status=404)
    if api in apis:
         v = validate_arguments(request, apis[api]["arguments"])
         if v.valid:
            return apis[api]["endpoint"](request, version, region, format)
         else:
             return Response("Invalid arguments: " + v.details, status=400)
    else:
        return Response("Unknown api: " + api, status=404)


def on_summary_doc(request) : 
    return render(api_doc(apis, instances), 'json', request)

def on_doc(request, api):
    return render(api_doc(apis, instances, api), 'json', request)

url_map = Map([
    Rule('/', endpoint=on_index),
    Rule('/<version>/', endpoint=on_index),
    Rule('/<version>/regions.<format>', endpoint = on_regions),
    Rule('/<region>/load.<format>', endpoint = on_load),
    Rule('/<region>/status.<format>', endpoint = on_status),
    Rule('/<version>/<region>/<api>.<format>', endpoint = on_api),
    Rule('/doc.json', endpoint = on_summary_doc),
    Rule('/doc.json/<api>', endpoint = on_doc)
    ])



@responder
def application(environ, start_response):
    request = Request(environ)
    urls = url_map.bind_to_environ(environ)
    return urls.dispatch(lambda fun, v: fun(request, **v),
            catch_http_exceptions=True)

if __name__ == '__main__':
    httpd = make_server('', 8088, application)
    print "Serving on port 8088..."
    httpd.serve_forever()


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

from instance_manager import NavitiaManager
from renderers import render, render_from_protobuf


def on_index(request, version = None, region = None ):
    return Response('Welcome to the navitia API. Have a look at http://www.navitia.io to learn how to use it.')


def on_regions(request, version, format):
    response = {'requested_api': 'REGIONS', 'regions': []}
    for region in NavitiaManager().instances.keys() : 
        req = type_pb2.Request()
        req.requested_api = type_pb2.METADATAS
        try:
            resp = NavitiaManager().send_and_receive(req, region)
            resp_dict = protobuf_to_dict(resp) 
            if 'metadatas' in resp_dict.keys():
                response['regions'].append(resp_dict['metadatas'])
        except DeadSocketException :
            response['regions'].append({"region_id" : region, "status" : "not running"})

    return render(response, format,  request.args.get('callback'))


def on_status(request_args, request, region):
    req = type_pb2.Request()
    req.requested_api = type_pb2.STATUS
    resp = NavitiaManager().send_and_receive(req, region)
    return resp

def on_metadatas(request_args, request, region):
    req = type_pb2.Request()
    req.requested_api = type_pb2.METADATAS
    resp = NavitiaManager().send_and_receive(req, region)
    return resp



def on_load(request, region, format):
    req = type_pb2.Request()
    req.requested_api = type_pb2.LOAD
    resp = NavitiaManager().send_and_receive(req, region)
    return render_from_protobuf(resp, format, request.args.get('callback'))


pb_type = {
        'stop_area': type_pb2.STOP_AREA,
        'stop_point': type_pb2.STOP_POINT,
        'city': type_pb2.CITY,
        'address': type_pb2.ADDRESS
        }

def on_autocomplete(request_args, version, region):
    req = type_pb2.Request()
    req.requested_api = type_pb2.AUTOCOMPLETE
    req.autocomplete.name = request_args['name']
    for object_type in request_args["object_type[]"]:
        req.autocomplete.types.append(pb_type[object_type])

    resp = NavitiaManager().send_and_receive(req, region)
    return resp


def stop_times(request_args, version, region, departure_filter, arrival_filter, api):
    req = type_pb2.Request()
    req.requested_api = api
    req.next_stop_times.departure_filter = departure_filter
    req.next_stop_times.arrival_filter = arrival_filter

    req.next_stop_times.from_datetime = request_args["from_datetime"]
    req.next_stop_times.duration = request_args["duration"]
    req.next_stop_times.depth = request_args["depth"]
    req.next_stop_times.nb_stoptimes = request_args["nb_stoptimes"] if "nb_stoptimes" in request_args else 0
    req.next_stop_times.wheelchair = request_args["wheelchair"]
    resp = NavitiaManager().send_and_receive(req, region)
    return resp


def on_line_schedule(request_args, version, region):
    return stop_times(request_args, version, region, request_args["filter"], "", type_pb2.LINE_SCHEDULE)

def on_next_arrivals(request_args, version, region):
    return stop_times(request_args, version, region, request_args["filter"], "", type_pb2.NEXT_DEPARTURES)

def on_next_departures(request_args, version, region):
    return stop_times(request_args, version, region, "", request_args["filter"], type_pb2.NEXT_ARRIVALS)

def on_stops_schedule(request_args, version, region):
    return stop_times(request_args, version, region, request_args["departure_filter"], request_args["arrival_filter"],type_pb2.STOPS_SCHEDULE)

def on_departure_board(request_args, version, region):
    return stop_times(request_args, version, region, request_args["filter"], "", type_pb2.DEPARTURE_BOARD)

def on_proximity_list(request_args, version, region):
    req = type_pb2.Request()
    req.requested_api = type_pb2.PROXIMITY_LIST
    req.proximity_list.coord.lon = request_args["lon"]
    req.proximity_list.coord.lat = request_args["lat"]
    req.proximity_list.distance = request_args["distance"]
    for object_type in request_args["object_type[]"]:
        req.proximity_list.types.append(pb_type[object_type])
    resp = NavitiaManager().send_and_receive(req, region)
    return resp


def journeys(requested_type, request_args, version, region):
    req = type_pb2.Request()
    req.requested_api = requested_type

    req.journeys.origin = request_args["origin"]
    req.journeys.destination = request_args["destination"] if "destination" in request_args else ""
    req.journeys.datetimes.append(request_args["datetime"])
    req.journeys.clockwise = request_args["clockwise"]
    #req.journeys.forbiddenline += request.args.getlist('forbiddenline[]')
    #req.journeys.forbiddenmode += request.args.getlist('forbiddenmode[]')
    #req.journeys.forbiddenroute += request.args.getlist('forbiddenroute[]')
    req.journeys.walking_speed = request_args["walking_speed"]
    req.journeys.walking_distance = request_args["walking_distance"]
    req.journeys.wheelchair = request_args["wheelchair"]
    resp = NavitiaManager().send_and_receive(req, region)
    return resp


    if before and after:
        resp.planner.before, resp.planner.after

    return resp


def on_journeys(requested_type):
    return lambda request, version, region: journeys(requested_type, request, version, region)



def ptref(requested_type, request_args, version, region):
    req = type_pb2.Request()
    req.requested_api = type_pb2.PTREFERENTIAL

    req.ptref.requested_type = requested_type
    req.ptref.filter = request_args["filter"]
    req.ptref.depth = request_args["depth"]
    resp = NavitiaManager().send_and_receive(req, region)
    return resp


def on_ptref(requested_type):
    return lambda request_args, version, region: ptref(requested_type, request_args, version, region)





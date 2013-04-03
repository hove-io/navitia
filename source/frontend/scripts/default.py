# coding=utf-8
import type_pb2
import request_pb2
import response_pb2
from protobuf_to_dict import protobuf_to_dict

from instance_manager import NavitiaManager, DeadSocketException, RegionNotFound
from renderers import render, render_from_protobuf
from werkzeug.wrappers import Response
from find_extrem_datetimes import *

pb_type = {
        'stop_area': type_pb2.STOP_AREA,
        'stop_point': type_pb2.STOP_POINT,
        'city': type_pb2.CITY,
        'address': type_pb2.ADDRESS,
        'poi': type_pb2.POI ,
        'admin' : type_pb2.ADMIN
        }

class Script:
    def pagination(self, request_pagination, objects, request):
        begin = int(request_pagination.startPage) * int(request_pagination.itemsPerPage)
        end = begin + int(request_pagination.itemsPerPage) 
        if end > request_pagination.totalResult:
            end = request_pagination.totalResult

        toDelete = [] 
        if begin < request_pagination.totalResult :
            del objects[end:]# todo -1 valable ?
            del objects[0:begin]
        else:
            del objects[0:]
        
        request_pagination.itemsOnPage = len(objects)
        query_args = ""
        for key, value in request.iteritems():
            if key != "request_pagination.startPage":
                if type(value) == type([]):
                    for v in value:
                        query_args += key + "=" +unicode(v) + "&"
                else:
                    query_args += key + "=" +unicode(value) + "&"
        if request_pagination.startPage > 0:
            request_pagination.previousPage = query_args+"startPage=%i"%(request_pagination.startPage-1)

        if end<request_pagination.totalResult:
            request_pagination.nextPage = query_args+"startPage=%i"%(request_pagination.startPage+1)


    


    def status(self, request_args, request, region):
        req = request_pb2.Request()
        req.requested_api = type_pb2.STATUS
        resp = NavitiaManager().send_and_receive(req, region)
        return resp

    def metadatas(request_args, request, region):
        req = request_pb2.Request()
        req.requested_api = type_pb2.METADATAS
        resp = NavitiaManager().send_and_receive(req, region)
        return resp



    def load(self, request, region, format):
        req = request_pb2.Request()
        req.requested_api = type_pb2.LOAD
        resp = NavitiaManager().send_and_receive(req, region)
        return render_from_protobuf(resp, format, request.args.get('callback'))


    def autocomplete(self, request_args, version, region):
        req = request_pb2.Request()
        req.requested_api = type_pb2.AUTOCOMPLETE
        req.autocomplete.name = request_args['name']
        req.autocomplete.depth = request_args['depth']
        req.autocomplete.nbmax = request_args['nbmax']
        for object_type in request_args["object_type[]"]:
            req.autocomplete.types.append(pb_type[object_type])

        resp = NavitiaManager().send_and_receive(req, region)
        pagination_resp = response_pb2.Pagination()
        pagination_resp.startPage = request_args["startPage"]
        pagination_resp.itemsPerPage = request_args["count"]
        if resp.autocomplete.items:
            objects = resp.autocomplete.items
            pagination_resp.totalResult = len(objects)
            self.pagination(pagination_resp, objects, request_args)
        else:
            pagination_resp.totalResult = 0
        resp.pagination.CopyFrom(pagination_resp)
        return resp


    def stop_times(self, request_args, version, region, departure_filter, arrival_filter, api):
        req = request_pb2.Request()
        req.requested_api = api
        req.next_stop_times.departure_filter = departure_filter
        req.next_stop_times.arrival_filter = arrival_filter
        req.next_stop_times.from_datetime = request_args["from_datetime"]
        req.next_stop_times.duration = request_args["duration"]
        req.next_stop_times.depth = request_args["depth"]
        req.next_stop_times.nb_stoptimes = request_args["nb_stoptimes"] if "nb_stoptimes" in request_args else 0
        resp = NavitiaManager().send_and_receive(req, region)
        return resp


    def route_schedule(self, request_args, version, region):
        return stop_times(request_args, version, region, request_args["filter"], "", type_pb2.ROUTE_SCHEDULES)

    def next_arrivals(self, request_args, version, region):
        return stop_times(request_args, version, region, "", request_args["filter"], type_pb2.NEXT_ARRIVALS)

    def next_departures(self, request_args, version, region):
        return stop_times(request_args, version, region, request_args["filter"], "", type_pb2.NEXT_DEPARTURES)

    def stops_schedule(self, request_args, version, region):
        return stop_times(request_args, version, region, request_args["departure_filter"], request_args["arrival_filter"],type_pb2.STOPS_SCHEDULES)

    def departure_board(self, request_args, version, region):
        return stop_times(request_args, version, region, request_args["filter"], "", type_pb2.DEPARTURE_BOARDS)

    
    def proximity_list(self, request_args, version, region):
        req = request_pb2.Request()
        req.requested_api = type_pb2.PROXIMITY_LIST
        req.proximity_list.uri = request_args["uri"]
        req.proximity_list.distance = request_args["distance"]
        req.proximity_list.depth = request_args["depth"]
        for object_type in request_args["object_type[]"]:
            req.proximity_list.types.append(pb_type[object_type])
        resp = NavitiaManager().send_and_receive(req, region)
        pagination_resp = response_pb2.Pagination()
        pagination_resp.startPage = request_args["startPage"]
        pagination_resp.itemsPerPage = request_args["count"]
        if resp.proximitylist.items:
            objects = resp.proximitylist.items
            pagination_resp.totalResult = len(objects)
            self.pagination(pagination_resp, objects, request_args)
        else:
            pagination_resp.totalResult = 0
        return resp


    def on_journeys(self, requested_type, request_args, version, region):
        req = request_pb2.Request()
        req.requested_api = requested_type

        req.journeys.origin = request_args["origin"]
        req.journeys.destination = request_args["destination"] if "destination" in request_args else ""
        req.journeys.datetimes.append(request_args["datetime"])
        req.journeys.clockwise = request_args["clockwise"]
        req.journeys.walking_speed = request_args["walking_speed"]
        req.journeys.walking_distance = request_args["walking_distance"]
        req.journeys.max_duration = request_args["max_duration"]
        for forbidden_uri in request_args["forbidden_uris[]"]:
            req.journeys.forbidden_uris.append(forbidden_uri)
        resp = NavitiaManager().send_and_receive(req, region)

        if resp.planner.journeys:
            (before, after) = extremes(resp, request_args)
            if before and after:
                resp.planner.before = before
                resp.planner.after = after

        return resp


    def journeys(self, request_args, version, region):
        return self.on_journeys(type_pb2.PLANNER, request_args, version, region)

    
    def isochrone(self, request_args, version, region):
        return self.on_journeys(type_pb2.ISOCHRONE, request_args, version, region)

    def on_ptref(self, requested_type, request_args, version, region):
        req = request_pb2.Request()
        req.requested_api = type_pb2.PTREFERENTIAL

        req.ptref.requested_type = requested_type
        req.ptref.filter = request_args["filter"]
        req.ptref.depth = request_args["depth"]
        resp = NavitiaManager().send_and_receive(req, region)
        pagination_resp = response_pb2.Pagination()
        pagination_resp.startPage = request_args["startPage"]
        pagination_resp.itemsPerPage = request_args["count"]
        if resp.ptref.ListFields():
            objects = resp.ptref.ListFields()[0][1]
            pagination_resp.totalResult = len(objects)
            self.pagination(pagination_resp, objects, request_args)
        else:
            pagination_resp.totalResult = 0
        resp.pagination.CopyFrom(pagination_resp)
        
        return resp

    def stop_areas(self, request_args, version, region):
        return self.on_ptref(type_pb2.STOP_AREA, request_args, version, region)

    def stop_points(self, request_args, version, region):
        return self.on_ptref(type_pb2.STOP_POINT, request_args, version, region)

    def lines(self, request_args, version, region):
        return self.on_ptref(type_pb2.LINE, request_args, version, region)

    def routes(self, request_args, version, region):
        return self.on_ptref(type_pb2.ROUTE, request_args, version, region)

    def networks(self, request_args, version, region):
        return self.on_ptref(type_pb2.NETWORK, request_args, version, region)

    def physical_modes(self, request_args, version, region):
        return self.on_ptref(type_pb2.PHYSICAL_MODE, request_args, version, region)

    def commercial_modes(self, request_args, version, region):
        return self.on_ptref(type_pb2.COMMERCIAL_MODE, request_args, version, region)

    def connections(self, request_args, version, region):
        return self.on_ptref(self, type_pb2.CONNECTION, request_args, version, region)

    def journey_pattern_points(request_args, version, region):
        return self.on_ptref(self, type_pb2.JOURNEY_PATTERN_POINT, request_args, version, region)

    def journey_patterns(request_args, version, region):
        return self.on_ptref(self, type_pb2.JOURNEY_PATTERNS, request_args, version, region)

    def companies(self, request_args, version, region):
        return self.on_ptref(type_pb2.COMPANY, request_args, version, region)

    def vehicle_journeys(self, request_args, version, region):
        return self.on_ptref(type_pb2.VEHICLE_JOURNEY, request_args, version, region)

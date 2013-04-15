# coding=utf-8
import type_pb2
import request_pb2
import response_pb2
from protobuf_to_dict import protobuf_to_dict
from apis import Apis, validation_decorator

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
        'admin' : type_pb2.ADMIN,
        'line' : type_pb2.LINE
        }

class Script:
    def __init__(self):
        self.apis = Apis().apis


    def __pagination(self, request_pagination, objects, request):
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

    def metadatas(self, request_args, request, region):
        req = request_pb2.Request()
        req.requested_api = type_pb2.METADATAS
        resp = NavitiaManager().send_and_receive(req, region)
        return resp



    def load(self, request, region, format):
        req = request_pb2.Request()
        req.requested_api = type_pb2.LOAD
        resp = NavitiaManager().send_and_receive(req, region)
        return render_from_protobuf(resp, format, request.args.get('callback'))


    @validation_decorator
    def autocomplete(self, request_args, version, region):
        req = request_pb2.Request()
        req.requested_api = type_pb2.AUTOCOMPLETE
        req.autocomplete.q = self.v.arguments['q']
        req.autocomplete.depth = self.v.arguments['depth']
        req.autocomplete.nbmax = self.v.arguments['nbmax']
        for object_type in self.v.arguments["object_type[]"]:
            req.autocomplete.types.append(pb_type[object_type])

        resp = NavitiaManager().send_and_receive(req, region)
        pagination_resp = response_pb2.Pagination()
        pagination_resp.startPage = self.v.arguments["startPage"]
        pagination_resp.itemsPerPage = self.v.arguments["count"]
        if resp.autocomplete.items:
            objects = resp.autocomplete.items
            pagination_resp.totalResult = len(objects)
            self.__pagination(pagination_resp, objects, self.v.arguments)
        else:
            pagination_resp.totalResult = 0
        resp.pagination.CopyFrom(pagination_resp)
        return resp


    def __stop_times(self, request_args, version, region, departure_filter, arrival_filter, api):
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


    @validation_decorator
    def route_schedules(self, request_args, version, region):
        return self.__stop_times(self.v.arguments, version, region, self.v.arguments["filter"], "", type_pb2.ROUTE_SCHEDULES)

    @validation_decorator
    def next_arrivals(self, request_args, version, region):
        return self.__stop_times(self.v.arguments, version, region, "", self.v.arguments["filter"], type_pb2.NEXT_ARRIVALS)

    @validation_decorator
    def next_departures(self, request_args, version, region):
        return self.__stop_times(self.v.arguments, version, region, self.v.arguments["filter"], "", type_pb2.NEXT_DEPARTURES)

    @validation_decorator
    def stops_schedules(self, request_args, version, region):
        return self.__stop_times(self.v.arguments, version, region, self.v.arguments["departure_filter"], self.v.arguments["arrival_filter"],type_pb2.STOPS_SCHEDULES)

    @validation_decorator
    def departure_boards(self, request_args, version, region):
        return self.__stop_times(self.v.arguments, version, region, self.v.arguments["filter"], "", type_pb2.DEPARTURE_BOARDS)

    
    @validation_decorator
    def proximity_list(self, request_args, version, region):
        req = request_pb2.Request()
        req.requested_api = type_pb2.PROXIMITY_LIST
        req.proximity_list.uri = self.v.arguments["uri"]
        req.proximity_list.distance = self.v.arguments["distance"]
        req.proximity_list.depth = self.v.arguments["depth"]
        for object_type in self.v.arguments["object_type[]"]:
            req.proximity_list.types.append(pb_type[object_type])
        resp = NavitiaManager().send_and_receive(req, region)
        pagination_resp = response_pb2.Pagination()
        pagination_resp.startPage = self.v.arguments["startPage"]
        pagination_resp.itemsPerPage = self.v.arguments["count"]
        if resp.proximitylist.items:
            objects = resp.proximitylist.items
            pagination_resp.totalResult = len(objects)
            self.__pagination(pagination_resp, objects, self.v.arguments)
        else:
            pagination_resp.totalResult = 0
        return resp


    def __fill_display_and_uris(self, resp):
        for journey in resp.planner.journeys:
            for section in journey.sections:
                if section.type == response_pb2.PUBLIC_TRANSPORT:
                    section.pt_display_informations.physical_mode = section.vehicle_journey.physical_mode.name
                    section.pt_display_informations.commercial_mode = section.vehicle_journey.route.line.commercial_mode.name
                    section.pt_display_informations.network = section.vehicle_journey.route.line.network.name
                    section.pt_display_informations.code = section.vehicle_journey.route.line.code
                    section.pt_display_informations.headsign = section.vehicle_journey.route.name
                    if section.destination.type == type_pb2.STOP_POINT:
                        section.pt_display_informations.direction = section.destination.stop_point.name
                    section.pt_display_informations.color = section.vehicle_journey.route.line.color
                    section.uris.vehicle_journey = section.vehicle_journey.uri
                    section.uris.line = section.vehicle_journey.route.line.uri
                    section.uris.route = section.vehicle_journey.route.uri
                    section.uris.commercial_mode = section.vehicle_journey.route.line.commercial_mode.uri
                    section.uris.physical_mode = section.vehicle_journey.physical_mode.uri
                    section.uris.network = section.vehicle_journey.route.line.network.uri
                    section.vehicle_journey.Clear()




    def __on_journeys(self, requested_type, request_args, version, region):
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

        self.__fill_display_and_uris(resp)
        
        

        return resp


    @validation_decorator
    def journeys(self, request_args, version, region):
        return self.__on_journeys(type_pb2.PLANNER, self.v.arguments, version, region)

    
    @validation_decorator
    def isochrone(self, request_args, version, region):
        return self.__on_journeys(type_pb2.ISOCHRONE, self.v.arguments, version, region)

    def __on_ptref(self, requested_type, request_args, version, region):
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
            self.__pagination(pagination_resp, objects, request_args)
        else:
            pagination_resp.totalResult = 0
        resp.pagination.CopyFrom(pagination_resp)
        
        return resp

    @validation_decorator
    def stop_areas(self, request_args, version, region):
        return self.__on_ptref(type_pb2.STOP_AREA, self.v.arguments, version, region)

    @validation_decorator
    def stop_points(self, request_args, version, region):
        return self.__on_ptref(type_pb2.STOP_POINT, self.v.arguments, version, region)

    @validation_decorator
    def lines(self, request_args, version, region):
        return self.__on_ptref(type_pb2.LINE, self.v.arguments, version, region)

    @validation_decorator
    def routes(self, request_args, version, region):
        return self.__on_ptref(type_pb2.ROUTE, self.v.arguments,  version, region)

    @validation_decorator
    def networks(self, request_args, version, region):
        return self.__on_ptref(type_pb2.NETWORK, self.v.arguments, version, region)

    @validation_decorator
    def physical_modes(self, request_args, version, region):
        return self.__on_ptref(type_pb2.PHYSICAL_MODE, self.v.arguments, version, region)

    @validation_decorator
    def commercial_modes(self, request_args, version, region):
        return self.__on_ptref(type_pb2.COMMERCIAL_MODE, self.v.arguments, version, region)

    @validation_decorator
    def connections(self, request_args, version, region):
        return self.__on_ptref(type_pb2.CONNECTION, self.v.arguments, version, region)

    @validation_decorator
    def journey_pattern_points(self, request_args, version, region):
        return self.__on_ptref(type_pb2.JOURNEY_PATTERN_POINT, self.v.arguments,  version, region)

    @validation_decorator
    def journey_patterns(self, request_args, version, region):
        return self.__on_ptref(type_pb2.JOURNEY_PATTERN, self.v.arguments, version, region)

    @validation_decorator
    def companies(self, request_args, version, region):
        return self.__on_ptref(type_pb2.COMPANY, self.v.arguments, version, region)

    @validation_decorator
    def vehicle_journeys(self, request_args, version, region):
        return self.__on_ptref(type_pb2.VEHICLE_JOURNEY, self.v.arguments, version, region)

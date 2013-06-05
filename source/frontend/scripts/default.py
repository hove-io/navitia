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
        'administrative_region' : type_pb2.ADMIN,
        'line' : type_pb2.LINE
        }

class Script:
    def __init__(self):
        self.apis = Apis().apis


    def __pagination(self, request_args, ressource_name, resp):
        request_pagination = response_pb2.Pagination()
        request_pagination.startPage = request_args["startPage"]
        request_pagination.itemsPerPage = request_args["count"]
        objects = None
        if resp.ListFields():
            for fd in resp.ListFields():
                if fd[0].name == ressource_name:
                    objects = fd[1]
                    request_pagination.totalResult = len(fd[1])
        else:
            request_pagination.totalResult = 0

        if objects: 
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
            for key, value in request_args.iteritems():
                if key != "startPage":
                    if type(value) == type([]):
                        for v in value:
                            query_args += key + "=" +unicode(v) + "&"
                    else:
                        query_args += key + "=" +unicode(value) + "&"
            if request_pagination.startPage > 0:
                request_pagination.previousPage = query_args+"startPage=%i"%(request_pagination.startPage-1)

            if end<request_pagination.totalResult:
                request_pagination.nextPage = query_args+"startPage=%i"%(request_pagination.startPage+1)
        resp.pagination.CopyFrom(request_pagination)


    


    def status(self, request_args, region):
        req = request_pb2.Request()
        req.requested_api = type_pb2.STATUS
        resp = NavitiaManager().send_and_receive(req, region)
        return resp

    def metadatas(self, request_args, region):
        req = request_pb2.Request()
        req.requested_api = type_pb2.METADATAS
        resp = NavitiaManager().send_and_receive(req, region)
        return resp



    def load(self, request, region, format):
        req = request_pb2.Request()
        req.requested_api = type_pb2.LOAD
        resp = NavitiaManager().send_and_receive(req, region)
        return render_from_protobuf(resp, format, request.args.get('callback'))


    def places(self, request_args, region):
        req = request_pb2.Request()
        req.requested_api = type_pb2.places
        req.places.q = request_args['q']
        req.places.depth = request_args['depth']
        req.places.nbmax = request_args['nbmax']
        for type in request_args["type[]"]:
            req.places.types.append(pb_type[type])

        for admin_uri in request_args["admin_uri[]"]:
            req.places.admin_uris.append(admin_uri)

        resp = NavitiaManager().send_and_receive(req, region)
        self.__pagination(request_args, "places", resp)

	for place in resp.places:
	    if place.HasField("address"):
	        post_code = place.address.name
		if place.address.house_number > 0:
		   post_code = str(place.address.house_number) + " " + place.address.name 
		
		for ad in place.address.administrative_regions:
		    if ad.zip_code != "":
		        post_code = post_code + ", " + ad.zip_code + " " + ad.name
		    else:
			post_code = post_code + ", " + ad.name
		place.name = post_code

        return resp


    def __stop_times(self, request_args, region, departure_filter, arrival_filter, api):
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


    def route_schedules(self, request_args, region):
        return self.__stop_times(request_args, region, request_args["filter"], "", type_pb2.ROUTE_SCHEDULES)

    def next_arrivals(self, request_args, region):
        return self.__stop_times(request_args, region, "", request_args["filter"], type_pb2.NEXT_ARRIVALS)

    def next_departures(self, request_args, region):
        return self.__stop_times(request_args, region, request_args["filter"], "", type_pb2.NEXT_DEPARTURES)

    def stops_schedules(self, request_args, region):
        return self.__stop_times(request_args, region, request_args["departure_filter"], request_args["arrival_filter"],type_pb2.STOPS_SCHEDULES)

    def departure_boards(self, request_args, region):
        return self.__stop_times(request_args, region, request_args["filter"], "", type_pb2.DEPARTURE_BOARDS)

    
    def places_nearby(self, request_args, region):
        req = request_pb2.Request()
        req.requested_api = type_pb2.places_nearby
        req.places_nearby.uri = request_args["uri"]
        req.places_nearby.distance = request_args["distance"]
        req.places_nearby.depth = request_args["depth"]
        for type in request_args["type[]"]:
            req.places_nearby.types.append(pb_type[type])
        resp = NavitiaManager().send_and_receive(req, region)
        self.__pagination(request_args, "places_nearby", resp)
        return resp


    def __fill_display_and_uris(self, resp):
        for journey in resp.journeys:
            for section in journey.sections:
                if section.type == response_pb2.PUBLIC_TRANSPORT:
                    section.pt_display_informations.physical_mode = section.vehicle_journey.physical_mode.name
                    section.pt_display_informations.commercial_mode = section.vehicle_journey.route.line.commercial_mode.name
                    section.pt_display_informations.network = section.vehicle_journey.route.line.network.name
                    section.pt_display_informations.code = section.vehicle_journey.route.line.code
                    section.pt_display_informations.headsign = section.vehicle_journey.route.name
                    if section.destination.HasField("stop_point"):
                        section.pt_display_informations.direction = section.destination.stop_point.name
                    if section.vehicle_journey.route.line.color != "":
                        section.pt_display_informations.color = section.vehicle_journey.route.line.color
                    section.uris.vehicle_journey = section.vehicle_journey.uri
                    section.uris.line = section.vehicle_journey.route.line.uri
                    section.uris.route = section.vehicle_journey.route.uri
                    section.uris.commercial_mode = section.vehicle_journey.route.line.commercial_mode.uri
                    section.uris.physical_mode = section.vehicle_journey.physical_mode.uri
                    section.uris.network = section.vehicle_journey.route.line.network.uri
                    section.ClearField("vehicle_journey")


    def __on_journeys(self, requested_type, request_args, region):
        req = request_pb2.Request()
        req.requested_api = requested_type
        req.journeys.origin = request_args["origin"]
        req.journeys.destination = request_args["destination"] if "destination" in request_args else ""
        req.journeys.datetimes.append(request_args["datetime"])
        req.journeys.clockwise = request_args["clockwise"]
        req.journeys.streetnetwork_params.walking_speed = request_args["walking_speed"]
        req.journeys.streetnetwork_params.walking_distance = request_args["walking_distance"]
        req.journeys.streetnetwork_params.origin_mode = request_args["origin_mode"]
        req.journeys.streetnetwork_params.destination_mode = request_args["destination_mode"] if "destination_mode" in request_args else ""
        req.journeys.streetnetwork_params.bike_speed = request_args["bike_speed"]
        req.journeys.streetnetwork_params.bike_distance = request_args["bike_distance"]
        req.journeys.streetnetwork_params.car_speed = request_args["car_speed"]
        req.journeys.streetnetwork_params.car_distance = request_args["car_distance"]
        req.journeys.streetnetwork_params.vls_speed = request_args["br_speed"]
        req.journeys.streetnetwork_params.vls_distance = request_args["br_distance"]
	req.journeys.streetnetwork_params.origin_filter = request_args["origin_filter"] if "origin_filter" in request_args else ""
	req.journeys.streetnetwork_params.destination_filter = request_args["destination_filter"] if "destination_filter" in request_args else ""
        req.journeys.max_duration = request_args["max_duration"]
        req.journeys.max_transfers = request_args["max_transfers"]
        if req.journeys.streetnetwork_params.origin_mode == "bike_rental":
            req.journeys.streetnetwork_params.origin_mode = "vls"
        if req.journeys.streetnetwork_params.destination_mode == "bike_rental":
            req.journeys.streetnetwork_params.destination_mode = "vls"

        for forbidden_uri in request_args["forbidden_uris[]"]:
            req.journeys.forbidden_uris.append(forbidden_uri)
        resp = NavitiaManager().send_and_receive(req, region)
        if resp.journeys:
            (before, after) = extremes(resp, request_args)
            if before and after:
                resp.prev = before
                resp.next = after
        self.__fill_display_and_uris(resp)
        return resp


    def journeys(self, request_args, region):
        return self.__on_journeys(type_pb2.PLANNER, request_args, region)

    
    def isochrone(self, request_args, region):
        return self.__on_journeys(type_pb2.ISOCHRONE, request_args, region)

    def __on_ptref(self, ressource_name, requested_type, request_args, region):
        req = request_pb2.Request()
        req.requested_api = type_pb2.PTREFERENTIAL

        req.ptref.requested_type = requested_type
        req.ptref.filter = request_args["filter"]
        req.ptref.depth = request_args["depth"]
        
        resp = NavitiaManager().send_and_receive(req, region)
        self.__pagination(request_args, ressource_name, resp) 
        return resp

    def stop_areas(self, request_args, region):
        return self.__on_ptref("stop_areas", type_pb2.STOP_AREA, request_args, region)

    def stop_points(self, request_args, region):
        return self.__on_ptref("stop_points", type_pb2.STOP_POINT, request_args, region)

    def lines(self, request_args, region):
        return self.__on_ptref("lines", type_pb2.LINE, request_args, region)

    def routes(self, request_args, region):
        return self.__on_ptref("routes", type_pb2.ROUTE, request_args,  region)

    def networks(self, request_args, region):
        return self.__on_ptref("networks", type_pb2.NETWORK, request_args, region)

    def physical_modes(self, request_args, region):
        return self.__on_ptref("physical_modes", type_pb2.PHYSICAL_MODE, request_args, region)

    def commercial_modes(self, request_args, region):
        return self.__on_ptref("commercial_modes", type_pb2.COMMERCIAL_MODE, request_args, region)

    def connections(self, request_args, region):
        return self.__on_ptref("connections", type_pb2.CONNECTION, request_args, region)

    def journey_pattern_points(self, request_args, region):
        return self.__on_ptref("journey_pattern_points", type_pb2.JOURNEY_PATTERN_POINT, request_args,  region)

    def journey_patterns(self, request_args, region):
        return self.__on_ptref("journey_patterns", type_pb2.JOURNEY_PATTERN, request_args, region)

    def companies(self, request_args, region):
        return self.__on_ptref("companies", type_pb2.COMPANY, request_args, region)

    def vehicle_journeys(self, request_args, region):
        return self.__on_ptref("vehicle_journeys", type_pb2.VEHICLE_JOURNEY, request_args, region)
    
    def pois(self, request_args, region):
        return self.__on_ptref("pois", type_pb2.POI, request_args, region)
    
    def poi_types(self, request_args, region):
        return self.__on_ptref("poi_types", type_pb2.POITYPE, request_args, region)

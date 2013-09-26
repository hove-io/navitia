# coding=utf-8
import type_pb2
import request_pb2
import response_pb2
from protobuf_to_dict import protobuf_to_dict

from instance_manager import NavitiaManager, DeadSocketException, RegionNotFound
from renderers import render, render_from_protobuf
from werkzeug.wrappers import Response
from find_extrem_datetimes import *
from qualifier import qualifier

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
        self.apis = ["places", "places_nearby", "next_departures",
                     "next_arrivals", "route_schedules", "stops_schedules",
                     "departure_boards", "stop_areas", "stop_points", "lines",
                     "routes", "physical_modes", "commercial_modes",
                     "connections", "journey_pattern_points",
                     "journey_patterns", "companies", "vehicle_journeys",
                     "pois", "poi_types", "journeys", "isochrone", "metadatas",
                     "status", "load", "networks"]




    def __pagination(self, request, ressource_name, resp):
        if resp.pagination.totalResult > 0:
            query_args = ""
            for key, value in request.iteritems():
                if key != "startPage":
                    if type(value) == type([]):
                        for v in value:
                            query_args += key + "=" +unicode(v) + "&"
                    else:
                        query_args += key + "=" +unicode(value) + "&"
            if resp.pagination.startPage > 0:
                page = resp.pagination.startPage-1
                resp.pagination.previousPage = query_args+"start_page=%i"% page
            last_id_page = (resp.pagination.startPage + 1) * resp.pagination.itemsPerPage
            if last_id_page  < resp.pagination.totalResult:
                page = resp.pagination.startPage+1
                resp.pagination.nextPage = query_args+"start_page=%i"% page


    def status(self, request, region):
        req = request_pb2.Request()
        req.requested_api = type_pb2.STATUS
        resp = NavitiaManager().send_and_receive(req, region)
        return resp

    def metadatas(self, request, region):
        req = request_pb2.Request()
        req.requested_api = type_pb2.METADATAS
        resp = NavitiaManager().send_and_receive(req, region)
        return resp



    def load(self, request, region, format):
        req = request_pb2.Request()
        req.requested_api = type_pb2.LOAD
        resp = NavitiaManager().send_and_receive(req, region)
        return render_from_protobuf(resp, format, request.arguments.get('callback'))


    def places(self, request, region):
        req = request_pb2.Request()
        req.requested_api = type_pb2.places
        req.places.q     = request['q']
        req.places.depth = request['depth']
        req.places.count = request['count']
        if request["type[]"]:
            for type in request["type[]"]:
                req.places.types.append(pb_type[type])

        if request["admin_uri[]"]:
            for admin_uri in request["admin_uri[]"]:
                req.places.admin_uris.append(admin_uri)

        resp = NavitiaManager().send_and_receive(req, region)
        self.__pagination(request, "places", resp)

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


    def __stop_times(self, request, region, departure_filter, arrival_filter, api):
        req = request_pb2.Request()
        req.requested_api = api
        st = req.next_stop_times
        st.departure_filter = departure_filter
        st.arrival_filter = arrival_filter
        st.from_datetime = request["from_datetime"]
        st.duration      = request["duration"]
        st.depth         = request["depth"]
        st.nb_stoptimes  = 0 if not "nb_stoptimes" in request.keys()\
                             else request["nb_stoptimes"]
        st.interface_version = 0 if not "interface_version" in request.keys()\
                                  else request["interface_version"]
        st.count = 10 if not "count" in request.keys() else request["count"]
        st.start_page = 0 if not "start_page" in request.keys()\
                           else request["start_page"]
        resp = NavitiaManager().send_and_receive(req, region)
        return resp


    def route_schedules(self, request, region):
        return self.__stop_times(request, region, request["filter"], "", type_pb2.ROUTE_SCHEDULES)

    def next_arrivals(self, request, region):
        return self.__stop_times(request, region, "", request["filter"], type_pb2.NEXT_ARRIVALS)

    def next_departures(self, request, region):
        return self.__stop_times(request, region, request["filter"], "", type_pb2.NEXT_DEPARTURES)

    def stops_schedules(self, request, region):
        return self.__stop_times(request, region, request["departure_filter"], request["arrival_filter"],type_pb2.STOPS_SCHEDULES)

    def departure_boards(self, request, region):
        return self.__stop_times(request, region, request["filter"], "", type_pb2.DEPARTURE_BOARDS)


    def places_nearby(self, request, region):
        req = request_pb2.Request()
        req.requested_api = type_pb2.places_nearby
        req.places_nearby.uri      = request["uri"]
        req.places_nearby.distance = request["distance"]
        req.places_nearby.depth    = request["depth"]
        req.places_nearby.count    = request["count"]
        if request["type[]"]:
            for type in request["type[]"]:
                req.places_nearby.types.append(pb_type[type])
        resp = NavitiaManager().send_and_receive(req, region)
        self.__pagination(request, "places_nearby", resp)
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
                    if(len(section.vehicle_journey.odt_message) > 0):
                        section.pt_display_informations.description = section.vehicle_journey.odt_message
                    section.pt_display_informations.vehicle_journey_type = section.vehicle_journey.vehicle_journey_type
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

    def get_journey(self, req, region, type_):
        if req.requested_api == type_pb2.PLANNER :
            resp = qualifier().qualifier_one(req, region)
        else:
            resp = NavitiaManager().send_and_receive(req, region)

        if resp.response_type in [response_pb2.NO_ORIGIN_NOR_DESTINATION_POINT,
                                  response_pb2.NO_ORIGIN_POINT,
                                  response_pb2.NO_DESTINATION_POINT]:
            resp.error = """
                        Could not find a stop point nearby.
                        Check the coordinates (did you mix up longitude and
                        latitude?). Maybe you are out of the covered region.
                        Maybe the coordinate snaped to a street of
                        OpenStreetMap with no connectivity to the street
                        network."""
        if resp.response_type == response_pb2.NO_SOLUTION:
            resp.error = "We found no solution. Maybe the are no vehicle \
                          running that day on all the nearest stop points?"
        if not resp.error and type_ == "asap":
            #We are looking for the asap result
            earliest_dt = None
            earliest_i = None
            for i in range(0, len(resp.journeys)):
                if not earliest_dt or\
                       earliest_dt > resp.journeys[i].arrival_date_time:
                    earliest_dt = resp.journeys[i].arrival_date_time
                    earliest_i = i
            if earliest_dt:
                #We list the journeys to delete
                to_delete = range(0, len(resp.journeys))
                del to_delete[earliest_i]
                to_delete.sort(reverse=True)
                #And then we delete it
                for i in to_delete:
                    del resp.journeys[i]
        #self.__fill_display_and_uris(resp)
        return resp


    def __on_journeys(self, requested_type, request, region):
	req = request_pb2.Request()
        req.requested_api = requested_type
        req.journeys.origin = request["origin"]
        if request["destination"]:
            req.journeys.destination = request["destination"] #if "destination" in request.keys() else ""
        req.journeys.datetimes.append(request["datetime"])
        req.journeys.clockwise = request["clockwise"]
        req.journeys.streetnetwork_params.walking_speed = request["walking_speed"]
        req.journeys.streetnetwork_params.walking_distance = request["walking_distance"]
        req.journeys.streetnetwork_params.origin_mode = request["origin_mode"]
        req.journeys.streetnetwork_params.destination_mode = request["destination_mode"] if "destination_mode" in request else ""
        req.journeys.streetnetwork_params.bike_speed = request["bike_speed"]
        req.journeys.streetnetwork_params.bike_distance = request["bike_distance"]
        req.journeys.streetnetwork_params.car_speed = request["car_speed"]
        req.journeys.streetnetwork_params.car_distance = request["car_distance"]
        req.journeys.streetnetwork_params.vls_speed = request["br_speed"]
        req.journeys.streetnetwork_params.vls_distance = request["br_distance"]
        req.journeys.streetnetwork_params.origin_filter = request["origin_filter"] if "origin_filter" in request else ""
        req.journeys.streetnetwork_params.destination_filter = request["destination_filter"] if "destination_filter" in request else ""
        req.journeys.max_duration = request["max_duration"]
        req.journeys.max_transfers = request["max_transfers"]
        if req.journeys.streetnetwork_params.origin_mode == "bike_rental":
            req.journeys.streetnetwork_params.origin_mode = "vls"
        if req.journeys.streetnetwork_params.destination_mode == "bike_rental":
            req.journeys.streetnetwork_params.destination_mode = "vls"
        if "forbidden_uris[]" in request and request["forbidden_uris[]"]:
            for forbidden_uri in request["forbidden_uris[]"]:
                req.journeys.forbidden_uris.append(forbidden_uri)
        resp = self.get_journey(req, region, request["type"])
        if len(resp.error) == 0:
            while request["count"] and request["count"] > len(resp.journeys):
                datetime = None
                if request["clockwise"]:
                    datetime = resp.journeys[-1].departure_date_time
                    last = str(int(datetime[-1])+1)
                else:
                    datetime = resp.journeys[-1].arrival_date_time
                    last = str(int(datetime[-1])-1)
                datetime = datetime[:-1] + last
                req.journeys.datetimes[0] = datetime
                tmp_resp = self.get_journey(req, region, request["type"])
                if len(tmp_resp.error) > 0 and len(tmp_resp.journeys) == 0:
                    break
                else:
                    resp.journeys.extend(tmp_resp.journeys)

            if request["count"] and len(resp.journeys) > request["count"]:
                to_delete = range(request["count"], len(resp.journeys))
                to_delete.sort(reverse=True)
                for i in to_delete:
                    del resp.journeys[i]
        return resp


    def journeys(self, request, region):
        return self.__on_journeys(type_pb2.PLANNER, request, region)


    def isochrone(self, request, region):
        return self.__on_journeys(type_pb2.ISOCHRONE, request, region)

    def __on_ptref(self, resource_name, requested_type, request, region):
        req = request_pb2.Request()
        req.requested_api = type_pb2.PTREFERENTIAL

        req.ptref.requested_type = requested_type
        req.ptref.filter     = request["filter"]
        req.ptref.depth      = request["depth"]
        req.ptref.start_page = request["start_page"]
        req.ptref.count      = request["count"]

        resp = NavitiaManager().send_and_receive(req, region)
        self.__pagination(request, resource_name, resp)
        return resp

    def stop_areas(self, request, region):
        return self.__on_ptref("stop_areas", type_pb2.STOP_AREA, request, region)

    def stop_points(self, request, region):
        return self.__on_ptref("stop_points", type_pb2.STOP_POINT, request, region)

    def lines(self, request, region):
        return self.__on_ptref("lines", type_pb2.LINE, request, region)

    def routes(self, request, region):
        return self.__on_ptref("routes", type_pb2.ROUTE, request,  region)

    def networks(self, request, region):
        return self.__on_ptref("networks", type_pb2.NETWORK, request, region)

    def physical_modes(self, request, region):
        return self.__on_ptref("physical_modes", type_pb2.PHYSICAL_MODE, request, region)

    def commercial_modes(self, request, region):
        return self.__on_ptref("commercial_modes", type_pb2.COMMERCIAL_MODE, request, region)

    def connections(self, request, region):
        return self.__on_ptref("connections", type_pb2.CONNECTION, request, region)

    def journey_pattern_points(self, request, region):
        return self.__on_ptref("journey_pattern_points", type_pb2.JOURNEY_PATTERN_POINT, request,  region)

    def journey_patterns(self, request, region):
        return self.__on_ptref("journey_patterns", type_pb2.JOURNEY_PATTERN, request, region)

    def companies(self, request, region):
        return self.__on_ptref("companies", type_pb2.COMPANY, request, region)

    def vehicle_journeys(self, request, region):
        return self.__on_ptref("vehicle_journeys", type_pb2.VEHICLE_JOURNEY, request, region)

    def pois(self, request, region):
        return self.__on_ptref("pois", type_pb2.POI, request, region)

    def poi_types(self, request, region):
        return self.__on_ptref("poi_types", type_pb2.POITYPE, request, region)

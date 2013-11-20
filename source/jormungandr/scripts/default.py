# coding=utf-8
import type_pb2
import request_pb2
import response_pb2
from instance_manager import NavitiaManager
from renderers import render_from_protobuf
from qualifier import qualifier_one
from datetime import datetime, timedelta
import itertools

pb_type = {
        'stop_area': type_pb2.STOP_AREA,
        'stop_point': type_pb2.STOP_POINT,
        'city': type_pb2.CITY,
        'address': type_pb2.ADDRESS,
        'poi': type_pb2.POI ,
        'administrative_region' : type_pb2.ADMINISTRATIVE_REGION,
        'line' : type_pb2.LINE
        }

class Script(object):
    def __init__(self):
        self.apis = ["places", "places_nearby", "next_departures",
                     "next_arrivals", "route_schedules", "stops_schedules",
                     "departure_boards", "stop_areas", "stop_points", "lines",
                     "routes", "physical_modes", "commercial_modes",
                     "connections", "journey_pattern_points",
                     "journey_patterns", "companies", "vehicle_journeys",
                     "pois", "poi_types", "journeys", "isochrone", "metadatas",
                     "status", "load", "networks", "place_uri"]


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
        req.places.search_type = request['search_type']
        if request["type[]"]:
            for type in request["type[]"]:
                req.places.types.append(pb_type[type])

        if request["admin_uri[]"]:
            for admin_uri in request["admin_uri[]"]:
                req.places.admin_uris.append(admin_uri)

        resp = NavitiaManager().send_and_receive(req, region)
        if len(resp.places) == 0 and request['search_type'] == 0:
            request["search_type"] = 1
            return self.places(request, region)
        self.__pagination(request, "places", resp)

        return resp

    def place_uri(self, request, region):
        req = request_pb2.Request()
        req.requested_api = type_pb2.place_uri
        req.place_uri.uri = request["uri"]
        return NavitiaManager().send_and_receive(req, region)

    def __stop_times(self, request, region, departure_filter, arrival_filter, api):
        req = request_pb2.Request()
        req.requested_api = api
        st = req.next_stop_times
        st.departure_filter = departure_filter
        st.arrival_filter = arrival_filter
        st.from_datetime = request["from_datetime"]
        st.duration      = request["duration"]
        st.depth         = request["depth"]
        st.nb_stoptimes  = 0 if not "nb_stoptimes" in request.keys() \
                             else request["nb_stoptimes"]
        st.interface_version = 0 if not "interface_version" in request.keys() \
                                  else request["interface_version"]
        st.count = 10 if not "count" in request.keys() else request["count"]
        st.start_page = 0 if not "start_page" in request.keys() \
                           else request["start_page"]
        if request["max_date_times"]:
            st.max_date_times = request["max_date_times"]
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
        req.places_nearby.uri        = request["uri"]
        req.places_nearby.distance   = request["distance"]
        req.places_nearby.depth      = request["depth"]
        req.places_nearby.count      = request["count"]
        req.places_nearby.start_page = request["start_page"]
        if request["type[]"]:
            for type in request["type[]"]:
                req.places_nearby.types.append(pb_type[type])
        req.places_nearby.filter = request["filter"]
        resp = NavitiaManager().send_and_receive(req, region)
        self.__pagination(request, "places_nearby", resp)
        return resp


    def __fill_uris(self, resp):
        for journey in resp.journeys:
            for section in journey.sections:

                if section.type == response_pb2.PUBLIC_TRANSPORT:
                    if section.HasField("pt_display_informations"):
                        section.uris.vehicle_journey = section.pt_display_informations.uris.vehicle_journey
                        section.uris.line = section.pt_display_informations.uris.line
                        section.uris.route = section.pt_display_informations.uris.route
                        section.uris.commercial_mode = section.pt_display_informations.uris.commercial_mode
                        section.uris.physical_mode = section.pt_display_informations.uris.physical_mode
                        section.uris.network = section.pt_display_informations.uris.network

    def get_journey(self, req, region, trip_type):
        resp = None

        for origin_mode, destination_mode in itertools.product(
                self.origin_modes, self.destination_modes):
            req.journeys.streetnetwork_params.origin_mode = origin_mode
            req.journeys.streetnetwork_params.destination_mode = destination_mode
            resp = NavitiaManager().send_and_receive(req, region)
            if resp.response_type == response_pb2.ITINERARY_FOUND:
                if req.requested_api == type_pb2.PLANNER:
                    qualifier_one(resp.journeys)
                break#result found, no need to inspect other fallback mode

        if resp and not resp.HasField("error") and trip_type == "rapid":
            #We are looking for the asap result
            earliest_dt = None
            earliest_i = None
            for i in range(0, len(resp.journeys)):
                if not earliest_dt \
                       or earliest_dt > resp.journeys[i].arrival_date_time:
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
        self.__fill_uris(resp)
        return resp


    def journey_compare(self, j1, j2):
        if datetime.strptime(j1.arrival_date_time, "%Y%m%dT%H%M%S") > datetime.strptime(j2.arrival_date_time, "%Y%m%dT%H%M%S") :
            return 1
        elif datetime.strptime(j1.arrival_date_time, "%Y%m%dT%H%M%S") == datetime.strptime(j2.arrival_date_time, "%Y%m%dT%H%M%S") :
            return 0
        else:
            return -1


    def __on_journeys(self, requested_type, request, region):
        req = request_pb2.Request()
        req.requested_api = requested_type
        req.journeys.origin = request["origin"]
        if request.has_key("destination") and request["destination"]:
            req.journeys.destination = request["destination"]
            self.destination_modes = request["destination_mode"]
        else:
            self.destination_modes = ["walking"]
        req.journeys.datetimes.append(request["datetime"])
        req.journeys.clockwise = request["clockwise"]
        req.journeys.streetnetwork_params.walking_speed = request["walking_speed"]
        req.journeys.streetnetwork_params.walking_distance = request["walking_distance"]
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
        req.journeys.wheelchair = request["wheelchair"]

        self.origin_modes = request["origin_mode"]

        if req.journeys.streetnetwork_params.origin_mode == "bike_rental":
            req.journeys.streetnetwork_params.origin_mode = "vls"
        if req.journeys.streetnetwork_params.destination_mode == "bike_rental":
            req.journeys.streetnetwork_params.destination_mode = "vls"
        if "forbidden_uris[]" in request and request["forbidden_uris[]"]:
            for forbidden_uri in request["forbidden_uris[]"]:
                req.journeys.forbidden_uris.append(forbidden_uri)
        if not request.has_key("type"):
            request["type"] = "all"
        #call to kraken
        resp = self.get_journey(req, region, request["type"])
        if len(resp.journeys) > 0 and request.has_key("count"):
            while request["count"] and request["count"] > len(resp.journeys):
                temp_datetime = None
                if request["clockwise"]:
                    str_dt = ""
                    if resp.journeys[-1].HasField("departure_date_time"):
                        temp_datetime = datetime.strptime(resp.journeys[-1].departure_date_time, "%Y%m%dT%H%M%S") + timedelta(seconds = 1)
                    else:
                        duration = int(resp.journeys[-1].duration) + 1
                        temp_datetime = datetime.strptime(req.journeys.datetimes[0], "%Y%m%dT%H%M%S") + timedelta(seconds=duration)
                else:
                    if resp.journeys[-1].HasField("arrival_date_time"):
                        temp_datetime = datetime.strptime(resp.journeys[-1].arrival_date_time, "%Y%m%dT%H%M%S") + timedelta(seconds = -1)
                    else:
                        duration = int(resp.journeys[-1].duration) -1
                        temp_datetime = datetime.strptime(req.journeys.datetimes[0], "%Y%m%dT%H%M%S") + timedelta(seconds=duration)


                req.journeys.datetimes[0] = temp_datetime.strftime("%Y%m%dT%H%M%S")
                tmp_resp = self.get_journey(req, region, request["type"])
                if len(tmp_resp.journeys) == 0:
                    break
                else:
                    resp.journeys.extend(tmp_resp.journeys)
            to_delete = []
            if request['destination']:
                for i in range(0, len(resp.journeys)):
                    if resp.journeys[i].type == "" and not i in to_delete:
                        to_delete.append(i)

            to_delete.sort(reverse=True)
            for i in to_delete:
                del resp.journeys[i]

            if request["count"] and len(resp.journeys) > request["count"]:
                to_delete = range(request["count"], len(resp.journeys))
                to_delete.sort(reverse=True)
                for i in to_delete:
                    del resp.journeys[i]

            if not request["clockwise"]:
                resp.journeys.sort(self.journey_compare)
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

# coding=utf-8
import copy
import navitiacommon.type_pb2 as type_pb2
import navitiacommon.request_pb2 as request_pb2
import navitiacommon.response_pb2 as response_pb2
from jormungandr.interfaces.v1.Journeys import journey
from jormungandr.renderers import render_from_protobuf
from qualifier import qualifier_one
from datetime import datetime, timedelta
import itertools

pb_type = {
    'stop_area': type_pb2.STOP_AREA,
    'stop_point': type_pb2.STOP_POINT,
    'address': type_pb2.ADDRESS,
    'poi': type_pb2.POI,
    'administrative_region': type_pb2.ADMINISTRATIVE_REGION,
    'line': type_pb2.LINE
}

f_date_time = "%Y%m%dT%H%M%S"


class Script(object):
    def __init__(self):
        self.apis = ["places", "places_nearby", "next_departures",
                     "next_arrivals", "route_schedules", "stops_schedules",
                     "departure_boards", "stop_areas", "stop_points", "lines",
                     "routes", "physical_modes", "commercial_modes",
                     "connections", "journey_pattern_points",
                     "journey_patterns", "companies", "vehicle_journeys",
                     "pois", "poi_types", "journeys", "isochrone", "metadatas",
                     "status", "load", "networks", "place_uri", "disruptions",
                     "calendars"]
        self.functional_params = {}

    def __pagination(self, request, ressource_name, resp):
        pagination = resp.pagination
        if pagination.totalResult > 0:
            query_args = ""
            for key, value in request.iteritems():
                if key != "startPage":
                    if isinstance(value, type([])):
                        for v in value:
                            query_args += key + "=" + unicode(v) + "&"
                    else:
                        query_args += key + "=" + unicode(value) + "&"
            if pagination.startPage > 0:
                page = pagination.startPage - 1
                pagination.previousPage = query_args
                pagination.previousPage += "start_page=%i" % page
            last_id_page = (pagination.startPage + 1) * pagination.itemsPerPage
            if last_id_page < pagination.totalResult:
                page = pagination.startPage + 1
                pagination.nextPage = query_args + "start_page=%i" % page

    def status(self, request, instance):
        req = request_pb2.Request()
        req.requested_api = type_pb2.STATUS
        resp = instance.send_and_receive(req)
        return resp

    def metadatas(self, request, instance):
        req = request_pb2.Request()
        req.requested_api = type_pb2.METADATAS
        resp = instance.send_and_receive(req)
        return resp

    def disruptions(self, request, instance):
        req = request_pb2.Request()
        req.requested_api = type_pb2.disruptions
        req.disruptions.depth = request['depth']
        req.disruptions.filter = request['filter']
        req.disruptions.count = request['count']
        req.disruptions.start_page = request['start_page']
        req.disruptions.datetime = request['datetime']
        req.disruptions.period = request['period']
        if request["forbidden_uris[]"]:
            for forbidden_uri in request["forbidden_uris[]"]:
                req.ptref.forbidden_uri.append(forbidden_uri)

        resp = instance.send_and_receive(req)
        return resp

    def load(self, request, instance, format):
        req = request_pb2.Request()
        req.requested_api = type_pb2.LOAD
        resp = instance.send_and_receive(req)
        return render_from_protobuf(resp, format,
                                    request.arguments.get('callback'))

    def places(self, request, instance):
        req = request_pb2.Request()
        req.requested_api = type_pb2.places
        req.places.q = request['q']
        req.places.depth = request['depth']
        req.places.count = request['count']
        req.places.search_type = request['search_type']
        if request["type[]"]:
            for type in request["type[]"]:
                req.places.types.append(pb_type[type])

        if request["admin_uri[]"]:
            for admin_uri in request["admin_uri[]"]:
                req.places.admin_uris.append(admin_uri)

        resp = instance.send_and_receive(req)
        if len(resp.places) == 0 and request['search_type'] == 0:
            request["search_type"] = 1
            return self.places(request, instance)
        self.__pagination(request, "places", resp)

        return resp

    def place_uri(self, request, instance):
        req = request_pb2.Request()
        req.requested_api = type_pb2.place_uri
        req.place_uri.uri = request["uri"]
        return instance.send_and_receive(req)

    def __stop_times(self, request, instance, departure_filter,
                     arrival_filter, api):
        req = request_pb2.Request()
        req.requested_api = api
        st = req.next_stop_times
        st.departure_filter = departure_filter
        st.arrival_filter = arrival_filter
        st.from_datetime = request["from_datetime"]
        st.duration = request["duration"]
        st.depth = request["depth"]
        if not "nb_stoptimes" in request:
            st.nb_stoptimes = 0
        else:
            st.nb_stoptimes = request["nb_stoptimes"]
        if not "interface_version" in request:
            st.interface_version = 0
        else:
            st.interface_version = request["interface_version"]
        st.count = 10 if not "count" in request.keys() else request["count"]
        if not "start_page" in request:
            st.start_page = 0
        else:
            st.start_page = request["start_page"]
        if request["max_date_times"]:
            st.max_date_times = request["max_date_times"]
        if request["forbidden_uris[]"]:
            for forbidden_uri in request["forbidden_uris[]"]:
                st.forbidden_uri.append(forbidden_uri)
        resp = instance.send_and_receive(req)
        return resp

    def route_schedules(self, request, instance):
        return self.__stop_times(request, instance, request["filter"], "",
                                 type_pb2.ROUTE_SCHEDULES)

    def next_arrivals(self, request, instance):
        return self.__stop_times(request, instance, "", request["filter"],
                                 type_pb2.NEXT_ARRIVALS)

    def next_departures(self, request, instance):
        return self.__stop_times(request, instance, request["filter"], "",
                                 type_pb2.NEXT_DEPARTURES)

    def stops_schedules(self, request, instance):
        return self.__stop_times(request, instance,
                                 request["departure_filter"],
                                 request["arrival_filter"],
                                 type_pb2.STOPS_SCHEDULES)

    def departure_boards(self, request, instance):
        return self.__stop_times(request, instance, request["filter"], "",
                                 type_pb2.DEPARTURE_BOARDS)

    def parse_journey_request(self, requested_type, request):
        """Parse the request dict and create the protobuf version"""
        req = request_pb2.Request()
        req.requested_api = requested_type
        req.journeys.origin = request["origin"]
        if "destination" in request and request["destination"]:
            req.journeys.destination = request["destination"]
            self.destination_modes = request["destination_mode"]
        else:
            self.destination_modes = ["walking"]
        req.journeys.datetimes.append(request["datetime"])
        req.journeys.clockwise = request["clockwise"]
        sn_params = req.journeys.streetnetwork_params
        if "max_duration_to_pt" in request:
            sn_params.max_duration_to_pt = request["max_duration_to_pt"]
        else:
            # for the moment we compute the non_TC duration
            # with the walking_distance
            max_duration = request["walking_distance"] / request["walking_speed"]
            sn_params.max_duration_to_pt = max_duration
        sn_params.walking_speed = request["walking_speed"]
        sn_params.bike_speed = request["bike_speed"]
        sn_params.car_speed = request["car_speed"]
        sn_params.bss_speed = request["bss_speed"]
        if "origin_filter" in request:
            sn_params.origin_filter = request["origin_filter"]
        else:
            sn_params.origin_filter = ""
        if "destination_filter" in request:
            sn_params.destination_filter = request["destination_filter"]
        else:
            sn_params.destination_filter = ""
        req.journeys.max_duration = request["max_duration"]
        req.journeys.max_transfers = request["max_transfers"]
        req.journeys.wheelchair = request["wheelchair"]

        self.origin_modes = request["origin_mode"]

        if req.journeys.streetnetwork_params.origin_mode == "bike_rental":
            req.journeys.streetnetwork_params.origin_mode = "bss"
        if req.journeys.streetnetwork_params.destination_mode == "bike_rental":
            req.journeys.streetnetwork_params.destination_mode = "bss"
        if "forbidden_uris[]" in request and request["forbidden_uris[]"]:
            for forbidden_uri in request["forbidden_uris[]"]:
                req.journeys.forbidden_uris.append(forbidden_uri)
        if not "type" in request:
            request["type"] = "all" #why ?

        return req

    def check_missing_journey(self, list_journey, initial_request):
        """ Check if some particular journeys are missing, and return:
                if it is the case a modified version of the request to be rerun
                else None"""
        if not "cheap_journey" in self.functional_params \
            or self.functional_params["cheap_journey"] != "True":
            return

        #we want to check if all journeys use the TER network
        #if it is true, we want to call kraken and forbid this network
        ter_uris = ["network:TER", "network:SNCF"]
        only_ter = all(
            any(
                section.pt_display_informations.uris.network in ter_uris
                for section in journey.sections
            )
            for journey in list_journey
        )

        if not only_ter:
            return None

        req = copy.deepcopy(initial_request)
        for uri in ter_uris:
            req.journeys.forbidden_uris.append(uri)

        return req

    def places_nearby(self, request, instance):
        req = request_pb2.Request()
        req.requested_api = type_pb2.places_nearby
        req.places_nearby.uri = request["uri"]
        req.places_nearby.distance = request["distance"]
        req.places_nearby.depth = request["depth"]
        req.places_nearby.count = request["count"]
        req.places_nearby.start_page = request["start_page"]
        if request["type[]"]:
            for type in request["type[]"]:
                req.places_nearby.types.append(pb_type[type])
        req.places_nearby.filter = request["filter"]
        resp = instance.send_and_receive(req)
        self.__pagination(request, "places_nearby", resp)
        return resp

    def __fill_uris(self, resp):
        for journey in resp.journeys:
            for section in journey.sections:
                if section.type != response_pb2.PUBLIC_TRANSPORT:
                    continue
                if section.HasField("pt_display_informations"):
                    uris = section.uris
                    pt_infos = section.pt_display_informations
                    uris.vehicle_journey = pt_infos.uris.vehicle_journey
                    uris.line = pt_infos.uris.line
                    uris.route = pt_infos.uris.route
                    uris.commercial_mode = pt_infos.uris.commercial_mode
                    uris.physical_mode = pt_infos.uris.physical_mode
                    uris.network = pt_infos.uris.network

    def call_kraken(self, req, instance):
        resp = None

        for o_mode, d_mode in itertools.product(
                self.origin_modes, self.destination_modes):
            req.journeys.streetnetwork_params.origin_mode = o_mode
            req.journeys.streetnetwork_params.destination_mode = d_mode
            resp = instance.send_and_receive(req)
            if resp.response_type == response_pb2.ITINERARY_FOUND:
                break  # result found, no need to inspect other fallback mode

        self.__fill_uris(resp)
        return resp

    def get_journey(self, pb_req, instance, original_request):
        resp = self.call_kraken(pb_req, instance)

        if not resp or pb_req.requested_api != type_pb2.PLANNER:
            return

        new_request = self.check_missing_journey(resp.journeys, pb_req)

        if new_request:
            #we have to call kraken again with a modified version of the request
            new_resp = self.call_kraken(new_request, instance)
            self.merge_response(resp, new_resp)

        #we qualify the journeys
        qualifier_one(resp.journeys)

        #we filter the journeys
        self.delete_journeys(resp, original_request)
        return resp


    def journey_compare(self, j1, j2):
        arrival_j1_f = datetime.strptime(j1.arrival_date_time, f_date_time)
        arrival_j2_f = datetime.strptime(j2.arrival_date_time, f_date_time)
        if arrival_j1_f > arrival_j2_f:
            return 1
        elif arrival_j1_f == arrival_j2_f:
            return 0
        else:
            return -1

    def fill_journeys(self, pb_req, request, instance):
        resp = self.get_journey(pb_req, instance, request)

        if len(resp.journeys) == 0:
            return resp  # no journeys found, useless to call kraken again

        while request["count"] > len(resp.journeys):
            temp_datetime = None
            if request['clockwise']:
                str_dt = ""
                last_journey = resp.journeys[-1]
                if last_journey.HasField("departure_date_time"):
                    l_date_time = last_journey.departure_date_time
                    l_date_time_f = datetime.strptime(l_date_time, f_date_time)
                    temp_datetime = l_date_time_f + timedelta(seconds=1)
                else:
                    duration = int(resp.journeys[-1].duration) + 1
                    r_datetime = pb_req.journeys.datetimes[0]
                    r_datetime_f = datetime.strptime(r_datetime, f_date_time)
                    temp_datetime = r_datetime_f + timedelta(seconds=duration)
            else:
                last_journey = resp.journeys[0]
                if resp.journeys[-1].HasField("arrival_date_time"):
                    l_date_time = last_journey.arrival_date_time
                    l_date_time_f = datetime.strptime(l_date_time, f_date_time)
                    temp_datetime = l_date_time_f + timedelta(seconds=-1)
                else:
                    duration = int(resp.journeys[-1].duration) - 1
                    r_datetime = pb_req.journeys.datetimes[0]
                    r_datetime_f = datetime.strptime(r_datetime, f_date_time)
                    temp_datetime = r_datetime_f + timedelta(seconds=duration)

            pb_req.journeys.datetimes[0] = temp_datetime.strftime(f_date_time)
            tmp_resp = self.get_journey(pb_req, instance, request)

            if len(tmp_resp.journeys) == 0:
                break  #no more journeys found, we stop

            self.merge_response(resp, tmp_resp)

        self.delete_journeys(resp, request)  # last filter
        return resp

    def merge_response(self, initial_response, new_response):
        #since it's not the first call to kraken, some kraken's id
        #might not be uniq anymore
        self.change_ids(new_response, len(initial_response.journeys))
        if len(new_response.journeys) == 0:
            return

        initial_response.journeys.extend(new_response.journeys)
        #we have to add the addition fare too
        if new_response.tickets:
            initial_response.tickets.extend(new_response.tickets)

    @staticmethod
    def change_ids(new_journeys, journey_count):
        #we need to change the fare id, the section id and the fare ref in the journey
        for ticket in new_journeys.tickets:
            journey_count += 1
            ticket.id = ticket.id + '_' + str(journey_count)
            for i in range(len(ticket.section_id)):
                ticket.section_id[i] = ticket.section_id[i] + '_' + str(journey_count)

        for new_journey in new_journeys.journeys:
            for i in range(len(new_journey.fare.ticket_id)):
                new_journey.fare.ticket_id[i] = new_journey.fare.ticket_id[i] \
                                                + '_' + str(journey_count)

            for section in new_journey.sections:
                section.id = section.id + '_' + str(journey_count)

    def delete_journeys(self, resp, request):
        if request["debug"] or not resp:
            return #in debug we want to keep all journeys

        if not request['destination']:
            return #for isochrone we don't want to filter

        if resp.HasField("error"):
            return #we don't filter anything if errors

        #filter on journey type (the qualifier)
        to_delete = []
        if request["type"] != "" and request["type"] != "all":
            to_delete.extend([idx for idx, j in enumerate(resp.journeys) if j.type != request["type"]])
        else:
            #by default, we filter non tagged journeys
            to_delete.extend([idx for idx, j in enumerate(resp.journeys) if j.type == ""])

        # list comprehension does not work with repeated field, so we have to delete them manually
        to_delete.sort(reverse=True)
        for idx in to_delete:
            del resp.journeys[idx]

        #after all filters, we filter not to give too many results
        if request["count"] and len(resp.journeys) > request["count"]:
            del resp.journeys[request["count"]:]

    def __on_journeys(self, requested_type, request, instance):
        req = self.parse_journey_request(requested_type, request)

        # call to kraken
        resp = self.fill_journeys(req, request, instance)

        if not request["clockwise"]:
            resp.journeys.sort(self.journey_compare)

        return resp

    def journeys(self, request, instance):
        return self.__on_journeys(type_pb2.PLANNER, request, instance)

    def isochrone(self, request, instance):
        return self.__on_journeys(type_pb2.ISOCHRONE, request, instance)

    def __on_ptref(self, resource_name, requested_type, request, instance):
        req = request_pb2.Request()
        req.requested_api = type_pb2.PTREFERENTIAL

        req.ptref.requested_type = requested_type
        req.ptref.filter = request["filter"]
        req.ptref.depth = request["depth"]
        req.ptref.start_page = request["start_page"]
        req.ptref.count = request["count"]
        if request["forbidden_uris[]"]:
            for forbidden_uri in request["forbidden_uris[]"]:
                req.ptref.forbidden_uri.append(forbidden_uri)

        resp = instance.send_and_receive(req)
        self.__pagination(request, resource_name, resp)
        return resp

    def stop_areas(self, request, instance):
        return self.__on_ptref("stop_areas", type_pb2.STOP_AREA, request,
                               instance)

    def stop_points(self, request, instance):
        return self.__on_ptref("stop_points", type_pb2.STOP_POINT, request,
                               instance)

    def lines(self, request, instance):
        return self.__on_ptref("lines", type_pb2.LINE, request, instance)

    def routes(self, request, instance):
        return self.__on_ptref("routes", type_pb2.ROUTE, request, instance)

    def networks(self, request, instance):
        return self.__on_ptref("networks", type_pb2.NETWORK, request, instance)

    def physical_modes(self, request, instance):
        return self.__on_ptref("physical_modes", type_pb2.PHYSICAL_MODE,
                               request, instance)

    def commercial_modes(self, request, instance):
        return self.__on_ptref("commercial_modes", type_pb2.COMMERCIAL_MODE,
                               request, instance)

    def connections(self, request, instance):
        return self.__on_ptref("connections", type_pb2.CONNECTION, request,
                               instance)

    def journey_pattern_points(self, request, instance):
        return self.__on_ptref("journey_pattern_points",
                               type_pb2.JOURNEY_PATTERN_POINT, request,
                               instance)

    def journey_patterns(self, request, instance):
        return self.__on_ptref("journey_patterns", type_pb2.JOURNEY_PATTERN,
                               request, instance)

    def companies(self, request, instance):
        return self.__on_ptref("companies", type_pb2.COMPANY, request,
                               instance)

    def vehicle_journeys(self, request, instance):
        return self.__on_ptref("vehicle_journeys", type_pb2.VEHICLE_JOURNEY,
                               request, instance)

    def pois(self, request, instance):
        return self.__on_ptref("pois", type_pb2.POI, request, instance)

    def poi_types(self, request, instance):
        return self.__on_ptref("poi_types", type_pb2.POITYPE, request,
                               instance)

    def calendars(self, request, instance):
        return self.__on_ptref("calendars", type_pb2.CALENDAR,
                               request, instance)
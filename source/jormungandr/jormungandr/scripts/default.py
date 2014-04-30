# coding=utf-8

# Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Canal TP (www.canaltp.fr).
# Help us simplify mobility and open public transport:
#     a non ending quest to the responsive locomotion way of traveling!
#
# LICENCE: This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
# Stay tuned using
# twitter @navitia
# IRC #navitia on freenode
# https://groups.google.com/d/forum/navitia
# www.navitia.io

import copy
import navitiacommon.type_pb2 as type_pb2
import navitiacommon.request_pb2 as request_pb2
import navitiacommon.response_pb2 as response_pb2
from jormungandr.interfaces.v1.Journeys import journey
from jormungandr.renderers import render_from_protobuf
from qualifier import qualifier_one
from datetime import datetime, timedelta
import itertools
from flask import current_app

pb_type = {
    'stop_area': type_pb2.STOP_AREA,
    'stop_point': type_pb2.STOP_POINT,
    'address': type_pb2.ADDRESS,
    'poi': type_pb2.POI,
    'administrative_region': type_pb2.ADMINISTRATIVE_REGION,
    'line': type_pb2.LINE
}

f_date_time = "%Y%m%dT%H%M%S"


def are_equals(journey1, journey2):
    """
    To decide that 2 journeys are equals, we loop through all values of the
    compare_journey_generator and stop at the first non equals value

    Note: the fillvalue is the value used when a generator is consumed
    (if the 2 generators does not return the same number of elt).
    by setting it to object(), we ensure that it will be !=
    from any values returned by the other generator
    """
    return all(a == b for a, b in itertools.izip_longest(compare_journey_generator(journey1),
                                                         compare_journey_generator(journey2),
                                                         fillvalue=object()))


def compare_journey_generator(journey):
    """
    Generator used to compare journeys together

    2 journeys are equals if they share for all the sections the same :
     * departure time
     * arrival time
     * vj
     * type
     * departure location
     * arrival location
    """
    yield journey.departure_date_time
    yield journey.arrival_date_time
    for s in journey.sections:
        yield s.type
        yield s.begin_date_time
        yield s.end_date_time
        yield s.vehicle_journey.uri if s.vehicle_journey else 'no_vj'
        #NOTE: we want to ensure that we always yield the same number of elt
        yield s.origin.uri if s.origin else 'no_origin'
        yield s.destination.uri if s.destination else 'no_destination'


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

    def calendars(self, request, instance):
        req = request_pb2.Request()
        req.requested_api = type_pb2.calendars
        req.calendars.depth = request['depth']
        req.calendars.filter = request['filter']
        req.calendars.count = request['count']
        req.calendars.start_page = request['start_page']
        req.calendars.start_date = request['start_date']
        req.calendars.end_date = request['end_date']
        if request["forbidden_uris[]"]:
            for forbidden_uri in request["forbidden_uris[]"]:
                req.ptref.forbidden_uri.append(forbidden_uri)

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
        st.show_codes = request["show_codes"]
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
        if "calendar" in request and request["calendar"]:
            st.calendar = request["calendar"]
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
        req.journeys.disruption_active = request["disruption_active"]
        req.journeys.show_codes = request["show_codes"]

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
        """
        Check if some particular journeys are missing, and return:
                if it is the case a modified version of the request to be rerun
                else None
        """
        if not "cheap_journey" in self.functional_params \
            or self.functional_params["cheap_journey"].lower() != "true":
            return (None, None)

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
            return (None, None)

        req = copy.deepcopy(initial_request)
        for uri in ter_uris:
            req.journeys.forbidden_uris.append(uri)

        return (req, "possible_cheap")

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
        if not resp:
            return
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

    def call_kraken(self, req, instance, tag=None):
        resp = None

        """
            for all combinaison of departure and arrival mode we call kraken
        """
        # filter walking if bss in mode ?
        for o_mode, d_mode in itertools.product(
                self.origin_modes, self.destination_modes):
            req.journeys.streetnetwork_params.origin_mode = o_mode
            req.journeys.streetnetwork_params.destination_mode = d_mode
            local_resp = instance.send_and_receive(req)

            if local_resp.response_type == response_pb2.ITINERARY_FOUND:

                # if a specific tag was provided, we tag the journeys
                # and we don't call the qualifier, it will be done after
                # with the journeys from the previous query
                if tag:
                    for j in local_resp.journeys:
                        j.type = tag
                else:
                    #we qualify the journeys
                    request_type = "arrival" if req.journeys.clockwise else "departure"
                    qualifier_one(local_resp.journeys, request_type)

                if not resp:
                    resp = local_resp
                else:
                    self.merge_response(resp, local_resp)
            if not resp:
                resp = local_resp
            current_app.logger.debug("for mode {}|{} we have found {} journeys".format(o_mode, d_mode, len(local_resp.journeys)))

        self.__fill_uris(resp)
        return resp

    def get_journey(self, pb_req, instance, original_request):
        resp = self.call_kraken(pb_req, instance)

        if not resp or (pb_req.requested_api != type_pb2.PLANNER and pb_req.requested_api != type_pb2.ISOCHRONE):
            return

        new_request, tag = self.check_missing_journey(resp.journeys, pb_req)

        if new_request:
            #we have to call kraken again with a modified version of the request
            new_resp = self.call_kraken(new_request, instance, tag)

            self.merge_response(resp, new_resp)

            # we qualify the journeys with the previous one
            # because we need all journeys to qualify them correctly
            request_type = "arrival" if new_request.journeys.clockwise else "departure"
            qualifier_one(resp.journeys, request_type)

        #we filter the journeys
        self.delete_journeys(resp, original_request)
        return resp

    def journey_compare(self, j1, j2):
        if j1.arrival_date_time != j2.arrival_date_time:
            return -1 if j1.arrival_date_time < j2.arrival_date_time else 1

        if j1.duration != j2.duration:
            return j1.duration - j2.duration

        if j1.nb_transfers != j2.nb_transfers:
            return j1.nb_transfers - j2.nb_transfers

        non_pt_duration_j1 = non_pt_duration_j2 = None
        for journey in [j1, j2]:
            non_pt_duration = 0
            for section in journey.sections:
                if section.type != response_pb2.PUBLIC_TRANSPORT:
                    non_pt_duration += section.duration
            if not non_pt_duration_j1:
                non_pt_duration_j1 = non_pt_duration
            else:
                non_pt_duration_j2 = non_pt_duration
        return non_pt_duration_j1 - non_pt_duration_j2

    def fill_journeys(self, pb_req, request, instance):
        """
        call kraken to get the requested number of journeys

        It more journeys are wanted, we ask for the next (or previous if not clockwise) departures
        """
        resp = self.get_journey(pb_req, instance, request)

        if not resp.journeys or len(resp.journeys) == 0:
            return resp  # no journeys found, useless to call kraken again

        last_best = next((j for j in resp.journeys if j.type == 'rapid'), None)
        cpt_attempt = 1  # we limit the number of call to at most the number of wanted journeys
        while request["min_nb_journeys"] > len(resp.journeys) and cpt_attempt <= request["min_nb_journeys"]:
            if not last_best:  # if no rapid journey has been found, no need to continue
                break
            cpt_attempt += 1

            new_datetime = None
            if request['clockwise']:
                l_date_time = last_best.departure_date_time
                l_date_time_f = datetime.strptime(l_date_time, f_date_time)
                new_datetime = l_date_time_f + timedelta(minutes=1)
            else:
                l_date_time = last_best.arrival_date_time
                l_date_time_f = datetime.strptime(l_date_time, f_date_time)
                new_datetime = l_date_time_f + timedelta(minutes=-1)

            # we copy the query not to have side effects
            next_request = copy.deepcopy(pb_req)
            next_request.journeys.datetimes[0] = new_datetime.strftime(f_date_time)
            del next_request.journeys.datetimes[1:]
            tmp_resp = self.get_journey(next_request, instance, request)

            if len(tmp_resp.journeys) == 0:
                break  #no more journeys found, we stop

            # we tag the journeys as 'next' or 'prev' journeys
            for j in tmp_resp.journeys:
                j.tags.append("next" if request['clockwise'] else "prev")

            last_best = next((j for j in tmp_resp.journeys if j.type == 'rapid'), None)

            self.merge_response(resp, tmp_resp)

        self.choose_best(resp)

        self.delete_journeys(resp, request)  # last filter
        self.sort_journeys(resp)
        return resp

    def choose_best(self, resp):
        """
        the best journey, is the first rapid, ie the rapid without additional tags ('prev' or 'next')
        """
        for j in resp.journeys:
            if j.type == 'rapid' and not j.tags:
                j.type = 'best'
                break  # we want to ensure the unicity of the best

    def merge_response(self, initial_response, new_response):
        #since it's not the first call to kraken, some kraken's id
        #might not be uniq anymore
        self.change_ids(new_response, len(initial_response.journeys))
        if len(new_response.journeys) == 0:
            return

        #we don't want to add a journey already there
        tickets_to_add = set()
        for new_j in new_response.journeys:

            if any(are_equals(new_j, old_j) for old_j in initial_response.journeys):
                current_app.logger.debug("the journey tag={}, departure={} "
                                         "was already there, we filter it".format(new_j.type, new_j.departure_date_time))
                continue  # already there, we don't want to add it

            initial_response.journeys.extend([new_j])
            for t in new_j.fare.ticket_id:
                tickets_to_add.add(t)

        # we have to add the additional fares too
        # if at least one journey has the ticket we add it
        initial_response.tickets.extend([t for t in new_response.tickets if t.id in tickets_to_add])

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
            tag_to_delete = ["", "possible_cheap"]
            to_delete.extend([idx for idx, j in enumerate(resp.journeys) if j.type in tag_to_delete])

        #we want to keep only one non pt (the first one)
        for tag in ["non_pt_bss", "non_pt_walk", "non_pt_bike"]:
            found_direct = False
            for idx, j in enumerate(resp.journeys):
                if j.type != tag:
                    continue
                if found_direct:
                    to_delete.append(idx)
                else:
                    found_direct = True

        # list comprehension does not work with repeated field, so we have to delete them manually
        to_delete.sort(reverse=True)
        for idx in to_delete:
            del resp.journeys[idx]

        #after all filters, we filter not to give too many results
        if request["max_nb_journeys"] and len(resp.journeys) > request["max_nb_journeys"]:
            del resp.journeys[request["max_nb_journeys"]:]

    def sort_journeys(self, resp):
        if len(resp.journeys) > 0:
            resp.journeys.sort(self.journey_compare)

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
        req.ptref.show_codes = request["show_codes"]
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

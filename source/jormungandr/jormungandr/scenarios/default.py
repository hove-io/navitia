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
from jormungandr.renderers import render_from_protobuf
from jormungandr.interfaces.common import pb_odt_level
from qualifier import qualifier_one
from datetime import datetime, timedelta
import itertools
from flask import current_app
import time
from jormungandr.scenarios.utils import pb_type, pt_object_type, are_equals, count_typed_journeys, journey_sorter
from jormungandr.scenarios import simple
import logging


class Scenario(simple.Scenario):

    def parse_journey_request(self, requested_type, request):
        """Parse the request dict and create the protobuf version"""
        req = request_pb2.Request()
        req.requested_api = requested_type
        if "origin" in request and request["origin"]:
            if requested_type != type_pb2.NMPLANNER:
                origins = ([request["origin"]], [0])
            else:
                origins = (request["origin"], request["origin_access_duration"])
            for i in range(0, len(origins[0])):
                location = req.journeys.origin.add()
                location.place = origins[0][i]
                location.access_duration = origins[1][i]
        if "destination" in request and request["destination"]:
            if requested_type != type_pb2.NMPLANNER:
                destinations = ([request["destination"]], [0])
            else:
                destinations = (request["destination"], request["destination_access_duration"])
            for i in range(0, len(destinations[0])):
                location = req.journeys.destination.add()
                location.place = destinations[0][i]
                location.access_duration = destinations[1][i]
            self.destination_modes = request["destination_mode"]
        else:
            self.destination_modes = ["walking"]
        if "datetime" in request and request["datetime"]:
            if isinstance(request["datetime"], int):
                request["datetime"] = [request["datetime"]]
            for dte in request["datetime"]:
                req.journeys.datetimes.append(dte)
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
        if "details" in request and request["details"]:
            req.journeys.details = request["details"]

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
        logger = logging.getLogger(__name__)
        # filter walking if bss in mode ?
        for o_mode, d_mode in itertools.product(self.origin_modes, self.destination_modes):
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
            logger.debug("for mode %s|%s we have found %s journeys: %s", o_mode, d_mode, len(local_resp.journeys), [j.type for j in local_resp.journeys])

        self.__fill_uris(resp)
        return resp

    def change_request(self, pb_req, resp):
        result = copy.deepcopy(pb_req)
        def get_uri_odt_with_zones(journey):
            result = []
            for section in journey.sections:
                if section.type == response_pb2.PUBLIC_TRANSPORT:
                    if section.pt_display_informations.vehicle_journey_type in [type_pb2.virtual_without_stop_time,
                                                                                type_pb2.stop_point_to_stop_point,
                                                                                type_pb2.address_to_stop_point,
                                                                                type_pb2.odt_point_to_point]:
                        result.append(section.uris.line)
                    else:
                        return []
            return result

        map_forbidden_uris = map(get_uri_odt_with_zones, resp.journeys)
        for forbidden_uris in map_forbidden_uris:
            for line_uri in forbidden_uris:
                if line_uri not in result.journeys.forbidden_uris:
                    result.journeys.forbidden_uris.append(line_uri)
        return result

    def fill_journeys(self, pb_req, request, instance):
        """
        call kraken to get the requested number of journeys

        It more journeys are wanted, we ask for the next (or previous if not clockwise) departures
        """
        resp = response_pb2.Response()
        next_request = copy.deepcopy(pb_req)
        nb_typed_journeys = 0
        cpt_attempt = 0
        max_attempts = 2 if not request["min_nb_journeys"] else request["min_nb_journeys"]*2
        at_least_one_journey_found = False
        while ((request["min_nb_journeys"] and request["min_nb_journeys"] > nb_typed_journeys) or\
            (not request["min_nb_journeys"] and nb_typed_journeys == 0)) and cpt_attempt < max_attempts:
            tmp_resp = self.call_kraken(next_request, instance)
            if len(tmp_resp.journeys) == 0:
                # if we do not yet have journeys, we get the tmp_resp to have the error if there are some
                if len(resp.journeys) == 0:
                    resp = tmp_resp
                break
            at_least_one_journey_found = True
            last_best = next((j for j in tmp_resp.journeys if j.type == 'rapid'), None)

            new_datetime = None
            one_minute = 60
            if request['clockwise']:
                #since dates are now posix time stamp, we only have to add the additional seconds
                if not last_best:
                    last_best = min(tmp_resp.journeys, key=lambda j: j.departure_date_time)
                    #In this case there is no journeys, so we stop
                    if not last_best:
                        break
                new_datetime = last_best.departure_date_time + one_minute
            else:
                if not last_best:
                    last_best = max(tmp_resp.journeys, key=lambda j: j.arrival_date_time)
                    #In this case there is no journeys, so we stop
                    if not last_best:
                        break
                new_datetime = last_best.arrival_date_time - one_minute

            next_request = self.change_request(pb_req, tmp_resp)
            next_request.journeys.datetimes[0] = new_datetime
            del next_request.journeys.datetimes[1:]
            # we tag the journeys as 'next' or 'prev' journey
            if len(resp.journeys):
                for j in tmp_resp.journeys:
                    j.tags.append("next" if request['clockwise'] else "prev")

            self.delete_journeys(tmp_resp, request, final_filter=False)
            self.merge_response(resp, tmp_resp)

            nb_typed_journeys = count_typed_journeys(resp.journeys)
            cpt_attempt += 1

        self.sort_journeys(resp, instance.journey_order, request['clockwise'])
        self.choose_best(resp)
        self.delete_journeys(resp, request, final_filter=True)  # filter one last time to remove similar journeys

        if len(resp.journeys) == 0 and at_least_one_journey_found and not resp.HasField("error"):
            error = resp.error
            error.id = response_pb2.Error.no_solution
            error.message = "No journey found, all were filtered"

        return resp

    def choose_best(self, resp):
        """
        prerequisite: Journeys has to be sorted before
        the best journey is the first rapid
        """
        for j in resp.journeys:
            if j.type == 'rapid':
                j.type = 'best'
                break  # we want to ensure the unicity of the best

    def merge_response(self, initial_response, new_response):
        #since it's not the first call to kraken, some kraken's id
        #might not be uniq anymore
        logger = logging.getLogger(__name__)
        self.change_ids(new_response, len(initial_response.journeys))
        if len(new_response.journeys) == 0:
            return

        #we don't want to add a journey already there
        tickets_to_add = set()
        for new_j in new_response.journeys:

            if any(are_equals(new_j, old_j) for old_j in initial_response.journeys):
                logger.debug("the journey tag={}, departure={} "
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
            ticket.id = ticket.id + '_' + str(journey_count)
            for i in range(len(ticket.section_id)):
                ticket.section_id[i] = ticket.section_id[i] + '_' + str(journey_count)

        for new_journey in new_journeys.journeys:
            for i in range(len(new_journey.fare.ticket_id)):
                new_journey.fare.ticket_id[i] = new_journey.fare.ticket_id[i] \
                                                + '_' + str(journey_count)

            for section in new_journey.sections:
                section.id = section.id + '_' + str(journey_count)

    def delete_journeys(self, resp, request, final_filter):
        """
        the final_filter filter the 'best' journey (it can't be done before since the best is chosen at the end)
        and it filter for the max wanted journey (we don't want that filter before the journeys are sorted)
        """
        if request["debug"] or not resp:
            return #in debug we want to keep all journeys

        if "destination" not in request or not request['destination']:
            return #for isochrone we don't want to filter

        if resp.HasField("error"):
            return #we don't filter anything if errors

        #filter on journey type (the qualifier)
        to_delete = []
        if request["type"] != "" and request["type"] != "all":
            #quick fix, if we want the rapid, we filter also the 'best' which is a rapid
            filter_type = [request["type"]] if request["type"] != "rapid" else ["rapid", "best"]
            #the best journey can only be filtered at the end. so we might want to filter anything but this journey
            if request["type"] != "best" or final_filter:
                to_delete.extend([idx for idx, j in enumerate(resp.journeys) if j.type not in filter_type])
        else:
            #by default, we filter non tagged journeys
            tag_to_delete = [""]
            to_delete.extend([idx for idx, j in enumerate(resp.journeys) if j.type in tag_to_delete])

        #we want to keep only one non pt (the first one)
        for tag in ["non_pt_bss", "non_pt_walk", "non_pt_bike", "car"]:
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

        if final_filter:
            #after all filters, we filter not to give too many results
            if request["max_nb_journeys"] and len(resp.journeys) > request["max_nb_journeys"]:
                del resp.journeys[request["max_nb_journeys"]:]

    def sort_journeys(self, resp, journey_order, clockwise=True):
        if len(resp.journeys) > 1:
            resp.journeys.sort(journey_sorter[journey_order](clockwise=clockwise))

    def __on_journeys(self, requested_type, request, instance):
        req = self.parse_journey_request(requested_type, request)

        # call to kraken
        resp = self.fill_journeys(req, request, instance)
        return resp

    def journeys(self, request, instance):
        return self.__on_journeys(type_pb2.PLANNER, request, instance)

    def nm_journeys(self, request, instance):
        return self.__on_journeys(type_pb2.NMPLANNER, request, instance)

    def isochrone(self, request, instance):
        return self.__on_journeys(type_pb2.ISOCHRONE, request, instance)

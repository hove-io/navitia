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

from __future__ import absolute_import, print_function, unicode_literals, division

import copy
import navitiacommon.type_pb2 as type_pb2
import navitiacommon.request_pb2 as request_pb2
import navitiacommon.response_pb2 as response_pb2
from jormungandr.scenarios.qualifier import qualifier_one
import itertools
from flask import current_app
from jormungandr.scenarios.utils import are_equals, count_typed_journeys, journey_sorter, \
    change_ids, updated_request_with_default, fill_uris, get_or_default
from jormungandr.scenarios import simple
import logging
from jormungandr.scenarios.helpers import bss_duration, bike_duration, car_duration, pt_duration
from jormungandr.scenarios.helpers import select_best_journey_by_time, select_best_journey_by_duration, \
    max_duration_fallback_modes
from jormungandr.scenarios.helpers import fallback_mode_comparator
from jormungandr.utils import pb_del_if, date_to_timestamp
import flask
import gevent, gevent.pool

non_pt_types = ['non_pt_walk', 'non_pt_bike', 'non_pt_bss']

def is_admin(entrypoint):
    return entrypoint.startswith('admin:')

class Scenario(simple.Scenario):

    def parse_journey_request(self, requested_type, request):
        """Parse the request dict and create the protobuf version"""
        req = request_pb2.Request()
        req.requested_api = requested_type
        req._current_datetime = date_to_timestamp(request["_current_datetime"])
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
        sn_params.max_walking_duration_to_pt = request["max_walking_duration_to_pt"]
        sn_params.max_bike_duration_to_pt = request["max_bike_duration_to_pt"]
        sn_params.max_bss_duration_to_pt = request["max_bss_duration_to_pt"]
        sn_params.max_car_duration_to_pt = request["max_car_duration_to_pt"]
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
        if request["max_extra_second_pass"]:
            req.journeys.max_extra_second_pass = request["max_extra_second_pass"]
        req.journeys.wheelchair = request["wheelchair"] or False  # default value is no wheelchair

        if request['data_freshness'] == 'realtime':
            req.journeys.realtime_level = type_pb2.REALTIME
        elif request['data_freshness'] == 'adapted_schedule':
            req.journeys.realtime_level = type_pb2.ADAPTED_SCHEDULE
        else:
            req.journeys.realtime_level = type_pb2.BASE_SCHEDULE

        if "details" in request and request["details"]:
            req.journeys.details = request["details"]

        req.journeys.walking_transfer_penalty = request['_walking_transfer_penalty']

        self.origin_modes = request["origin_mode"]

        if req.journeys.streetnetwork_params.origin_mode == "bike_rental":
            req.journeys.streetnetwork_params.origin_mode = "bss"
        if req.journeys.streetnetwork_params.destination_mode == "bike_rental":
            req.journeys.streetnetwork_params.destination_mode = "bss"
        for forbidden_uri in get_or_default(request, "forbidden_uris[]", []):
            req.journeys.forbidden_uris.append(forbidden_uri)
        for allowed_id in get_or_default(request, "allowed_id[]", []):
            req.journeys.allowed_id.append(allowed_id)
        if not "type" in request:
            request["type"] = "all" #why ?

        #for the default scenario, we filter the walking if we have walking + bss

        # Technically, bss mode enable walking (if it is better than bss)
        # so if the user ask for walking and bss, we only keep bss
        for fallback_modes in self.origin_modes, self.destination_modes:
            if 'walking' in fallback_modes and 'bss' in fallback_modes:
                fallback_modes.remove('walking')

        return req

    def call_kraken(self, req, instance, tag=None):
        resp = None

        """
            for all combinaison of departure and arrival mode we call kraken
        """
        logger = logging.getLogger(__name__)
        futures = []

        def worker(o_mode, d_mode, instance, request, request_id):
            return (o_mode, d_mode, instance.send_and_receive(request, request_id=request_id))

        pool = gevent.pool.Pool(current_app.config.get('GREENLET_POOL_SIZE', 3))
        for o_mode, d_mode in itertools.product(self.origin_modes, self.destination_modes):
            #since we use multiple green thread we have to copy the request
            local_req = copy.deepcopy(req)
            local_req.journeys.streetnetwork_params.origin_mode = o_mode
            local_req.journeys.streetnetwork_params.destination_mode = d_mode
            if o_mode == 'car' or (is_admin(req.journeys.origin[0].place) and is_admin(req.journeys.destination[0].place)):
                # we don't want direct path for car or for admin to admin journeys
                req.journeys.streetnetwork_params.enable_direct_path = False
            else:
                req.journeys.streetnetwork_params.enable_direct_path = True
            futures.append(pool.spawn(worker, o_mode, d_mode, instance, local_req, request_id=flask.request.id))

        for future in gevent.iwait(futures):
            o_mode, d_mode, local_resp = future.get()
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

                fill_uris(local_resp)
                if not resp:
                    resp = local_resp
                else:
                    self.merge_response(resp, local_resp)
            if not resp:
                resp = local_resp
            logger.debug("for mode %s|%s we have found %s journeys: %s", o_mode, d_mode, len(local_resp.journeys), [j.type for j in local_resp.journeys])

        return resp

    def change_request(self, pb_req, resp, forbidden_uris=[]):
        result = copy.deepcopy(pb_req)
        def get_uri_odt_with_zones(journey):
            result = []
            for section in journey.sections:
                if section.type == response_pb2.PUBLIC_TRANSPORT:
                    tags = section.additional_informations
                    if response_pb2.ODT_WITH_ZONE in tags or \
                       response_pb2.ODT_WITH_STOP_POINT in tags:
                        result.append(section.uris.line)
                    else:
                        return []
            return result

        map_forbidden_uris = map(get_uri_odt_with_zones, resp.journeys)
        for uris in map_forbidden_uris:
            for line_uri in uris:
                if line_uri not in result.journeys.forbidden_uris:
                    forbidden_uris.append(line_uri)
        for forbidden_uri in forbidden_uris:
            result.journeys.forbidden_uris.append(forbidden_uri)


        return result, forbidden_uris

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
        forbidden_uris = []

        #call to kraken must be done with fallback mode in the right order:
        #walking, then bss, then bike, and finally car
        #in some case a car journey can be equivalent to a walking journey, typically when crowfly are used
        #and only the first one is kept, and we want the walking one to be kept!
        self.origin_modes.sort(fallback_mode_comparator)
        self.destination_modes.sort(fallback_mode_comparator)

        while ((request["min_nb_journeys"] and request["min_nb_journeys"] > nb_typed_journeys) or\
            (not request["min_nb_journeys"] and nb_typed_journeys == 0)) and cpt_attempt < max_attempts:
            tmp_resp = self.call_kraken(next_request, instance)
            if len(tmp_resp.journeys) == 0:
                # if we do not yet have journeys, we get the tmp_resp to have the error if there are some
                if len(resp.journeys) == 0:
                    resp = tmp_resp
                break
            at_least_one_journey_found = True

            if request['clockwise']:
                new_datetime = self.next_journey_datetime(tmp_resp.journeys, request['clockwise'])
            else:
                new_datetime = self.previous_journey_datetime(tmp_resp.journeys, request['clockwise'])

            if new_datetime is None:
                break

            next_request, forbidden_uris = self.change_request(pb_req, tmp_resp, forbidden_uris)
            next_request.journeys.datetimes[0] = new_datetime
            del next_request.journeys.datetimes[1:]
            # we tag the journeys as 'next' or 'prev' journey
            if len(resp.journeys):
                for j in tmp_resp.journeys:
                    j.tags.append("next" if request['clockwise'] else "prev")

            self.merge_response(resp, tmp_resp)
            #we delete after the merge, else we will have duplicate non_pt journey in the count
            self.delete_journeys(resp, request, final_filter=False)

            nb_typed_journeys = count_typed_journeys(resp.journeys)
            cpt_attempt += 1

        if not request['debug']:
            self._remove_not_long_enough_fallback(resp, instance)
            self._remove_not_long_enough_tc_with_fallback(resp, instance)
            self._delete_too_long_journey(resp, instance, request['clockwise'])
        self.sort_journeys(resp, instance.journey_order, request['clockwise'])
        self.choose_best(resp)
        self.delete_journeys(resp, request, final_filter=True)  # filter one last time to remove similar journeys

        if len(resp.journeys) == 0 and at_least_one_journey_found and not resp.HasField(str("error")):
            error = resp.error
            error.id = response_pb2.Error.no_solution
            error.message = "No journey found, all were filtered"

        self._compute_pagination_links(resp, instance, request['clockwise'])

        return resp

    def _delete_too_long_journey(self, resp, instance, clockwise):
        """
        remove journey a lot longer than the asap journey
        """
        logger = logging.getLogger(__name__)
        max_duration = self._find_max_duration(resp.journeys, instance, clockwise)
        if not max_duration:
            return
        to_delete = set()
        for idx, journey in enumerate(resp.journeys):
            if journey.duration > max_duration:
                to_delete.add(idx)
                logger.debug('delete journey %s because it is longer than %s', journey.type, max_duration)
        self.erase_journeys(resp, to_delete)

    @staticmethod
    def next_journey_datetime(journeys, clockwise):
        """
        by default to get the next journey, we add one minute to:
        the best if we have one, else to the journey that has the earliest departure
        """
        if not journeys:
            return None
        last_best = next((j for j in journeys if j.type in ('best', 'rapid')), None) or \
                    min(journeys, key=lambda j: j.departure_date_time)

        if not last_best:
            return None

        one_minute = 60
        #since dates are now posix time stamp, we only have to add the additional seconds
        return last_best.departure_date_time + one_minute

    @staticmethod
    def previous_journey_datetime(journeys, clockwise):
        """
        by default to get the previous journey, we substract one minute to:
        the best if we have one, else to the journey that has the tardiest arrival
        """
        if not journeys:
            return None
        last_best = next((j for j in journeys if j.type in ('best', 'rapid')), None) or \
                    max(journeys, key=lambda j: j.arrival_date_time)

        if not last_best:
            return None

        one_minute = 60
        #since dates are now posix time stamp, we only have to add the additional seconds
        return last_best.arrival_date_time - one_minute

    def _find_max_duration(self, journeys, instance, clockwise):
        """
        Calculate the max duration of a journey from a pool of journey.
        We can search the earliest one (by default) or the shortest one.
        Restriction are put on the fallback mode used, by default only walking is allowed.
        """
        if not journeys:
            return None
        criteria = {'time': select_best_journey_by_time, 'duration': select_best_journey_by_duration}
        fallback_modes = max_duration_fallback_modes[instance.max_duration_fallback_mode]
        asap_journey = criteria[instance.max_duration_criteria](journeys, clockwise, fallback_modes)
        if not asap_journey:
            return None
        return (asap_journey.duration * instance.factor_too_long_journey) + instance.min_duration_too_long_journey

    def choose_best(self, resp):
        """
        prerequisite: Journeys has to be sorted before
        the best journey is the first rapid
        """
        for j in resp.journeys:
            if j.type == 'rapid':
                j.type = 'best'
                break # we want to ensure the unicity of the best

    def merge_response(self, initial_response, new_response):
        #since it's not the first call to kraken, some kraken's id
        #might not be uniq anymore
        logger = logging.getLogger(__name__)
        change_ids(new_response, len(initial_response.journeys))
        if len(new_response.journeys) == 0:
            return

        #if the initial response was an error we remove the error since we have result now
        if initial_response.HasField(str('error')):
            initial_response.ClearField(str('error'))

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

        initial_feed_publishers = {}
        for fp in initial_response.feed_publishers:
            initial_feed_publishers[fp.id] = fp
        initial_response.feed_publishers.extend([fp for fp in new_response.feed_publishers if fp.id not in initial_feed_publishers])

        # handle impacts
        for i in new_response.impacts:
            if any(other.uri == i.uri for other in initial_response.impacts):
                continue
            initial_response.impacts.extend([i])

    def erase_journeys(self, resp, to_delete):
        """
        remove a list of journeys from a response and delete all referenced objects like by example the tickets
        """
        deleted_sections = set()
        for idx in sorted(to_delete, reverse=True):
            for section in resp.journeys[idx].sections:
                deleted_sections.add(section.id)
            del resp.journeys[idx]

        pb_del_if(resp.tickets, lambda t: set(t.section_id).issubset(deleted_sections))

    def delete_journeys(self, resp, request, final_filter):
        """
        the final_filter filter the 'best' journey (it can't be done before since the best is chosen at the end)
        and it filter for the max wanted journey (we don't want that filter before the journeys are sorted)
        """
        if request["debug"] or not resp:
            return #in debug we want to keep all journeys

        if "destination" not in request or not request['destination']:
            return #for isochrone we don't want to filter

        if "clockwise" not in request or request['clockwise']:
            if "destination" in request and isinstance(request['destination'], list) \
                                        and len(request['destination']) > 1:
                return #for n-m calculation we don't want to filter

        if "clockwise" in request and not request['clockwise']:
            if "origin" in request and isinstance(request['origin'], list) \
                                   and len(request['origin']) > 1:
                return #for n-m calculation we don't want to filter

        if resp.HasField(str("error")):
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
        self.erase_journeys(resp, to_delete)

        if final_filter:
            #after all filters, we filter not to give too many results
            if request["max_nb_journeys"] and len(resp.journeys) > request["max_nb_journeys"]:
                del resp.journeys[request["max_nb_journeys"]:]

    def sort_journeys(self, resp, journey_order, clockwise=True):
        if len(resp.journeys) > 1:
            resp.journeys.sort(journey_sorter[journey_order](clockwise=clockwise))

    def __on_journeys(self, requested_type, request, instance):
        updated_request_with_default(request, instance)
        req = self.parse_journey_request(requested_type, request)

        # call to kraken
        resp = self.fill_journeys(req, request, instance)
        return resp

    def journeys(self, request, instance):
        return self.__on_journeys(type_pb2.PLANNER, request, instance)

    def isochrone(self, request, instance):
        updated_request_with_default(request, instance)
        req = self.parse_journey_request(type_pb2.ISOCHRONE, request)

        # call to kraken
        # TODO: check mode size
        req.journeys.streetnetwork_params.origin_mode = self.origin_modes[0]
        req.journeys.streetnetwork_params.destination_mode = self.destination_modes[0]
        resp = instance.send_and_receive(req)

        if not request['debug']:
            # on isochrone we can filter the number of max journeys
            if request["max_nb_journeys"] and len(resp.journeys) > request["max_nb_journeys"]:
                del resp.journeys[request["max_nb_journeys"]:]

        return resp

    def _remove_not_long_enough_fallback(self, resp, instance):
        to_delete = []
        for idx, journey in enumerate(resp.journeys):
            if journey.type in non_pt_types:
                continue
            bike_dur = bike_duration(journey)
            car_dur = car_duration(journey)
            bss_dur = bss_duration(journey)
            if bike_dur and bike_dur < instance.min_bike:
                to_delete.append(idx)
            elif bss_dur and bss_dur < instance.min_bss:
                to_delete.append(idx)
            elif car_dur and car_dur < instance.min_car:
                to_delete.append(idx)

        logger = logging.getLogger(__name__)
        logger.debug('remove %s journey with not enough fallback duration: %s',
                len(to_delete), [resp.journeys[i].type for i in to_delete])
        self.erase_journeys(resp, to_delete)

    def _remove_not_long_enough_tc_with_fallback(self, resp, instance):
        to_delete = []
        for idx, journey in enumerate(resp.journeys):
            if journey.type in non_pt_types:
                continue
            bike_dur = bike_duration(journey)
            car_dur = car_duration(journey)
            tc_dur = pt_duration(journey)
            bss_dur = bss_duration(journey)
            if bike_dur and tc_dur < instance.min_tc_with_bike:
                to_delete.append(idx)
            elif bss_dur and tc_dur < instance.min_tc_with_bss:
                to_delete.append(idx)
            elif car_dur and tc_dur < instance.min_tc_with_car:
                to_delete.append(idx)

        logger = logging.getLogger(__name__)
        logger.debug('remove %s journey with not enough tc duration: %s',
                len(to_delete), [resp.journeys[i].type for i in to_delete])
        self.erase_journeys(resp, to_delete)

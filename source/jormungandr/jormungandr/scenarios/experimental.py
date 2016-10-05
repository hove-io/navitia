# Copyright (c) 2001-2015, Canal TP and/or its affiliates. All rights reserved.
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
import logging
from flask.ext.restful import abort
from jormungandr.scenarios import new_default
from navitiacommon import type_pb2, response_pb2, request_pb2
import uuid
from jormungandr.scenarios.utils import fill_uris
from jormungandr.planner import JourneyParameters
from flask import g
from jormungandr.utils import get_uri_pt_object
from jormungandr import app


def create_crowfly(_from, to, begin, end, mode='walking'):
    section = response_pb2.Section()
    section.type = response_pb2.CROW_FLY
    section.origin.CopyFrom(_from)
    section.destination.CopyFrom(to)
    section.duration = end-begin
    section.begin_date_time = begin
    section.end_date_time = end
    section.street_network.mode = response_pb2.Walking
    section.id = unicode(uuid.uuid4())
    return section


class SectionSorter(object):
    def __call__(self, a, b):
        if a.begin_date_time != b.begin_date_time:
            return -1 if a.begin_date_time < b.begin_date_time else 1
        else:
            return -1 if a.end_date_time < b.end_date_time else 1


def get_max_fallback_duration(request, mode):
    if mode == 'walking':
        return request['max_walking_duration_to_pt']
    if mode == 'bss':
        return request['max_bss_duration_to_pt']
    if mode == 'bike':
        return request['max_bike_duration_to_pt']
    if mode == 'car':
        return request['max_car_duration_to_pt']
    raise ValueError('unknown mode: {}'.format(mode))


#TODO: make this work, it's dynamically imported, so the function is register too late
#@app.before_request
def _init_g():
    g.origins_fallback = {}
    g.destinations_fallback = {}
    g.fallback_direct_path = {}
    g.requested_origin = None
    g.requested_destination = None


def create_parameters(request):
    return JourneyParameters(max_duration=request['max_duration'],
                             max_transfers=request['max_transfers'],
                             wheelchair=request['wheelchair'] or False,
                             realtime_level=request['data_freshness'],
                             max_extra_second_pass=request['max_extra_second_pass'],
                             walking_transfer_penalty=request['_walking_transfer_penalty'],
                             forbidden_uris=request['forbidden_uris[]'])


def _update_crowfly_duration(instance, fallback_list, mode, stop_area_uri, crow_fly_stop_points):
    stop_points = instance.georef.get_stop_points_for_stop_area(stop_area_uri)
    for stop_point in stop_points:
        if mode in fallback_list:
            fallback_list[mode][stop_point.uri] = 0
            crow_fly_stop_points.add(stop_point.uri)


def _rename_journey_sections_ids(start_idx, sections):
    for s in sections:
        s.id = "dp_section_{}".format(start_idx)
        start_idx += 1


def _extend_pt_sections_with_direct_path(pt_journey, dp_journey):
    if getattr(dp_journey, 'journeys', []) and hasattr(dp_journey.journeys[0], 'sections'):
        _rename_journey_sections_ids(len(pt_journey.sections), dp_journey.journeys[0].sections)
        pt_journey.sections.extend(dp_journey.journeys[0].sections)


def _reverse_journeys(res):
    if not getattr(res, "journeys"):
        return res
    for j in res.journeys:
        if not getattr(j, "sections"):
            continue
        previous_section_begin = j.arrival_date_time
        for s in j.sections:
            from copy import deepcopy
            o = deepcopy(s.origin)
            d = deepcopy(s.destination)
            s.origin.CopyFrom(d)
            s.destination.CopyFrom(o)
            s.end_date_time = previous_section_begin
            previous_section_begin = s.begin_date_time = s.end_date_time - s.duration
    return res


class Scenario(new_default.Scenario):

    def __init__(self):
        super(Scenario, self).__init__()

    def _get_direct_path(self, instance, mode, pt_object_origin, pt_object_destination, datetime, clockwise, reverse_sections=False):
        dp_key = (mode, pt_object_origin.uri, pt_object_destination.uri, datetime, clockwise, reverse_sections)
        dp = g.fallback_direct_path.get(dp_key)
        if not dp:
            dp = g.fallback_direct_path[dp_key] = instance.street_network_service.direct_path(mode,
                                                                                              pt_object_origin,
                                                                                              pt_object_destination,
                                                                                              datetime,
                                                                                              clockwise)
            if reverse_sections:
                _reverse_journeys(dp)
        return dp

    def _get_stop_points(self, instance, place, mode, max_duration, reverse=False, max_nb_crowfly=5000):
        # we use place_nearby of kraken at the first place to get stop_points around the place, then call the
        # one_to_many(or many_to_one according to the arg "reverse") service to take street network into consideration
        # TODO: reverse is not handled as so far

        # When max_duration is 0, there is no need to compute the fallback to pt, except if place is a stop_point or a
        # stop_area
        if max_duration == 0:
            if instance.georef.get_stop_points_from_uri(place.uri):
                return {place.uri: 0}
            else:
                return {}
        places_crowfly = instance.georef.get_crow_fly(get_uri_pt_object(place), mode, max_duration, max_nb_crowfly)

        sn_routing_matrix = instance.street_network_service.get_street_network_routing_matrix([place],
                                                                                             places_crowfly,
                                                                                             mode,
                                                                                             max_duration)
        if not sn_routing_matrix.rows[0].duration:
            return {}
        import numpy as np
        durations = np.array(sn_routing_matrix.rows[0].duration)
        valid_duration_idx = np.argwhere((durations > -1) & (durations < max_duration)).flatten()
        return dict(zip([places_crowfly[i].uri for i in valid_duration_idx],
                        durations[(durations > -1) & (durations < max_duration)].flatten()))

    def _extend_journey(self, instance, pt_journey, mode, pb_from, pb_to, departure_date_time, nm, clockwise,
                        reverse_sections=False):
        import copy
        departure_dp = self._get_direct_path(instance,
                                             mode,
                                             pb_from,
                                             pb_to,
                                             departure_date_time,
                                             clockwise,
                                             reverse_sections)
        if clockwise:
            pt_journey.duration += nm[pb_to.uri]
        elif not reverse_sections:
            pt_journey.duration += nm[pb_from.uri]
        elif reverse_sections:
            pt_journey.duration += nm[pb_to.uri]

        departure_direct_path = copy.deepcopy(departure_dp)
        pt_journey.durations.walking += departure_direct_path.journeys[0].durations.walking
        _extend_pt_sections_with_direct_path(pt_journey, departure_direct_path)

    def _build_journey(self, journey, instance, _from, to, dep_mode, arr_mode, crow_fly_stop_points):
        origins = g.origins_fallback[dep_mode]
        destinations = g.destinations_fallback[arr_mode]

        departure = journey.sections[0].origin
        arrival = journey.sections[-1].destination
        last_section_end = journey.sections[-1].end_date_time

        journey.departure_date_time = journey.departure_date_time - origins[departure.uri]
        journey.arrival_date_time = journey.arrival_date_time + destinations[arrival.uri]

        if _from.uri != departure.uri:
            if departure.uri in crow_fly_stop_points:
                journey.sections.extend([create_crowfly(_from, departure, journey.departure_date_time,
                                         journey.sections[0].begin_date_time)])
            else:
                self._extend_journey(instance, journey, dep_mode, _from, departure, journey.departure_date_time,
                                     origins,
                                     True)

        if to.uri != arrival.uri:
            if arrival.uri in crow_fly_stop_points:
                journey.sections.extend([create_crowfly(arrival, to, last_section_end,
                                                        journey.arrival_date_time)])
            else:
                o = arrival
                d = to
                reverse_sections = False
                if arr_mode == 'car':
                    o, d, reverse_sections = d, o, True
                self._extend_journey(instance, journey, arr_mode, o, d, journey.arrival_date_time, destinations, False,
                                     reverse_sections)

        journey.durations.total = journey.duration
        #it's not possible to insert in a protobuf list, so we add the sections at the end, then we sort them
        journey.sections.sort(SectionSorter())

    def call_kraken(self, request_type, request, instance, krakens_call):
        """
        For all krakens_call, call the kraken and aggregate the responses

        return the list of all responses
        """
        logger = logging.getLogger(__name__)
        logger.debug('datetime: %s', request['datetime'])
        crow_fly_stop_points = set()
        if not g.requested_origin:
            g.requested_origin = instance.georef.place(request['origin'])
        if not g.requested_destination:
            g.requested_destination = instance.georef.place(request['destination'])

        for dep_mode, arr_mode in krakens_call:
            if dep_mode not in g.origins_fallback and request.get('max_duration', 0):
                g.origins_fallback[dep_mode] = self._get_stop_points(instance,
                                                                     g.requested_origin, dep_mode,
                                                                     get_max_fallback_duration(request, dep_mode))

                #Fetch all the stop points of this stop_area and replaces all the durations by 0 in the table
                #g.origins_fallback[dep_mode]
                _update_crowfly_duration(instance, g.origins_fallback, dep_mode, request['origin'],
                                         crow_fly_stop_points)

            if arr_mode not in g.destinations_fallback and request.get('max_duration', 0):
               g.destinations_fallback[arr_mode] = self._get_stop_points(instance,
                                                                         g.requested_destination, arr_mode,
                                                                         get_max_fallback_duration(request, arr_mode),
                                                                         reverse=True)

               #Fetch all the stop points of this stop_area and replaces all the durations by 0 in the table
               #g.destinations_fallback[arr_mode]
               _update_crowfly_duration(instance, g.destinations_fallback, arr_mode, request['destination'],
                                        crow_fly_stop_points)
        resp = []
        journey_parameters = create_parameters(request)
        for dep_mode, arr_mode in krakens_call:
            #todo: this is probably shared between multiple thread
            self.nb_kraken_calls += 1
            direct_path = self._get_direct_path(instance,
                                                dep_mode,
                                                g.requested_origin,
                                                g.requested_destination,
                                                request['datetime'],
                                                request['clockwise'])
            if direct_path.journeys:
                journey_parameters.direct_path_duration = direct_path.journeys[0].durations.total
                #Since _get_direct_method returns a reference instead of an object we don't add in response
                #if already exists.
                if direct_path not in resp:
                    resp.append(direct_path)
            else:
                journey_parameters.direct_path_duration = None

            fall_back_origins = g.origins_fallback.get(dep_mode)
            fall_back_destinations = g.destinations_fallback.get(arr_mode)
            if not fall_back_origins or not fall_back_destinations or not request.get('max_duration', 0):
                continue

            local_resp = instance.planner.journeys(fall_back_origins,
                                                   fall_back_destinations,
                                                   request['datetime'],
                                                   request['clockwise'],
                                                   journey_parameters)

            if local_resp.HasField(b"error") and local_resp.error.id == response_pb2.Error.error_id.Value('no_solution') \
                    and direct_path.journeys:
                local_resp.ClearField(b"error")
            if local_resp.HasField(b"error"):
                return [local_resp]

            # for log purpose we put and id in each journeys
            for idx, j in enumerate(local_resp.journeys):
                j.internal_id = "{resp}-{j}".format(resp=self.nb_kraken_calls, j=idx)

            for journey in local_resp.journeys:
                self._build_journey(journey,
                                    instance,
                                    g.requested_origin,
                                    g.requested_destination,
                                    dep_mode,
                                    arr_mode,
                                    crow_fly_stop_points)

            resp.append(local_resp)
            logger.debug("for mode %s|%s we have found %s journeys", dep_mode, arr_mode, len(local_resp.journeys))
            logger.debug("for mode %s|%s we have found %s direct path", dep_mode, arr_mode, len(direct_path.journeys))

        for r in resp:
            fill_uris(r)
        return resp

    def isochrone(self, request, instance):
        return self.__on_journeys(type_pb2.ISOCHRONE, request, instance)

    def journeys(self, request, instance):
        logger = logging.getLogger(__name__)
        logger.warn('using experimental scenario!!!')
        _init_g()
        return self.__on_journeys(type_pb2.PLANNER, request, instance)

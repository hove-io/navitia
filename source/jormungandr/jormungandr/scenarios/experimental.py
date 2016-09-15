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

from jormungandr import app


def create_crowfly(_from, to, begin, end, mode='walking'):
    section = response_pb2.Section()
    section.type = response_pb2.CROW_FLY
    section.origin.CopyFrom(_from)
    section.destination.CopyFrom(to)
    section.duration = end-begin;
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


def update_crowfly_duration(instance, fallback_list, mode, stop_area_uri):
    if 'stop_area' not in stop_area_uri:
        return
    stop_points = instance.georef.get_stop_points_for_stop_area(stop_area_uri)
    for stop_point in stop_points:
        if fallback_list[mode].get(stop_point.uri):
            fallback_list[mode][stop_point.uri] = 0


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
        # TODO: cache by (mode, origin, destination) and redate with datetime and clockwise
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

    def _build_journey(self, journey, instance, _from, to, dep_mode, arr_mode):
        import copy
        origins = g.origins_fallback[dep_mode]
        destinations = g.destinations_fallback[arr_mode]

        departure = journey.sections[0].origin
        arrival = journey.sections[-1].destination
        last_section_end = journey.sections[-1].end_date_time

        journey.departure_date_time = journey.departure_date_time - origins[departure.uri]
        journey.arrival_date_time = journey.arrival_date_time + destinations[arrival.uri]

        if _from.uri != departure.uri:
            if origins[departure.uri] == 0:
                journey.sections.extend([create_crowfly(_from, departure, journey.departure_date_time,
                                         journey.sections[0].begin_date_time)])
            else:
                departure_dp = self._get_direct_path(instance,
                                                     dep_mode,
                                                     _from,
                                                     departure,
                                                     journey.departure_date_time,
                                                     True)
            if origins[departure.uri] != 0:
                journey.duration += origins[departure.uri]
                departure_direct_path = copy.deepcopy(departure_dp)
                journey.durations.walking += departure_direct_path.journeys[0].durations.walking
                _extend_pt_sections_with_direct_path(journey, departure_direct_path)

        if to.uri != arrival.uri:
            if destinations[arrival.uri] == 0:
                journey.sections.extend([create_crowfly(arrival, to, last_section_end,
                                                        journey.arrival_date_time)])
            else:
                o = arrival
                d = to
                reverse_sections = False
                if arr_mode == 'car':
                    o, d, reverse_sections = d, o, True
                dp_journey = self._get_direct_path(instance,
                                                   arr_mode,
                                                   o,
                                                   d,
                                                   journey.arrival_date_time,
                                                   False,
                                                   reverse_sections)
            if destinations[arrival.uri] != 0:
                journey.duration += destinations[arrival.uri]
                arrival_direct_path = copy.deepcopy(dp_journey)
                journey.durations.walking += arrival_direct_path.journeys[0].durations.walking
                _extend_pt_sections_with_direct_path(journey, arrival_direct_path)

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

        for dep_mode, arr_mode in krakens_call:
            if dep_mode not in g.origins_fallback:
                g.origins_fallback[dep_mode] = instance.georef.get_stop_points(request['origin'],
                        dep_mode,
                        get_max_fallback_duration(request, dep_mode))
            #Fetch all the stop points of this stop_area and replaces all the durations by 0 in the table
            #g.origins_fallback[dep_mode]
            update_crowfly_duration(instance, g.origins_fallback, dep_mode,  request['origin'])

            if arr_mode not in g.destinations_fallback:
                g.destinations_fallback[arr_mode] = instance.georef.get_stop_points(request['destination'],
                        arr_mode,
                        get_max_fallback_duration(request, arr_mode), reverse=True)
            #Fetch all the stop points of this stop_area and replaces all the durations by 0 in the table
            #g.destinations_fallback[arr_mode]
            update_crowfly_duration(instance, g.destinations_fallback, arr_mode, request['destination'])

        if not g.requested_origin:
            g.requested_origin = instance.georef.place(request['origin'])
        if not g.requested_destination:
            g.requested_destination = instance.georef.place(request['destination'])

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
                resp.append(direct_path)
            else:
                journey_parameters.direct_path_duration = None
            local_resp = instance.planner.journeys(g.origins_fallback[dep_mode],
                                                   g.destinations_fallback[arr_mode],
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
                                    arr_mode)

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

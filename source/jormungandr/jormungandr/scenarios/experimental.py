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


def build_journey(journey, _from, to, origins, destinations):
    departure = journey.sections[0].origin
    arrival = journey.sections[-1].destination

    journey.duration = journey.duration + origins[departure.uri] + destinations[arrival.uri]
    journey.departure_date_time = journey.departure_date_time - origins[departure.uri]
    journey.arrival_date_time = journey.arrival_date_time + destinations[arrival.uri]

    #it's not possible to insert in a protobuf list, so we add the sections at the end, then we sort them
    journey.sections.extend([
        create_crowfly(arrival, to, journey.sections[-1].end_date_time,
                       journey.arrival_date_time)])
    journey.sections.extend([
        create_crowfly(_from, departure, journey.departure_date_time,
                       journey.sections[0].begin_date_time)])
    journey.sections.sort(SectionSorter())


#TODO: make this work, it's dynamically imported, so the function is register too late
#@app.before_request
def _init_g():
    g.origins_fallback = {}
    g.destinations_fallback = {}
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


class Scenario(new_default.Scenario):

    def __init__(self):
        super(Scenario, self).__init__()

    def _get_direct_path(self, instance, mode, pt_object_origin,
                         pt_object_destination, datetime, clockwise):
        # TODO: cache by (mode, origin, destination) and redate with datetime and clockwise
        return instance.street_network_service.direct_path(mode, pt_object_origin,
                                                           pt_object_destination, datetime, clockwise)

    def _get_stop_points(self, instance, place, mode, max_duration, reverse=False, max_nb_crowfly=5000):
        # we use place_nearby of kraken at the first place to get stop_points around the place, then call the
        # one_to_many(or many_to_one according to the arg "reverse") service to take street network into consideration
        # TODO: reverse is not handled as so far
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

    def call_kraken(self, request_type, request, instance, krakens_call):
        """
        For all krakens_call, call the kraken and aggregate the responses

        return the list of all responses
        """
        logger = logging.getLogger(__name__)
        logger.debug('datetime: %s', request['datetime'])

        if not g.requested_origin:
            g.requested_origin = instance.georef.place(request['origin'])
        if not g.requested_destination:
            g.requested_destination = instance.georef.place(request['destination'])

        for dep_mode, arr_mode in krakens_call:
            if dep_mode not in g.origins_fallback:
                g.origins_fallback[dep_mode] = self._get_stop_points(instance,
                                                                     g.requested_origin, dep_mode,
                                                                     get_max_fallback_duration(request, dep_mode))

                #logger.debug('origins %s: %s', dep_mode, g.origins_fallback[dep_mode])

            if arr_mode not in g.destinations_fallback:
                g.destinations_fallback[arr_mode] = self._get_stop_points(instance,
                                                                          g.requested_destination, arr_mode,
                                                                          get_max_fallback_duration(request, arr_mode),
                                                                          reverse=True)
                #logger.debug('destinations %s: %s', arr_mode, g.destinations_fallback[arr_mode])

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
                build_journey(journey,
                                   g.requested_origin,
                                   g.requested_destination,
                                   g.origins_fallback[dep_mode],
                                   g.destinations_fallback[arr_mode])

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

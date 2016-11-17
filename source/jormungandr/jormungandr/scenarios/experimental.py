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
from jormungandr.scenarios import new_default
from navitiacommon import type_pb2, response_pb2, request_pb2
import uuid
from jormungandr.scenarios.utils import fill_uris
from jormungandr.planner import JourneyParameters
from flask import g
from jormungandr.utils import get_uri_pt_object, generate_id
from jormungandr import app
import gevent
import gevent.pool
import collections
import gevent.monkey
gevent.monkey.patch_socket()

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
    g.origins_places_crowfly = {}
    g.destinations_places_crowfly = {}
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


def _update_crowfly_duration(instance, mode, stop_area_uri):
    stop_points = instance.georef.get_stop_points_for_stop_area(stop_area_uri)
    crowfly = set()
    fallback_list = collections.defaultdict(dict)
    for stop_point in stop_points:
        fallback_list[mode][stop_point.uri] = 0
        crowfly.add(stop_point.uri)
    return crowfly, fallback_list


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


def _get_places_crowfly(instance, mode, place, max_duration, max_nb_crowfly=5000, **kwargs):
    # When max_duration is 0, there is no need to compute the fallback to pt, except if place is a stop_point or a
    # stop_area
    if max_duration == 0:
        if instance.georef.get_stop_points_from_uri(place.uri):
            return {mode: place}
        else:
            return {mode: []}
    return {mode: instance.georef.get_crow_fly(get_uri_pt_object(place), mode, max_duration,
                                               max_nb_crowfly, **kwargs)}


def _sn_routing_matrix(instance, place, places_crowfly, mode, max_duration, request, **kwargs):
    # When max_duration is 0, there is no need to compute the fallback to pt, except if place is a stop_point or a
    # stop_area
    if max_duration == 0:
        if place:
            return {mode: {place.uri: 0}}
        else:
            return {mode: {}}
    places = places_crowfly[mode]
    sn_routing_matrix = instance.street_network_service.get_street_network_routing_matrix([place],
                                                                                           places,
                                                                                           mode,
                                                                                           max_duration,
                                                                                           request,
                                                                                           **kwargs)
    if not sn_routing_matrix.rows[0].duration:
            return {mode: {}}
    import numpy as np
    durations = np.array(sn_routing_matrix.rows[0].duration)
    valid_duration_idx = np.argwhere((durations > -1) & (durations < max_duration)).flatten()
    return {mode: dict(zip([places[i].uri for i in valid_duration_idx],
                       durations[(durations > -1) & (durations < max_duration)].flatten()))}


def worker_routing_matrix(instance, dep_arr, place, places_crowfly, mode, max_duration, request, **kwargs):
    return mode, dep_arr, _sn_routing_matrix(instance, place, places_crowfly, mode, max_duration, request, **kwargs)


def worker_crowfly(instance, place, dep_arr, mode, request, max_nb_crowfly=5000, **kwargs):
    return (mode, dep_arr, _get_places_crowfly(instance, mode, place, get_max_fallback_duration(request, mode),
                                               max_nb_crowfly, **kwargs))


def worker_update_crowfly_duration(instance, dep_arr, mode, place):
    return mode, dep_arr, _update_crowfly_duration(instance,  mode, place)


def worker_direct_path(func, fallback_direct_path, instance, mode, origin, destination, datetime,
                       clockwise, request):
    return (mode, func(instance, mode, origin, destination, datetime, clockwise, request,
                       fallback_direct_path))


def worker_journey(instance, dep_mode, arr_mode, pt_object_origin, pt_object_destination, request,
                 fallback_direct_path, fall_back_origins, fall_back_destinations, journey_parameters,
                 reverse_sections=False):
    dp_key = (dep_mode, pt_object_origin.uri, pt_object_destination.uri, request['datetime'],
              request['clockwise'], reverse_sections)
    dp = fallback_direct_path.get(dp_key)
    if dp.journeys:
        journey_parameters.direct_path_duration = dp.journeys[0].durations.total
    else:
        journey_parameters.direct_path_duration = None
    origins = fall_back_origins.get(dep_mode)
    destinations = fall_back_destinations.get(arr_mode)
    if not fall_back_origins or not fall_back_destinations or not request.get('max_duration', 0):
        return (dep_mode, arr_mode, None)
    return (dep_mode, arr_mode, instance.planner.journeys(origins, destinations,
                                                          request['datetime'], request['clockwise'],
                                                          journey_parameters))


def worker_from(journey, instance, fisrt_section, func_extend_journey, _from, crow_fly_stop_points, request, dep_mode,
                fallback_direct_path, origins_fallback):

    departure = fisrt_section.origin
    journey.departure_date_time = journey.departure_date_time - origins_fallback[departure.uri]
    if _from.uri != departure.uri:
        if departure.uri in crow_fly_stop_points:
            journey.sections.extend([create_crowfly(_from, departure, journey.departure_date_time,
                                     journey.sections[0].begin_date_time)])
        else:
            func_extend_journey(instance, journey, dep_mode, _from, departure, journey.departure_date_time,
                                 origins_fallback, True, request, fallback_direct_path)
    return journey


def worker_to(journey, instance, last_section, func_extend_journey, to, crow_fly_stop_points, request, arr_mode,
              fallback_direct_path, destinations_fallback):

    arrival = last_section.destination
    journey.arrival_date_time = journey.arrival_date_time + destinations_fallback[arrival.uri]
    last_section_end = last_section.end_date_time

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
            func_extend_journey(instance, journey, arr_mode, o, d, journey.arrival_date_time,
                                destinations_fallback, False, request, fallback_direct_path, reverse_sections)
    return journey


class Worker(object):
    def __init__(self, instance, krakens_call, request, size=app.config.get('GREENLET_POOL_SIZE', 3)):
        self.pool = gevent.pool.Pool(size)
        self.instance = instance
        self.krakens_call = krakens_call
        self.request = request
        self.speed_switcher = {
            "walking": instance.walking_speed,
            "bike": instance.bike_speed,
            "car": instance.car_speed,
            "bss": instance.bss_speed,
        }

    def get_routing_matrix_futures(self, origin, destination,
                                   origins_places_crowfly,
                                   destinations_places_crowfly,
                                   request):
        futures = []
        for dep_mode, arr_mode in self.krakens_call:
            futures.append(self.pool.spawn(worker_routing_matrix,
                                           self.instance,
                                           "dep",
                                           origin,
                                           origins_places_crowfly,
                                           dep_mode,
                                           get_max_fallback_duration(request, dep_mode),
                                           self.request,
                                           **self.speed_switcher))

            futures.append(self.pool.spawn(worker_routing_matrix,
                                           self.instance,
                                           "arr",
                                           destination,
                                           destinations_places_crowfly,
                                           arr_mode,
                                           get_max_fallback_duration(request, arr_mode),
                                           self.request,
                                           **self.speed_switcher))
        return futures

    def get_crowfly_futures(self, origin, destination, max_nb_crowfly=5000):
        futures = []
        for dep_mode, arr_mode in self.krakens_call:
            futures.append(self.pool.spawn(worker_crowfly, self.instance, origin, "dep", dep_mode,
                                           self.request, max_nb_crowfly, **self.speed_switcher))
            futures.append(self.pool.spawn(worker_crowfly, self.instance, destination, "arr", arr_mode,
                                           self.request, max_nb_crowfly, **self.speed_switcher))
        return futures

    def get_update_crowfly_duration_futures(self):
        futures_duration = []
        for dep_mode, arr_mode in self.krakens_call:
            futures_duration.append(self.pool.spawn(worker_update_crowfly_duration, self.instance, "dep", dep_mode,
                                                    self.request['origin']))
            futures_duration.append(self.pool.spawn(worker_update_crowfly_duration, self.instance, "arr", arr_mode,
                                                    self.request['destination']))
        return futures_duration

    def get_direct_path_futures(self, func, fallback_direct_path, origin, destination):
        futures_direct_path = []
        for dep_mode, arr_mode in self.krakens_call:
            futures_direct_path.append(self.pool.spawn(worker_direct_path, func,
                                                              fallback_direct_path, self.instance,
                                                              dep_mode, origin, destination,
                                                              self.request['datetime'],
                                                              self.request['clockwise'],
                                                              self.request))
        return futures_direct_path

    def get_journey_futures(self, origin, destination, fallback_direct_path, origins_fallback,
                            destinations_fallback, journey_parameters):
        futures_jourenys = []
        for dep_mode, arr_mode in self.krakens_call:
            futures_jourenys.append(self.pool.spawn(worker_journey, self.instance,
                                                        dep_mode, arr_mode, origin,
                                                        destination, self.request,
                                                        fallback_direct_path, origins_fallback, destinations_fallback,
                                                        journey_parameters))
        return futures_jourenys

    def build_journeys(self, func_extend_journey, map_response, crow_fly_stop_points):
        futures = []
        for values in map_response:
            dep_mode = values[0]
            arr_mode = values[1]
            journey = values[2]
            # from
            futures.append(self.pool.spawn(worker_from, journey, self.instance,
                                                journey.sections[0], func_extend_journey, g.requested_origin,
                                                crow_fly_stop_points, self.request,
                                                dep_mode, g.fallback_direct_path, g.origins_fallback[dep_mode]))
            # to
            futures.append(self.pool.spawn(worker_to, journey, self.instance,
                                                journey.sections[-1], func_extend_journey, g.requested_destination,
                                                crow_fly_stop_points, self.request,
                                                arr_mode, g.fallback_direct_path, g.destinations_fallback[arr_mode]))
        return futures


class Scenario(new_default.Scenario):

    def __init__(self):
        super(Scenario, self).__init__()

    def _get_direct_path(self, instance, mode, pt_object_origin, pt_object_destination, datetime, clockwise,
                         request, fallback_direct_path, reverse_sections=False):
        dp_key = (mode, pt_object_origin.uri, pt_object_destination.uri, datetime, clockwise, reverse_sections)
        dp = fallback_direct_path.get(dp_key)
        if not dp:
            dp = fallback_direct_path[dp_key] = instance.street_network_service.direct_path(mode,
                                                                                              pt_object_origin,
                                                                                              pt_object_destination,
                                                                                              datetime,
                                                                                              clockwise,
                                                                                              request)
            if reverse_sections:
                _reverse_journeys(dp)
        return dp

    def _extend_journey(self, instance, pt_journey, mode, pb_from, pb_to, departure_date_time, nm, clockwise,
                        request, fallback_direct_path, reverse_sections=False):
        import copy

        departure_dp = self._get_direct_path(instance, mode, pb_from, pb_to, departure_date_time, clockwise, request,
                                             fallback_direct_path, reverse_sections)
        if clockwise:
            pt_journey.duration += nm[pb_to.uri]
        elif not reverse_sections:
            pt_journey.duration += nm[pb_from.uri]
        elif reverse_sections:
            pt_journey.duration += nm[pb_to.uri]

        departure_direct_path = copy.deepcopy(departure_dp)
        pt_journey.durations.walking += departure_direct_path.journeys[0].durations.walking
        _extend_pt_sections_with_direct_path(pt_journey, departure_direct_path)

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

        worker = Worker(instance, krakens_call, request)

        if request.get('max_duration', 0):
            futures = worker.get_crowfly_futures(g.requested_origin, g.requested_destination, 5000)
            for future in gevent.iwait(futures):
                _, dep_arr, resp = future.get()
                if dep_arr == "dep":
                    g.origins_places_crowfly.update(resp)
                else:
                    g.destinations_places_crowfly.update(resp)

            futures = worker.get_routing_matrix_futures(g.requested_origin, g.requested_destination,
                                                        g.origins_places_crowfly, g.destinations_places_crowfly,
                                                        request)
            for future in gevent.iwait(futures):
                _, dep_arr, resp = future.get()
                if dep_arr == "dep":
                    g.origins_fallback.update(resp)
                else:
                    g.destinations_fallback.update(resp)

            futures = worker.get_update_crowfly_duration_futures()
            for future in gevent.iwait(futures):
                _, dep_arr, resp = future.get()
                crow_fly_stop_points |= resp[0]
                fb = g.origins_fallback if dep_arr == "dep" else g.destinations_fallback
                for mode in (mode for mode in resp[1] if mode in fb):
                    fb[mode].update(resp[1][mode])
        resp = []
        journey_parameters = create_parameters(request)

        futures = worker.get_direct_path_futures(self._get_direct_path, g.fallback_direct_path,
                                                             g.requested_origin,
                                                             g.requested_destination)
        for future in gevent.iwait(futures):
            self.nb_kraken_calls += 1
            dep_mode, resp_direct_path = future.get()
            if resp_direct_path.journeys:
                if resp_direct_path not in resp:
                    resp_direct_path.journeys[0].internal_id = str(generate_id())
                    resp.append(resp_direct_path)

        futures = worker.get_journey_futures(g.requested_origin, g.requested_destination,
                                                      g.fallback_direct_path, g.origins_fallback,
                                                      g.destinations_fallback, journey_parameters)

        map_response = []
        for future in gevent.iwait(futures):
            dep_mode, arr_mode, local_resp = future.get()
            if local_resp == None:
                continue
            dp_key = (dep_mode, g.requested_origin.uri, g.requested_destination.uri, request['datetime'],
                      request['clockwise'], False)
            direct_path = g.fallback_direct_path.get(dp_key)

            if local_resp.HasField(b"error") and local_resp.error.id == response_pb2.Error.error_id.Value('no_solution') \
                    and direct_path.journeys:
                local_resp.ClearField(b"error")
            if local_resp.HasField(b"error"):
                return [local_resp]
            # for log purpose we put and id in each journeys
            for j in local_resp.journeys:
                j.internal_id = str(generate_id())
                map_response.append((dep_mode, arr_mode, j))
            resp.append(local_resp)

        futures = worker.build_journeys(self._extend_journey, map_response, crow_fly_stop_points)
        for future in gevent.iwait(futures):
            journey = future.get()
            journey.durations.total = journey.duration
            journey.sections.sort(SectionSorter())

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

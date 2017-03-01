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
from navitiacommon import type_pb2, response_pb2
import uuid
from jormungandr.scenarios.utils import fill_uris
from jormungandr.planner import JourneyParameters
from flask import g
from jormungandr.utils import get_uri_pt_object, generate_id, get_pt_object_coord
from jormungandr import app
import gevent
import gevent.pool
import collections
import copy

def create_crowfly(pt_journey, crowfly_origin, crowfly_destination, begin, end, mode='walking'):
    section = response_pb2.Section()
    section.type = response_pb2.CROW_FLY
    section.origin.CopyFrom(crowfly_origin)
    section.destination.CopyFrom(crowfly_destination)
    section.duration = end - begin
    pt_journey.durations.walking += section.duration
    pt_journey.durations.total += section.duration
    pt_journey.duration += section.duration
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


# dp_durations (for direct path duration) is used to limit the
# fallback duration of the first section.  Because we don't want to
# have more fallback than if we go to the destination directly, we can
# limit the radius of the fallback with this value.
def get_max_fallback_duration(request, mode, dp_durations={}):
    if mode in ['walking', 'bss', 'bike', 'car']:
        max_duration = request['max_{}_duration_to_pt'.format(mode)]
        dp_duration = dp_durations.get(mode, max_duration)
        return min(max_duration, dp_duration)
    raise ValueError('unknown mode: {}'.format(mode))


def make_direct_path_key(dep_mode, orig_uri, dest_uri, is_fallback_at_end,
                         datetime, is_start_fallback_datetime):
    '''
    A, B and mode matters obviously
    whether it's a fallback at the beginning or end of journey also matters especially for car
    (to know if we park before or after)
    Nota: datetime and is_start_fallback_datetime is not taken into consideration so far because we assume
    that a direct path from A to B remains the same even the departure time are different (no realtime)
    Nota: this implementation is connector-specific (so shouldn't be here)
    '''
    return dep_mode, is_fallback_at_end, orig_uri, dest_uri


def make_direct_path_duration_by_mode(fallback_direct_path_pool):
    res = {}
    for key, dp in fallback_direct_path_pool.items():
        if dp.journeys:
            # key[0] is the mode of the direct path
            res[key[0]] = dp.journeys[0].durations.total
    return res


def get_direct_path_if_exists(fallback_direct_path_pool, mode, orig_uri, dest_uri,
                              datetime, is_start_fallback_datetime, is_fallback_at_end):
    """
    in this function, we retrieve from fallback_direct_path_pool the direct path regarding to the given
    parameters(mode, orig_uri, etc...) then we recompute the datetimes of the found direct path,
    since the request datetime might no longer be the same (but we consider the same fallback duration).
    Nota: this implementation is connector-specific (so shouldn't be here)
    """
    dp_key = make_direct_path_key(mode, orig_uri, dest_uri, is_fallback_at_end,
                                  datetime, is_start_fallback_datetime)
    dp_copy = copy.deepcopy(fallback_direct_path_pool.get(dp_key))
    if not dp_copy or not dp_copy.journeys:
        return None

    # align datetimes to requested ones (as we consider fallback duration are the same no matter when)
    journey = dp_copy.journeys[0]
    if is_start_fallback_datetime:
        journey.departure_date_time = datetime
        journey.arrival_date_time = datetime + journey.duration
    else:
        journey.departure_date_time = datetime - journey.duration
        journey.arrival_date_time = datetime

    delta = journey.departure_date_time - journey.sections[0].begin_date_time
    if delta != 0:
        for s in journey.sections:
            s.begin_date_time += delta
            s.end_date_time += delta
    return dp_copy


class FallbackDuration(dict):
    """
    Dictionary containing the id of the places and the duration to arrive there and the projection status
    Example :
    {
        "walking" : {"id1" : {"duration": 6, "status": response_pb2.reached},
                    "id2" : {"duration": 15, "status": response_pb2.unknown}}}
    """
    def reset_if_exist(self, mode, uri):
        value = self.get(mode)
        if value and uri in value:
            self.update({uri: {"duration": 0, "status": response_pb2.reached}})

    def get_duration(self, mode, uri):
        values = self.get(mode)
        return values.get(uri).get('duration')

    def is_crowfly_needed(self, mode, uri):
        values = self.get(mode)
        return values.get(uri).get("status") == response_pb2.unknown

    def get_locations_contexts(self, mode):
        return {key: value.get("duration") for key, value in self.get(mode, {}).items()}

    def merge_reached_values(self, mode, values):
        local_values = self.get(mode)
        if not local_values:
            self.update({mode:{}})
            local_values = self.get(mode)
        for key, value in values.items():
            local_values.update({key: {"duration": value, "status": response_pb2.reached}})


#TODO: make this work, it's dynamically imported, so the function is register too late
#@app.before_request
def _init_g():
    g.origins_places_crowfly = {}
    g.destinations_places_crowfly = {}
    g.origins_fallback = FallbackDuration()
    g.destinations_fallback = FallbackDuration()
    g.fallback_direct_path_pool = {} #dict[direct_path_key]=direct_path
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




def _update_crowfly_duration(instance, mode, requested_entry_point):
    """
    compute the list of stoppoint that can be accessed for free by crowfly
    for a stoparea it's the list of it's stoppoints
    for an admin it's the list of it's main_stop_area's stoppoints

    we also look if there are on demand transport (odt) stoppoints in the area

    :return
        the list of the freely accessed stoppoints
        the list of odt stoppoints
        the fallback duration for all those stoppoints
    """
    stop_points = []
    if requested_entry_point.embedded_type == type_pb2.STOP_AREA:
        stop_points = instance.georef.get_stop_points_for_stop_area(requested_entry_point.uri)
    elif requested_entry_point.embedded_type == type_pb2.ADMINISTRATIVE_REGION:
        stop_points = [sp for sa in requested_entry_point.administrative_region.main_stop_areas
                       for sp in sa.stop_points]
    elif requested_entry_point.embedded_type == type_pb2.STOP_POINT:
        stop_points = [requested_entry_point.stop_point]

    crowfly_sps = set()
    odt_stop_points = set()
    fallback_list = collections.defaultdict(dict)
    for stop_point in stop_points:
        fallback_list[mode][stop_point.uri] = 0
        crowfly_sps.add(stop_point.uri)

    coord = get_pt_object_coord(requested_entry_point)

    if coord:
        odt_sps = instance.georef.get_odt_stop_points(coord)
        for stop_point in odt_sps:
            fallback_list[mode][stop_point.uri] = 0
            odt_stop_points.add(stop_point.uri)

    return crowfly_sps, odt_stop_points, fallback_list


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
            o = copy.deepcopy(s.origin)
            d = copy.deepcopy(s.destination)
            s.origin.CopyFrom(d)
            s.destination.CopyFrom(o)
            s.end_date_time = previous_section_begin
            previous_section_begin = s.begin_date_time = s.end_date_time - s.duration
        j.sections.sort(SectionSorter())
    return res


def _get_places_crowfly(instance, mode, place, max_duration_to_pt, max_nb_crowfly=5000, **kwargs):
    # When max_duration_to_pt is 0, there is no need to compute the fallback to pt, except if place is a stop_point or a
    # stop_area
    if max_duration_to_pt == 0:
        # When max_duration_to_pt is 0, we can get on the public transport ONLY if the place is a stop_point
        if instance.georef.get_stop_points_from_uri(place.uri):
            return {mode: place}
        else:
            return {mode: []}
    coord = get_pt_object_coord(place)
    if not coord.lat and not coord.lon:
        return {mode: []}
    res = instance.georef.get_crow_fly(get_uri_pt_object(place), mode, max_duration_to_pt,
                                       max_nb_crowfly, **kwargs)

    return {mode: res}


def _get_duration(resp, place, mode, **kwargs):
    from math import sqrt
    map_response = {
        response_pb2.reached: resp.duration,
        # Calculate duration
        response_pb2.unknown: int((place.distance*sqrt(2))/kwargs.get(mode))
    }
    return map_response[resp.routing_status]


def _sn_routing_matrix(instance, origins, destinations, is_orig_center, mode, max_duration_to_pt, request, **kwargs):
    # When max_duration_to_pt is 0, there is no need to compute the fallback to pt, except if place is a stop_point or a
    # stop_area
    center_isochrone = origins[0] if is_orig_center else destinations[0]
    places_isochrone = destinations if is_orig_center else origins
    if max_duration_to_pt == 0:
        # When max_duration_to_pt is 0, we can get on the public transport ONLY if the place is a stop_point
        if instance.georef.get_stop_points_from_uri(center_isochrone.uri):
            return {mode: {center_isochrone.uri: {"duration": 0, "status": response_pb2.reached}}}
        else:
            return {mode: {}}
    sn_routing_matrix = instance.get_street_network_routing_matrix(origins,
                                                                   destinations,
                                                                   mode,
                                                                   max_duration_to_pt,
                                                                   request,
                                                                   **kwargs)
    if not len(sn_routing_matrix.rows) or not len(sn_routing_matrix.rows[0].routing_response):
        return {mode: {}}

    result = {mode: {}}
    for pos, r in enumerate(sn_routing_matrix.rows[0].routing_response):
        if r.routing_status != response_pb2.unreached:
            duration = _get_duration(r, places_isochrone[pos], mode, **kwargs)
            if duration < max_duration_to_pt:
                result[mode].update({places_isochrone[pos].uri: {'duration': duration, 'status': r.routing_status}})
    return result


class AsyncWorker(object):
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

    def get_routing_matrix_futures(self, origin, destination, origins_places_crowfly, destinations_places_crowfly, dp_durations):
        origin_futures = []
        destination_futures = []

        called_dep_modes = set()
        called_arr_modes = set()
        for dep_mode, arr_mode in self.krakens_call:
            if dep_mode not in called_dep_modes:
                origin_futures.append(self.pool.spawn(_sn_routing_matrix,
                                                      self.instance,
                                                      [origin],
                                                      origins_places_crowfly[dep_mode],
                                                      True,
                                                      dep_mode,
                                                      get_max_fallback_duration(self.request, dep_mode, dp_durations),
                                                      self.request,
                                                      **self.speed_switcher))
                called_dep_modes.add(dep_mode)
            if arr_mode not in called_arr_modes:
                destination_futures.append(self.pool.spawn(_sn_routing_matrix,
                                                           self.instance,
                                                           destinations_places_crowfly[arr_mode],
                                                           [destination],
                                                           False,
                                                           arr_mode,
                                                           get_max_fallback_duration(self.request, arr_mode),
                                                           self.request,
                                                           **self.speed_switcher))
                called_arr_modes.add(arr_mode)

        return origin_futures, destination_futures

    def get_crowfly_futures(self, origin, destination, dp_durations):
        origin_futures = []
        destination_futures = []

        called_dep_modes = set()
        called_arr_modes = set()
        for dep_mode, arr_mode in self.krakens_call:
            if dep_mode not in called_dep_modes:
                origin_futures.append(self.pool.spawn(_get_places_crowfly,
                                                      self.instance,
                                                      dep_mode,
                                                      origin,
                                                      get_max_fallback_duration(self.request, dep_mode, dp_durations),
                                                      **self.speed_switcher))
                called_dep_modes.add(dep_mode)
            if arr_mode not in called_arr_modes:
                destination_futures.append(self.pool.spawn(_get_places_crowfly,
                                                           self.instance,
                                                           arr_mode,
                                                           destination,
                                                           get_max_fallback_duration(self.request, arr_mode),
                                                           **self.speed_switcher))
                called_arr_modes.add(arr_mode)
        return origin_futures, destination_futures

    def get_update_crowfly_duration_futures(self):
        origin_futures = []
        destination_futures = []
        called_dep_modes = set()
        called_arr_modes = set()
        for dep_mode, arr_mode in self.krakens_call:
            if dep_mode not in called_dep_modes:
                origin_futures.append(self.pool.spawn(_update_crowfly_duration, self.instance, dep_mode,
                                                      g.requested_origin))
                called_dep_modes.add(dep_mode)
            if arr_mode not in called_arr_modes:
                destination_futures.append(self.pool.spawn(_update_crowfly_duration, self.instance, arr_mode,
                                                           g.requested_destination))
                called_arr_modes.add(arr_mode)
        return origin_futures, destination_futures

    @staticmethod
    def _get_direct_path(instance, mode, pt_object_origin, pt_object_destination,
                         datetime, is_start_fallback_datetime, request, is_fallback_at_end):
        dp_key = make_direct_path_key(mode, pt_object_origin.uri, pt_object_destination.uri,
                                      is_fallback_at_end, datetime, is_start_fallback_datetime)
        dp = instance.direct_path(mode,
                                  pt_object_origin,
                                  pt_object_destination,
                                  datetime,
                                  is_start_fallback_datetime,
                                  request)
        if is_fallback_at_end:
            _reverse_journeys(dp)
        return dp_key, dp

    def get_direct_path_futures(self, fallback_direct_path_pool,
                                origin, destination,
                                datetime, is_start_fallback_datetime,
                                is_fallback_at_end,
                                modes):
        futures_direct_path = []
        for dep_mode in modes:
            dp_key = make_direct_path_key(dep_mode, origin.uri, destination.uri,
                                          is_fallback_at_end, datetime, is_start_fallback_datetime)
            if dp_key not in fallback_direct_path_pool:
                futures_direct_path.append(self.pool.spawn(self._get_direct_path,
                                                           self.instance,
                                                           dep_mode, origin, destination,
                                                           datetime, is_start_fallback_datetime,
                                                           self.request,
                                                           is_fallback_at_end))
                # We initialise the value in fallback_direct_path_pool,
                # so that the same direct path won't be computed twice
                fallback_direct_path_pool[dp_key] = None
        return futures_direct_path

    def get_pt_journey_futures(self, origin, destination, fallback_direct_path_pool,
                               origins_fallback, destinations_fallback, journey_parameters):
        futures_jourenys = []
        is_fallback_at_end = False
        instance = self.instance
        datetime = self.request['datetime']
        clockwise = self.request['clockwise']
        for dep_mode, arr_mode in self.krakens_call:
            dp_key = make_direct_path_key(dep_mode, origin.uri, destination.uri,
                                          is_fallback_at_end, datetime, clockwise)
            dp = fallback_direct_path_pool.get(dp_key)
            journey_parameters = copy.deepcopy(journey_parameters)
            if dp.journeys:
                journey_parameters.direct_path_duration = dp.journeys[0].durations.total
            else:
                journey_parameters.direct_path_duration = None

            origins = origins_fallback.get_locations_contexts(dep_mode)
            destinations = destinations_fallback.get_locations_contexts(arr_mode)

            # default argument to workaround late binding
            # http://stackoverflow.com/questions/3431676/creating-functions-in-a-loop
            def worker_journey(dep_mode=dep_mode,
                               arr_mode=arr_mode,
                               journey_parameters=journey_parameters,
                               origins=origins,
                               destinations=destinations):
                if not origins or not destinations or not self.request.get('max_duration', 0):
                    return dep_mode, arr_mode, None
                return dep_mode, arr_mode, instance.planner.journeys(origins, destinations,
                                                                     datetime, clockwise,
                                                                     journey_parameters)
            futures_jourenys.append(self.pool.spawn(worker_journey))
        return futures_jourenys

    @staticmethod
    def _extend_journey(pt_journey, mode, pb_from, pb_to,
                        datetime, is_start_fallback_datetime,
                        fallback_direct_path_pool, is_fallback_at_end):
        fallback_dp = get_direct_path_if_exists(fallback_direct_path_pool, mode, pb_from.uri, pb_to.uri,
                                                datetime, is_start_fallback_datetime, is_fallback_at_end)
        fallback_copy = copy.deepcopy(fallback_dp)
        pt_journey.duration += fallback_copy.journeys[0].duration
        pt_journey.durations.walking += fallback_copy.journeys[0].durations.walking
        _extend_pt_sections_with_direct_path(pt_journey, fallback_copy)

    def _build_from(self, journey, first_section, journey_origin, crowfly_stop_points, odt_stop_points,
                    dep_mode, fallback_direct_path_pool, origins_fallback):

        pt_origin = first_section.origin
        journey.departure_date_time = journey.departure_date_time - \
                                      origins_fallback.get_duration(dep_mode, pt_origin.uri)
        if journey_origin.uri != pt_origin.uri:
            if pt_origin.uri in odt_stop_points:
                journey.sections[0].origin.CopyFrom(journey_origin)
            elif pt_origin.uri in crowfly_stop_points or origins_fallback.is_crowfly_needed(dep_mode, pt_origin.uri):
                journey.sections.extend([create_crowfly(journey, journey_origin, pt_origin, journey.departure_date_time,
                                         journey.sections[0].begin_date_time)])
            else:
                # extend the journey with the fallback routing path
                is_start_fallback_datetime = True
                is_fallback_at_end = False
                self._extend_journey(journey, dep_mode, journey_origin, pt_origin,
                                     journey.departure_date_time, is_start_fallback_datetime,
                                     fallback_direct_path_pool, is_fallback_at_end)
        journey.sections.sort(SectionSorter())
        return journey

    def _build_to(self, journey, last_section, journey_destination, crowfly_stop_points, odt_stop_points,
                  arr_mode, fallback_direct_path_pool, destinations_fallback):

        pt_destination = last_section.destination
        journey.arrival_date_time = journey.arrival_date_time + \
                                    destinations_fallback.get_duration(arr_mode, pt_destination.uri)
        last_section_end = last_section.end_date_time

        if journey_destination.uri != pt_destination.uri:
            if pt_destination.uri in odt_stop_points:
                journey.sections[-1].destination.CopyFrom(journey_destination)
            elif pt_destination.uri in crowfly_stop_points or destinations_fallback.is_crowfly_needed(arr_mode, pt_destination.uri):
                journey.sections.extend([create_crowfly(journey, pt_destination, journey_destination, last_section_end,
                                                        journey.arrival_date_time)])
            else:
                is_start_fallback_datetime = False
                # extend the journey with the fallback routing path
                o = pt_destination
                d = journey_destination
                is_fallback_at_end = False
                if arr_mode == 'car':
                    o, d, is_fallback_at_end = d, o, True
                self._extend_journey(journey, arr_mode, o, d,
                                     journey.arrival_date_time, is_start_fallback_datetime,
                                     fallback_direct_path_pool, is_fallback_at_end)
        journey.sections.sort(SectionSorter())
        return journey

    def get_fallback_direct_path_futures(self, map_response, crowfly_stop_points, odt_stop_points):
        futures = []
        for dep_mode, arr_mode, journey in map_response:
            # from
            departure = journey.sections[0].origin
            clockwise = False
            is_fallback_at_end = False
            arrival_datetime = journey.departure_date_time
            # In the following cases, we don't need to compute the fallback direct path:
            # 1. the origin of the first section and the requested_origin are the same
            # 2. the origin of the first section belongs to a stop_area
            # 3. the origin of the first section belongs to odt stop_points
            if g.requested_origin.uri != departure.uri and \
                departure.uri not in odt_stop_points and departure.uri not in crowfly_stop_points:
                futures.extend(self.get_direct_path_futures(g.fallback_direct_path_pool,
                                                            g.requested_origin,
                                                            departure,
                                                            arrival_datetime,
                                                            clockwise,
                                                            is_fallback_at_end,
                                                            [dep_mode]))
            # to
            arrival = journey.sections[-1].destination
            clockwise = True
            is_fallback_at_end = False
            # In some cases, we don't need to compute the fallback direct path
            # Similar reasoning as above
            departure_datetime = journey.arrival_date_time
            if g.requested_destination.uri != arrival.uri and arrival.uri not in odt_stop_points \
                    and arrival.uri not in crowfly_stop_points:
                o, d = arrival, g.requested_destination
                if arr_mode == 'car':
                    o, d, is_fallback_at_end = d, o, True
                futures.extend(self.get_direct_path_futures(g.fallback_direct_path_pool,
                                                            o,
                                                            d,
                                                            departure_datetime,
                                                            clockwise,
                                                            is_fallback_at_end,
                                                            [arr_mode]))
        return futures

    def build_journeys(self, map_response, crowfly_stop_points, odt_stop_points):
        futures = []
        for dep_mode, arr_mode, journey in map_response:
            # from
            futures.append(self.pool.spawn(self._build_from, journey,
                                           journey.sections[0], g.requested_origin,
                                           crowfly_stop_points, odt_stop_points,
                                           dep_mode, g.fallback_direct_path_pool, g.origins_fallback))
            # to
            futures.append(self.pool.spawn(self._build_to, journey,
                                           journey.sections[-1], g.requested_destination,
                                           crowfly_stop_points, odt_stop_points,
                                           arr_mode, g.fallback_direct_path_pool, g.destinations_fallback))

        for future in gevent.iwait(futures):
            journey = future.get()
            journey.durations.total = journey.duration
            journey.sections.sort(SectionSorter())


class Scenario(new_default.Scenario):

    def __init__(self):
        super(Scenario, self).__init__()

    @staticmethod
    def _make_error_response(message, error_id):
        r = response_pb2.Response()
        r.error.message = message
        r.error.id = error_id
        return r

    @staticmethod
    def update_error_message(response, error_id, message):
        response.error.id = error_id
        response.error.message = message

    def call_kraken(self, request_type, request, instance, krakens_call):
        """
        For all krakens_call, call the kraken and aggregate the responses

        return the list of all responses
        """
        logger = logging.getLogger(__name__)
        logger.debug('datetime: %s', request['datetime'])

        # odt_stop_points is a set of stop_point.uri with is_zonal = true used to manage tad_zonal
        odt_stop_points = set()

        # crowfly_stop_points is a set of stop_point.uri used to create a crowfly section.
        crowfly_stop_points = set()

        if not g.requested_origin:
            g.requested_origin = instance.georef.place(request['origin'])
            if not g.requested_origin:
                r = self._make_error_response("The entry point: {} is not valid".format(request['origin']),
                                              response_pb2.Error.unknown_object)
                return [r]

        if not g.requested_destination:
            g.requested_destination = instance.georef.place(request['destination'])
            if not g.requested_destination:
                r = self._make_error_response("The entry point: {} is not valid".format(request['destination']),
                                              response_pb2.Error.unknown_object)
                return [r]

        worker = AsyncWorker(instance, krakens_call, request)

        resp = []

        # Now we compute the direct path with all requested departure
        # mode their time will be used to initialized our PT calls and
        # to bound the fallback duration of the first section.
        is_fallback_at_end = False
        futures = worker.get_direct_path_futures(g.fallback_direct_path_pool,
                                                 g.requested_origin,
                                                 g.requested_destination,
                                                 request['datetime'],
                                                 request['clockwise'],
                                                 is_fallback_at_end,
                                                 {mode for mode, _ in krakens_call})
        for future in gevent.iwait(futures):
            resp_key, resp_direct_path = future.get()
            g.fallback_direct_path_pool[resp_key] = resp_direct_path
            if resp_direct_path.journeys:
                resp_direct_path.journeys[0].internal_id = str(generate_id())
                resp.append(resp_direct_path)

        if request.get('max_duration', 0):
            direct_path_duration_by_mode = make_direct_path_duration_by_mode(g.fallback_direct_path_pool)

            # Get all stop_points around the requested origin within a crowfly range
            # Calls on origins and destinations are asynchronous
            orig_futures, dest_futures = worker.get_crowfly_futures(g.requested_origin,
                                                                    g.requested_destination,
                                                                    direct_path_duration_by_mode)
            gevent.joinall(orig_futures + dest_futures)
            for future in orig_futures:
                g.origins_places_crowfly.update(future.get())
            for future in dest_futures:
                g.destinations_places_crowfly.update(future.get())

            # Once we get crow fly stop points with origins and destinations, we start
            # the computation NM: the fallback matrix which contains the arrival duration for crowfly stop_points
            # from origin/destination
            # Ex:
            #                    stop_point1   stop_point2  stop_point3
            # request_origin_1     86400(s)      43200(s)     21600(s)
            # As a side note this won't work the day when our ETA will be impacted by the datetime of the journey,
            # at least for the arrival when doing a "departure after" request.
            orig_futures, dest_futures = worker.get_routing_matrix_futures(g.requested_origin,
                                                                           g.requested_destination,
                                                                           g.origins_places_crowfly,
                                                                           g.destinations_places_crowfly,
                                                                           direct_path_duration_by_mode)
            gevent.joinall(orig_futures + dest_futures)
            for future in orig_futures:
                g.origins_fallback.update(future.get())
            for future in dest_futures:
                g.destinations_fallback.update(future.get())

            # In Some special cases, like "odt" or "departure(arrive) from(to) a stop_area",
            # the first(last) section should be treated differently
            orig_futures, dest_futures = worker.get_update_crowfly_duration_futures()
            gevent.joinall(orig_futures + dest_futures)

            def _updater(_futures, fb, crowfly_stop_points, odt_stop_points):
                for f in _futures:
                    crowfly_res, odt_res, fb_res = f.get()
                    crowfly_stop_points |= crowfly_res
                    odt_stop_points |= odt_res
                    for mode in (mode for mode in fb_res if mode in fb):
                        fb.merge_reached_values(mode, fb_res[mode])

            _updater(orig_futures, g.origins_fallback, crowfly_stop_points, odt_stop_points)
            _updater(dest_futures, g.destinations_fallback, crowfly_stop_points, odt_stop_points)

            # We update the fallback duration matrix if the requested origin/destination is also
            # present in the fallback duration matrix, which means from stop_point_1 to itself, it takes 0 second
            # Ex:
            #                stop_point1   stop_point2  stop_point3
            # stop_point_1         0(s)       ...          ...
            for dep_mode, arr_mode in krakens_call:
                g.origins_fallback.reset_if_exist(dep_mode, g.requested_origin.uri)
                g.destinations_fallback.reset_if_exist(arr_mode, g.requested_destination.uri)

        # Here starts the computation for pt journey
        journey_parameters = create_parameters(request)
        futures = worker.get_pt_journey_futures(g.requested_origin, g.requested_destination,
                                                g.fallback_direct_path_pool, g.origins_fallback,
                                                g.destinations_fallback, journey_parameters)

        response_tuples = []
        for future in gevent.iwait(futures):
            dep_mode, arr_mode, local_resp = future.get()
            if local_resp is None:
                continue
            is_fallback_at_end = False
            dp_key = make_direct_path_key(dep_mode, g.requested_origin.uri, g.requested_destination.uri,
                                          is_fallback_at_end, request['datetime'], request['clockwise'])
            direct_path = g.fallback_direct_path_pool.get(dp_key)

            if local_resp.HasField(b"error") and local_resp.error.id == response_pb2.Error.error_id.Value('no_solution') \
                    and direct_path.journeys:
                local_resp.ClearField(b"error")
            if local_resp.HasField(b"error"):
                #Here needs to modify error message of no_solution
                if len(g.origins_fallback[dep_mode]) == 0:
                    self.update_error_message(local_resp, response_pb2.Error.no_origin, "no origin point")
                elif len(g.destinations_fallback[arr_mode]) == 0:
                    self.update_error_message(local_resp, response_pb2.Error.no_destination, "no destination point")

                return [local_resp]

            # for log purpose we put and id in each journeys
            for j in local_resp.journeys:
                j.internal_id = str(generate_id())
                response_tuples.append((dep_mode, arr_mode, j))
            resp.append(local_resp)

        # Once the pt journey is found, we need to reconstruct the whole journey with fallback regarding the mode

        # For the sake of performance, we compute at first all fallback direct path asynchronously
        # then we update the pool of direct paths
        futures = worker.get_fallback_direct_path_futures(response_tuples, crowfly_stop_points, odt_stop_points)
        for future in gevent.iwait(futures):
            resp_key, resp_direct_path = future.get()
            g.fallback_direct_path_pool[resp_key] = resp_direct_path

        # Now we construct the whole journey by concatenating the fallback direct path with the pt journey
        worker.build_journeys(response_tuples, crowfly_stop_points, odt_stop_points)

        #If resp doesn't contain any response we have to add an error message
        if len(resp) == 0:
            if len(g.origins_fallback[dep_mode]) == 0 and len(g.destinations_fallback[arr_mode]) == 0:
                resp.append(self._make_error_response("no origin point nor destination point",
                                                      response_pb2.Error.no_origin_nor_destination))
            elif len(g.origins_fallback[dep_mode]) == 0:
                resp.append(self._make_error_response("no origin point", response_pb2.Error.no_origin))
            elif len(g.destinations_fallback[arr_mode]) == 0:
                resp.append(self._make_error_response("no destination point", response_pb2.Error.no_destination))
            return resp
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

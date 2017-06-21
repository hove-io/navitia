#  Copyright (c) 2001-2016, Canal TP and/or its affiliates. All rights reserved.
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
from navitiacommon import response_pb2
import logging
import pybreaker
import requests as requests
from jormungandr import app
import json
from jormungandr.exceptions import TechnicalError, InvalidArguments, ApiNotFound
from jormungandr.utils import is_url, kilometers_to_meters, get_pt_object_coord, decode_polyline, \
    timestamp_to_datetime
from copy import deepcopy
from jormungandr.street_network.street_network import AbstractStreetNetworkService, StreetNetworkPathKey, \
    StreetNetworkPathType


def _get_coord(pt_object):
    coord = get_pt_object_coord(pt_object)
    return 'geo!{lat},{lon}'.format(lat=coord.lat, lon=coord.lon)


def get_here_mode(mode):
    if mode == 'walking':
        return 'pedestrian'
    elif mode == 'bike':
        return 'bicycle'
    elif mode == 'car':
        return 'car'
    else:  # HERE does not handle bss
        raise TechnicalError('HERE does not handle the mode {}'.format(mode))


def _str_to_dt(datetime):
    return timestamp_to_datetime(datetime).strftime("%Y-%m-%dT%H:%M:%S%z")


class Here(AbstractStreetNetworkService):

    def __init__(self, instance, service_base_url, id='here', timeout=10, api_id=None, api_code=None,
                 **kwargs):
        self.instance = instance
        self.sn_system_id = id
        if not service_base_url:
            raise ValueError('service_url {} is not a valid HERE url'.format(service_base_url))
        service_base_url = service_base_url.rstrip('/')
        self.routing_service_url = 'https://{base_url}/calculateroute.json'.format(base_url=service_base_url)
        self.matrix_service_url = 'https://matrix.{base_url}/calculatematrix.json'.format(base_url=service_base_url)
        self.api_id = api_id
        self.api_code = api_code
        self.timeout = timeout
        self.max_points = 100  # max number of point asked in the routing matrix
        self.breaker = pybreaker.CircuitBreaker(fail_max=app.config['CIRCUIT_BREAKER_MAX_HERE_FAIL'],
                                                reset_timeout=app.config['CIRCUIT_BREAKER_HERE_TIMEOUT_S'])

    def _call_here(self, url, params):
        log = logging.getLogger(__name__)
        log.debug('Here routing service, url: {}'.format(url))
        try:
            return self.breaker.call(requests.get, url, timeout=self.timeout, params=params)
        except pybreaker.CircuitBreakerError as e:
            log.error('Valhalla routing service dead (error: {})'.format(e))
            self.record_external_failure('circuit breaker open')
            raise TechnicalError('HERE service not available')
        except requests.Timeout as t:
            log.error('Valhalla routing service dead (error: {})'.format(t))
            self.record_external_failure('timeout')
            raise TechnicalError('impossible to access HERE service, timeout reached')
        except Exception as e:
            log.exception('Valhalla routing error')
            self.record_external_failure(str(e))
            raise TechnicalError('impossible to access HERE service')

    @staticmethod
    def _read_response(response, origin, destination, mode, fallback_extremity, request):
        resp = response_pb2.Response()
        resp.status_code = 200
        routes = response.get('response', {}).get('route', [])
        if not routes:
            resp.response_type = response_pb2.NO_SOLUTION
            return resp

        resp.response_type = response_pb2.ITINERARY_FOUND
        route = routes[0]

        journey = resp.journeys.add()
        journey.duration = route.get('summary', {}).get('travelTime')

        datetime, represents_start_fallback = fallback_extremity
        if represents_start_fallback:
            journey.departure_date_time = datetime
            journey.arrival_date_time = datetime + journey.duration
        else:
            journey.departure_date_time = datetime - journey.duration
            journey.arrival_date_time = datetime

        journey.requested_date_time = request['datetime']
        journey.durations.total = journey.duration

        if mode == 'walking':
            journey.durations.walking = journey.duration

        legs = route.get('leg')
        if not legs:
            return journey

        leg = legs[0]

        section = journey.sections.add()
        section.type = response_pb2.STREET_NETWORK

        section.duration = leg.get('travelTime', 0)
        section.begin_date_time = journey.departure_date_time
        section.end_date_time = section.begin_date_time + section.duration

        section.id = 'section_0'
        section.length = int(leg.get('length', 0))

        section.origin.CopyFrom(origin)
        section.destination.CopyFrom(destination)

        section.street_network.length = section.length
        section.street_network.duration = section.duration
        map_mode = {
            "walking": response_pb2.Walking,
            "car": response_pb2.Car,
            "bike": response_pb2.Bike
        }
        section.street_network.mode = map_mode[mode]
        for maneuver in leg.get('maneuver', []):
            path_item = section.street_network.path_items.add()
            # TODO get the street name
            path_item.length = maneuver.get('length')
            path_item.duration = maneuver.get('travelTime')
            # TODO: calculate direction
            path_item.direction = 0

        shape = route.get('shape', [])
        for sh in shape:
            coord = section.street_network.coordinates.add()
            lat, lon = sh.split(',')
            coord.lon = float(lon)
            coord.lat = float(lat)

        return resp

    def get_direct_path_params(self, origin, destination, mode, fallback_extremity):
        params = {
            # those are used to identify in the API
            'app_id': self.api_id,
            'app_code': self.api_code,
            'waypoint0': _get_coord(origin),
            'waypoint1': _get_coord(destination),
            'routeAttributes': 'sh',  # to get the shape in the response
            # used to get the travel time in the response
            'summaryAttributes': 'traveltime',
            # used to get the fasted journeys using the given mode and with traffic data
            'mode': 'fastest;{mode};traffic:enabled'.format(mode=get_here_mode(mode)),
        }
        datetime, represents_start_fallback = fallback_extremity
        if represents_start_fallback:
            params['departure'] = _str_to_dt(datetime)
        else:
            params['arrival'] = _str_to_dt(datetime)

        return params

    def direct_path(self, mode, pt_object_origin, pt_object_destination, fallback_extremity, request, direct_path_type):
        params = self.get_direct_path_params(pt_object_origin, pt_object_destination, mode, fallback_extremity)
        r = self._call_here(self.routing_service_url, params=params)
        if r.status_code != 200:
            l = logging.getLogger(__name__)
            l.debug('impossible to find a path between {o} and {d}, response ({code}) {r}'
                   .format(o=pt_object_origin, d=pt_object_destination, code=r.status_code, r=r.text))
            resp = response_pb2.Response()
            resp.status_code = 200
            resp.response_type = response_pb2.NO_SOLUTION
            return resp

        json = r.json()

        logging.getLogger(__name__).debug('here response = {}'.format(json))

        return self._read_response(json, pt_object_origin, pt_object_destination, mode,
                                   fallback_extremity, request)

    @classmethod
    def _get_matrix(cls, json_response, origins, destinations):
        sn_routing_matrix = response_pb2.StreetNetworkRoutingMatrix()
        # by convenience we create a temporary structure. If it's a bottleneck, refactor this
        entries = {
            (e.get('startIndex'), e.get('destinationIndex')): e.get('summary', {}).get('travelTime')
            for e in json_response.get('response', {}).get('matrixEntry', [])
        }
        for i_o, o in enumerate(origins):
            row = sn_routing_matrix.rows.add()
            for i_d, d in enumerate(destinations):
                routing = row.routing_response.add()
                travel_time = entries.get((i_o, i_d))
                if travel_time:
                    routing.duration = travel_time
                    routing.routing_status = response_pb2.reached
                else:
                    # since we limit the number of points given to HERE, we say that all the others cannot be
                    # reached (and not response_pb2.unkown since we don't want a crow fly section)
                    routing.duration = -1
                    routing.routing_status = response_pb2.unreached

        return sn_routing_matrix

    def get_matrix_params(self, origins, destinations, mode, max_duration, request):
        params = {
            # those are used to identify in the API
            'app_id': self.api_id,
            'app_code': self.api_code,
            # used to get the travel time in the response
            'summaryAttributes': 'traveltime',
            # used to get the fasted journeys using the given mode and with traffic data
            'mode': 'fastest;{mode};traffic:enabled'.format(mode=get_here_mode(mode)),
        }

        # for the ending fallback matrix (or the beginning of non clockwise query), we do not know the
        # precise departure/arrival (as it depend on the public transport taken)
        # for the moment we only give the departure time of the matrix as the requested departure
        # (so it's a mistake in 50% of the cases).
        # TODO: do better :D
        datetime = request['datetime']
        params['departure'] = _str_to_dt(datetime)

        # Note: for the moment we limit to self.max_points the number of n:m asked
        for i, o in enumerate(origins[:self.max_points]):
            params['start{}'.format(i)] = _get_coord(o)

        for i, o in enumerate(destinations[:self.max_points]):
            params['destination{}'.format(i)] = _get_coord(o)

        # TODO handle max_duration (but the API can only handle a max distance (searchRange)

        return params

    def get_street_network_routing_matrix(self, origins, destinations, mode, max_duration, request, **kwargs):
        params = self.get_matrix_params(origins, destinations, mode, max_duration, request)
        r = self._call_here(self.matrix_service_url, params=params)

        if r.status_code != 200:
            logging.getLogger(__name__).error('impossible to query HERE: {} with {} response: {}'
                                              .format(r.url, r.status_code, r.text))
            raise TechnicalError('invalid HERE call, impossible to query: {}'.format(r.url))

        resp_json = r.json()

        return self._get_matrix(resp_json, origins, destinations)

    def make_path_key(self, mode, orig_uri, dest_uri, streetnetwork_path_type, period_extremity):
        """
        :param orig_uri, dest_uri, mode: matters obviously
        :param streetnetwork_path_type: whether it's a fallback at
        the beginning, the end of journey or a direct path without PT also matters especially for car (to know if we
        park before or after)
        :param period_extremity: is a PeriodExtremity (a datetime and it's meaning on the
        fallback period)
        Nota: period_extremity is taken into consideration so far because we assume that a
        direct path from A to B change change with the realtime
        """
        return StreetNetworkPathKey(mode, orig_uri, dest_uri, streetnetwork_path_type, period_extremity)

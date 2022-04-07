#  Copyright (c) 2001-2016, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.hove.com).
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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
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
from jormungandr.utils import is_url, kilometers_to_meters, get_pt_object_coord, decode_polyline
from copy import deepcopy
from jormungandr.street_network.street_network import (
    AbstractStreetNetworkService,
    StreetNetworkPathKey,
    StreetNetworkPathType,
)
import six


def fill_park_section(section, point, type, begin_dt, duration):
    section.type = type
    section.duration = duration
    section.begin_date_time = begin_dt
    section.end_date_time = begin_dt + duration

    section.origin.CopyFrom(point)
    section.destination.CopyFrom(point)


class Valhalla(AbstractStreetNetworkService):
    def __init__(self, instance, service_url, modes=[], id='valhalla', timeout=10, api_key=None, **kwargs):
        self.instance = instance
        self.sn_system_id = id
        if not is_url(service_url):
            raise ValueError('service_url {} is not a valid url'.format(service_url))
        self.service_url = service_url
        self.api_key = api_key
        self.timeout = timeout
        self.modes = modes
        self.costing_options = kwargs.get('costing_options', None)
        # kilometres is default units
        self.directions_options = {'units': 'kilometers'}
        self.breaker = pybreaker.CircuitBreaker(
            fail_max=app.config['CIRCUIT_BREAKER_MAX_VALHALLA_FAIL'],
            reset_timeout=app.config['CIRCUIT_BREAKER_VALHALLA_TIMEOUT_S'],
        )
        # the mode's park cost represent the initial cost to leave the given mode
        # it is used to represent that it takes time to park a car or a bike
        # since valhalla does not handle such a time, we rig valhalla's result to add it
        self.mode_park_cost = kwargs.get('mode_park_cost', {})  # a dict giving the park time (in s) by mode

    def status(self):
        return {
            'id': six.text_type(self.sn_system_id),
            'class': self.__class__.__name__,
            'modes': self.modes,
            'timeout': self.timeout,
            'circuit_breaker': {
                'current_state': self.breaker.current_state,
                'fail_counter': self.breaker.fail_counter,
                'reset_timeout': self.breaker.reset_timeout,
            },
        }

    def _call_valhalla(self, url, method=requests.post, data=None):
        logging.getLogger(__name__).debug('Valhalla routing service , call url : {}'.format(url))
        logging.getLogger(__name__).debug('data : {}'.format(data))
        headers = {}
        if self.api_key:
            headers['api_key'] = self.api_key
        try:
            return self.breaker.call(method, url, timeout=self.timeout, data=data, headers=headers)
        except pybreaker.CircuitBreakerError as e:
            logging.getLogger(__name__).error('Valhalla routing service dead (error: {})'.format(e))
            self.record_external_failure('circuit breaker open')
        except requests.Timeout as t:
            logging.getLogger(__name__).error('Valhalla routing service dead (error: {})'.format(t))
            self.record_external_failure('timeout')
        except Exception as e:
            logging.getLogger(__name__).exception('Valhalla routing error')
            self.record_external_failure(str(e))
        return None

    @classmethod
    def _format_coord(cls, pt_object, api='route'):
        if api not in ['route', 'sources_to_targets']:
            logging.getLogger(__name__).error('Valhalla routing service , invalid api {}'.format(api))
            raise ApiNotFound('Valhalla routing service , invalid api {}'.format(api))

        coord = get_pt_object_coord(pt_object)
        dict_coord = {"lat": coord.lat, "lon": coord.lon}
        if api == 'route':
            dict_coord["type"] = "break"
        return dict_coord

    def _get_costing_options(self, valhalla_mode, request):
        costing_options = deepcopy(self.costing_options)
        if valhalla_mode not in ['pedestrian', 'bicycle']:
            return costing_options
        if not costing_options:
            costing_options = {}
        if valhalla_mode == 'pedestrian':
            if 'pedestrian' not in costing_options:
                costing_options['pedestrian'] = {}
            costing_options['pedestrian']['walking_speed'] = request['walking_speed'] * 3.6
        elif valhalla_mode == 'bicycle':
            if 'bicycle' not in costing_options:
                costing_options['bicycle'] = {}
            costing_options['bicycle']['cycling_speed'] = request['bike_speed'] * 3.6
        return costing_options

    @classmethod
    def _get_response(
        cls,
        json_resp,
        mode,
        pt_object_origin,
        pt_object_destination,
        fallback_extremity,
        direct_path_type,
        mode_park_cost=None,
    ):
        """
        :param fallback_extremity: is a PeriodExtremity (a datetime and it's meaning on the fallback period)
        """
        map_mode = {
            "walking": response_pb2.Walking,
            "car": response_pb2.Car,
            "bike": response_pb2.Bike,
            "car_no_park": response_pb2.Car,
        }
        resp = response_pb2.Response()
        resp.status_code = 200
        resp.response_type = response_pb2.ITINERARY_FOUND

        journey = resp.journeys.add()
        journey.duration = json_resp['trip']['summary']['time'] + (mode_park_cost or 0)
        datetime, represents_start_fallback = fallback_extremity
        offset = 0  # offset if there is a leaving park section
        if represents_start_fallback:
            journey.departure_date_time = datetime
            journey.arrival_date_time = datetime + journey.duration
        else:
            journey.departure_date_time = datetime - journey.duration
            journey.arrival_date_time = datetime

        beginning_fallback = (
            direct_path_type == StreetNetworkPathType.BEGINNING_FALLBACK
            or direct_path_type == StreetNetworkPathType.DIRECT
        )
        if not beginning_fallback and mode_park_cost is not None:
            # we also add a LEAVE_PARKING section if needed
            fill_park_section(
                section=journey.sections.add(),
                point=pt_object_origin,
                type=response_pb2.LEAVE_PARKING,
                begin_dt=journey.departure_date_time,
                duration=mode_park_cost,
            )
            offset = mode_park_cost

        journey.durations.total = journey.duration

        if mode == 'walking':
            journey.durations.walking = journey.duration

        previous_section_endtime = journey.departure_date_time + offset
        for index, leg in enumerate(json_resp['trip']['legs']):
            section = journey.sections.add()
            section.type = response_pb2.STREET_NETWORK

            section.duration = leg['summary']['time']
            section.begin_date_time = previous_section_endtime
            section.end_date_time = section.begin_date_time + section.duration
            previous_section_endtime = section.end_date_time

            section.id = 'section_{}'.format(index)
            section.length = int(kilometers_to_meters(leg['summary']['length']))

            if index == 0:
                section.origin.CopyFrom(pt_object_origin)
            if index == len(json_resp['trip']['legs']) - 1:
                section.destination.CopyFrom(pt_object_destination)

            section.street_network.length = section.length
            section.street_network.duration = section.duration
            section.street_network.mode = map_mode[mode]
            for maneuver in leg['maneuvers']:
                path_item = section.street_network.path_items.add()
                if 'street_names' in maneuver and len(maneuver['street_names']) > 0:
                    path_item.name = maneuver['street_names'][0]
                path_item.length = kilometers_to_meters(maneuver['length'])
                path_item.duration = maneuver['time']
                # TODO: calculate direction
                path_item.direction = 0

            shape = decode_polyline(leg['shape'])
            for sh in shape:
                coord = section.street_network.coordinates.add()
                coord.lon = sh[0]
                coord.lat = sh[1]

        if beginning_fallback and mode_park_cost is not None:
            fill_park_section(
                section=journey.sections.add(),
                point=pt_object_destination,
                type=response_pb2.PARK,
                begin_dt=previous_section_endtime,
                duration=mode_park_cost,
            )

        return resp

    @classmethod
    def _get_valhalla_mode(cls, kraken_mode):
        map_mode = {"walking": "pedestrian", "car": "auto", "car_no_park": "auto", "bike": "bicycle"}
        if kraken_mode not in map_mode:
            logging.getLogger(__name__).error('Valhalla, mode {} not implemented'.format(kraken_mode))
            raise InvalidArguments('Valhalla, mode {} not implemented'.format(kraken_mode))
        return map_mode.get(kraken_mode)

    def _make_request_arguments(
        self, mode, pt_object_origins, pt_object_destinations, request, api='route', max_duration=None
    ):

        args = {}
        valhalla_mode = self._get_valhalla_mode(mode)
        args['costing'] = valhalla_mode

        costing_options = self._get_costing_options(valhalla_mode, request)
        if costing_options:
            args['costing_options'] = costing_options

        if api == 'route':
            args['directions_options'] = self.directions_options
            args['locations'] = [self._format_coord(pt_object_origins[0])] + [
                self._format_coord(pt_object_destinations[0])
            ]
        if api == 'sources_to_targets':
            args['sources'] = [self._format_coord(origin, api) for origin in pt_object_origins]
            args['targets'] = [self._format_coord(destination, api) for destination in pt_object_destinations]

            for key, value in self.directions_options.items():
                args[key] = value
        return json.dumps(args)

    @classmethod
    def _check_response(cls, response):
        if response == None:
            raise TechnicalError('impossible to access valhalla service')
        if response.status_code != 200:
            logging.getLogger(__name__).error(
                'Valhalla service unavailable, impossible to query : {}'
                ' with response : {}'.format(response.url, response.text)
            )
            raise TechnicalError('Valhalla service unavailable, impossible to query : {}'.format(response.url))

    def _direct_path(
        self,
        instance,
        mode,
        pt_object_origin,
        pt_object_destination,
        fallback_extremity,
        request,
        direct_path_type,
        request_id,
    ):
        data = self._make_request_arguments(
            mode, [pt_object_origin], [pt_object_destination], request, api='route'
        )
        r = self._call_valhalla('{}/{}'.format(self.service_url, 'route'), requests.post, data)
        if r is not None and r.status_code == 400 and r.json()['error_code'] == 442:
            # error_code == 442 => No path could be found for input
            resp = response_pb2.Response()
            resp.status_code = 200
            resp.response_type = response_pb2.NO_SOLUTION
            return resp
        self._check_response(r)
        resp_json = r.json()
        return self._get_response(
            resp_json,
            mode,
            pt_object_origin,
            pt_object_destination,
            fallback_extremity,
            direct_path_type,
            mode_park_cost=self.mode_park_cost.get(mode),
        )

    @classmethod
    def _get_matrix(cls, json_response, mode_park_cost):
        sn_routing_matrix = response_pb2.StreetNetworkRoutingMatrix()
        # kraken dosn't handle n-m, we can't have more than one row as they will be ignored
        row = sn_routing_matrix.rows.add()
        for source_to_target in json_response['sources_to_targets']:
            for one in source_to_target:
                routing = row.routing_response.add()
                if one['time']:
                    # the mode's base cost represent the initial cost to take the given mode
                    # it is used to represent that it takes time to park a car or a bike
                    # since valhalla does not handle such a time, we tweak valhalla's result to add it
                    routing.duration = one['time'] + (mode_park_cost or 0)
                    routing.routing_status = response_pb2.reached
                else:
                    routing.duration = -1
                    routing.routing_status = response_pb2.unknown
        return sn_routing_matrix

    def _get_street_network_routing_matrix(
        self, instance, origins, destinations, mode, max_duration, request, request_id, **kwargs
    ):
        # for now valhalla only manages 1-n request, so we reverse request if needed
        if len(origins) > 1:
            if len(destinations) > 1:
                logging.getLogger(__name__).error('routing matrix error, no unique center point')
                raise TechnicalError('routing matrix error, no unique center point')

        data = self._make_request_arguments(mode, origins, destinations, request, api='sources_to_targets')
        r = self._call_valhalla('{}/{}'.format(self.service_url, 'sources_to_targets'), requests.post, data)
        self._check_response(r)
        resp_json = r.json()
        return self._get_matrix(resp_json, mode_park_cost=self.mode_park_cost.get(mode))

    def make_path_key(self, mode, orig_uri, dest_uri, streetnetwork_path_type, period_extremity):
        """
        :param orig_uri, dest_uri, mode: matters obviously
        :param streetnetwork_path_type: whether it's a fallback at
        the beginning, the end of journey or a direct path without PT also matters especially for car (to know if we
        park before or after)
        :param period_extremity: is a PeriodExtremity (a datetime and it's meaning on the
        fallback period)
        Nota: period_extremity is not taken into consideration so far because we assume that a
        direct path from A to B remains the same even the departure time are different (no realtime)
        """
        return StreetNetworkPathKey(mode, orig_uri, dest_uri, streetnetwork_path_type, None)

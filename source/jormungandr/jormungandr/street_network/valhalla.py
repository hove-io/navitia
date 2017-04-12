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
from jormungandr.utils import is_url, kilometers_to_meters, get_pt_object_coord, decode_polyline
from copy import deepcopy
from jormungandr.street_network.street_network import AbstractStreetNetworkService, StreetNetworkPathKey


class Valhalla(AbstractStreetNetworkService):

    def __init__(self, instance, service_url, id='valhalla', timeout=10, api_key=None, **kwargs):
        self.instance = instance
        self.sn_system_id = id
        if not is_url(service_url):
            raise ValueError('service_url {} is not a valid url'.format(service_url))
        self.service_url = service_url
        self.api_key = api_key
        self.timeout = timeout
        self.costing_options = kwargs.get('costing_options', None)
        # kilometres is default units
        self.directions_options = {
            'units': 'kilometers'
        }
        self.breaker = pybreaker.CircuitBreaker(fail_max=app.config['CIRCUIT_BREAKER_MAX_VALHALLA_FAIL'],
                                                reset_timeout=app.config['CIRCUIT_BREAKER_VALHALLA_TIMEOUT_S'])

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
        if api not in ['route', 'one_to_many']:
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
    def _get_response(cls, json_resp, mode, pt_object_origin, pt_object_destination, fallback_extremity):
        '''
        :param fallback_extremity: is a PeriodExtremity (a datetime and it's meaning on the fallback period)
        '''
        map_mode = {
            "walking": response_pb2.Walking,
            "car": response_pb2.Car,
            "bike": response_pb2.Bike
        }
        resp = response_pb2.Response()
        resp.status_code = 200
        resp.response_type = response_pb2.ITINERARY_FOUND

        journey = resp.journeys.add()
        journey.duration = json_resp['trip']['summary']['time']
        datetime, represents_start_fallback = fallback_extremity
        if represents_start_fallback:
            journey.departure_date_time = datetime
            journey.arrival_date_time = datetime + journey.duration
        else:
            journey.departure_date_time = datetime - journey.duration
            journey.arrival_date_time = datetime

        journey.durations.total = journey.duration

        if mode == 'walking':
            journey.durations.walking = journey.duration

        previous_section_endtime = journey.departure_date_time
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

        return resp

    @classmethod
    def _get_valhalla_mode(cls, kraken_mode):
        map_mode = {
            "walking": "pedestrian",
            "car": "auto",
            "bike": "bicycle"
        }
        if kraken_mode not in map_mode:
            logging.getLogger(__name__).error('Valhalla, mode {} not implemented'.format(kraken_mode))
            raise InvalidArguments('Valhalla, mode {} not implemented'.format(kraken_mode))
        return map_mode.get(kraken_mode)

    def _make_request_arguments(self, mode, pt_object_origin, pt_object_destinations, request, api='route', max_duration=None):

        valhalla_mode = self._get_valhalla_mode(mode)
        destinations = [self._format_coord(destination, api) for destination in pt_object_destinations]
        args = {
            'locations': [self._format_coord(pt_object_origin)] + destinations,
            'costing': valhalla_mode
        }

        costing_options = self._get_costing_options(valhalla_mode, request)
        if costing_options:
            args['costing_options'] = costing_options

        if api == 'route':
            args['directions_options'] = self.directions_options
        if api == 'one_to_many':
            for key, value in self.directions_options.items():
                args[key] = value
        return json.dumps(args)

    @classmethod
    def _check_response(cls, response):
        if response == None:
            raise TechnicalError('impossible to access valhalla service')
        if response.status_code != 200:
            logging.getLogger(__name__).error('Valhalla service unavailable, impossible to query : {}'
                                              ' with response : {}'
                                              .format(response.url, response.text))
            raise TechnicalError('Valhalla service unavailable, impossible to query : {}'.
                                 format(response.url))

    def direct_path(self, mode, pt_object_origin, pt_object_destination, fallback_extremity, request, direct_path_type):
        data = self._make_request_arguments(mode, pt_object_origin, [pt_object_destination], request, api='route')
        r = self._call_valhalla('{}/{}'.format(self.service_url, 'route'), requests.post, data)
        if r is not None and r.status_code == 400 and r.json()['error_code'] == 442:
            # error_code == 442 => No path could be found for input
            resp = response_pb2.Response()
            resp.status_code = 200
            resp.response_type = response_pb2.NO_SOLUTION
            return resp
        self._check_response(r)
        resp_json = r.json()
        return self._get_response(resp_json, mode, pt_object_origin, pt_object_destination, fallback_extremity)

    @classmethod
    def _get_matrix(cls, json_response):
        sn_routing_matrix = response_pb2.StreetNetworkRoutingMatrix()
        for one_to_many in json_response['one_to_many']:
            row = sn_routing_matrix.rows.add()
            for one in one_to_many[1:]:
                routing = row.routing_response.add()
                if one['time']:
                    routing.duration = one['time']
                    routing.routing_status = response_pb2.reached
                else:
                    routing.duration = -1
                    routing.routing_status = response_pb2.unknown
        return sn_routing_matrix

    def get_street_network_routing_matrix(self, origins, destinations, mode, max_duration, request, **kwargs):

        #for now valhalla only manages 1-n request, so we reverse request if needed
        if len(origins) > 1:
            if len(destinations) > 1:
                logging.getLogger(__name__).error('routing matrix error, no unique center point')
                raise TechnicalError('routing matrix error, no unique center point')
            else:
                origins, destinations = destinations, origins

        data = self._make_request_arguments(mode, origins[0], destinations, request, api='one_to_many')
        r = self._call_valhalla('{}/{}'.format(self.service_url, 'one_to_many'), requests.post, data)
        self._check_response(r)
        resp_json = r.json()
        return self._get_matrix(resp_json)

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

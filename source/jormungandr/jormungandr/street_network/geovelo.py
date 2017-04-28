#  Copyright (c) 2001-2017, Canal TP and/or its affiliates. All rights reserved.
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
import requests as requests
import pybreaker
import json
from navitiacommon import response_pb2
from jormungandr import app
from jormungandr.exceptions import TechnicalError, InvalidArguments, UnableToParse
from jormungandr.street_network.street_network import AbstractStreetNetworkService, StreetNetworkPathKey
from jormungandr.utils import get_pt_object_coord, is_url, decode_polyline


class Geovelo(AbstractStreetNetworkService):

    def __init__(self, instance, service_url, id='geovelo', timeout=10, api_key=None, **kwargs):
        self.instance = instance
        self.sn_system_id = id
        if not is_url(service_url):
            raise ValueError('service_url {} is not a valid url'.format(service_url))
        self.service_url = service_url
        self.api_key = api_key
        self.timeout = timeout
        self.breaker = pybreaker.CircuitBreaker(fail_max=app.config['CIRCUIT_BREAKER_MAX_GEOVELO_FAIL'],
                                                reset_timeout=app.config['CIRCUIT_BREAKER_GEOVELO_TIMEOUT_S'])

    @classmethod
    def _pt_object_summary_isochrone(cls, pt_object):
        coord = get_pt_object_coord(pt_object)
        return [coord.lat, coord.lon, getattr(pt_object, 'uri', None)]

    @classmethod
    def _make_request_arguments_isochrone(cls, origins, destinations):
        return {
            'starts': [cls._pt_object_summary_isochrone(o) for o in origins],
            'ends': [cls._pt_object_summary_isochrone(o) for o in destinations],
        }

    @classmethod
    def _make_request_arguments_direct_path(cls, origin, destination):
        coord_orig = get_pt_object_coord(origin)
        coord_dest = get_pt_object_coord(destination)
        return {
            'waypoints': [
                {'latitude': coord_orig.lat, 'longitude': coord_orig.lon},
                {'latitude': coord_dest.lat, 'longitude': coord_dest.lon}
            ],
            'bikeDetails': {
                'profile': 'MEDIAN', # can be BEGINNER, EXPERT
                'bikeType': 'TRADITIONAL', # can be 'BSS'
                # 'averageSpeed': '6' # in km/h, BEGINNER sets it to 13
            },
            'transportModes': ['BIKE']
        }

    def _call_geovelo(self, url, method=requests.post, data=None):
        logging.getLogger(__name__).debug('Geovelo routing service , call url : {}'.format(url))
        try:
            return self.breaker.call(method, url, timeout=self.timeout, data=data,
                                     headers={'content-type': 'application/json',
                                              'Api-Key': self.api_key})
        except pybreaker.CircuitBreakerError as e:
            logging.getLogger(__name__).error('Geovelo routing service dead (error: {})'.format(e))
            self.record_external_failure('circuit breaker open')
        except requests.Timeout as t:
            logging.getLogger(__name__).error('Geovelo routing service dead (error: {})'.format(t))
            self.record_external_failure('timeout')
        except Exception as e:
            logging.getLogger(__name__).exception('Geovelo routing error')
            self.record_external_failure(str(e))
        return None

    @classmethod
    def _get_matrix(cls, json_response):
        '''
        build the 1-n response matrix from geovelo table
        each element is ["start", "end", duration] (first being the header)
        as it is ordered, we only consider the third
        '''
        sn_routing_matrix = response_pb2.StreetNetworkRoutingMatrix()
        row = sn_routing_matrix.rows.add()
        #checking header of geovelo's response
        if json_response[0] != ["start_reference", "end_reference", "duration"]:
            logging.getLogger(__name__).error('Geovelo parsing error. Response: {}'.format(json_response))
            raise UnableToParse('Geovelo parsing error. Response: {}'.format(json_response))
        for e in json_response[1:]:
            routing = row.routing_response.add()
            if e[2]:
                routing.duration = e[2]
                routing.routing_status = response_pb2.reached
            else:
                routing.duration = -1
                routing.routing_status = response_pb2.unknown
        return sn_routing_matrix

    @classmethod
    def _check_response(cls, response):
        if response is None:
            raise TechnicalError('impossible to access geovelo service')
        if response.status_code != 200:
            logging.getLogger(__name__).error('Geovelo service unavailable, impossible to query : {}'
                                              ' with response : {}'
                                              .format(response.url, response.text))
            raise TechnicalError('Geovelo service unavailable, impossible to query : {}'.
                                 format(response.url))

    def get_street_network_routing_matrix(self, origins, destinations, street_network_mode, max_duration, request, **kwargs):
        if street_network_mode != "bike":
            logging.getLogger(__name__).error('Geovelo, mode {} not implemented'.format(street_network_mode))
            raise InvalidArguments('Geovelo, mode {} not implemented'.format(street_network_mode))
        if len(origins) != 1 and len(destinations) != 1:
            logging.getLogger(__name__).error('Geovelo, managing only 1-n in connector, requested {}-{}'
                                              .format(len(origins), len(destinations)))
            raise InvalidArguments('Geovelo, managing only 1-n in connector, requested {}-{}'
                                   .format(len(origins), len(destinations)))

        data = self._make_request_arguments_isochrone(origins, destinations)
        r = self._call_geovelo('{}/{}'.format(self.service_url, 'api/v2/routes_m2m'),
                               requests.post, json.dumps(data))
        self._check_response(r)
        resp_json = r.json()

        if len(resp_json) - 1 != len(origins) * len(destinations):
            logging.getLogger(__name__).error('Geovelo nb response != nb requested')
            raise UnableToParse('Geovelo nb response != nb requested')

        return self._get_matrix(resp_json)


    @classmethod
    def _get_response(cls, json_response, mode, pt_object_origin, pt_object_destination, fallback_extremity):
        '''
        :param fallback_extremity: is a PeriodExtremity (a datetime and it's meaning on the fallback period)
        '''

        # map giving the change of direction (degrees) from geovelo indications
        # see: http://developers.geovelo.fr/#/documentation/compute
        map_instructions_direction = {
            'HEAD_ON': 0,
            'GO_STRAIGHT': 0,
            'REACHED_YOUR_DESTINATION': 0,
            'REACH_VIA_LOCATION': 0,
            'ELEVATOR': 0,
            'CROSSING': 0,
            'ENTER_ROUND_ABOUT': 90,
            'LEAVE_ROUND_ABOUT': 90,
            'STAY_ON_ROUND_ABOUT': 0,
            'ENTER_AGAINST_ALLOWED_DIRECTION': 0,
            'LEAVE_AGAINST_ALLOWED_DIRECTION': 0,
            'TURN_SLIGHT_RIGHT': 45,
            'TURN_SLIGHT_LEFT': -45,
            'TURN_RIGHT': 90,
            'TURN_LEFT': -90,
            'TURN_SHARP_RIGHT': 135,
            'TURN_SHARP_LEFT': -135,
            'U-TURN': 180
        }

        resp = response_pb2.Response()
        resp.status_code = 200
        resp.response_type = response_pb2.ITINERARY_FOUND

        geovelo_resp = json_response[0]

        journey = resp.journeys.add()
        journey.duration = geovelo_resp['duration']
        datetime, represents_start_fallback = fallback_extremity
        if represents_start_fallback:
            journey.departure_date_time = datetime
            journey.arrival_date_time = datetime + journey.duration
        else:
            journey.departure_date_time = datetime - journey.duration
            journey.arrival_date_time = datetime

        journey.durations.total = journey.duration

        previous_section_endtime = journey.departure_date_time
        for index, geovelo_section in enumerate(geovelo_resp['sections']):
            section = journey.sections.add()
            section.type = response_pb2.STREET_NETWORK

            section.duration = geovelo_section['duration']
            section.begin_date_time = previous_section_endtime
            section.end_date_time = section.begin_date_time + section.duration
            previous_section_endtime = section.end_date_time

            section.id = 'section_{}'.format(index)
            section.length = int(geovelo_section['details']['distances']['total'])

            if index == 0:
                section.origin.CopyFrom(pt_object_origin)
            if index == len(geovelo_resp['sections']) - 1:
                section.destination.CopyFrom(pt_object_destination)

            section.street_network.duration = section.duration
            section.street_network.length = section.length
            section.street_network.mode = response_pb2.Bike

            speed = section.length / section.duration if section.duration != 0 else 0

            for geovelo_instruction in geovelo_section['details']['instructions'][1:]:
                path_item = section.street_network.path_items.add()
                path_item.name = geovelo_instruction[1]
                path_item.length = geovelo_instruction[2]
                path_item.duration = round(path_item.length * speed)
                path_item.direction = map_instructions_direction.get(geovelo_instruction[0], 0)

            shape = decode_polyline(geovelo_resp['sections'][0]['geometry'])
            for sh in shape:
                coord = section.street_network.coordinates.add()
                coord.lon = sh[0]
                coord.lat = sh[1]

        return resp

    def direct_path(self, mode, pt_object_origin, pt_object_destination, fallback_extremity, request, direct_path_type):
        if mode != "bike":
            logging.getLogger(__name__).error('Geovelo, mode {} not implemented'.format(mode))
            raise InvalidArguments('Geovelo, mode {} not implemented'.format(mode))

        data = self._make_request_arguments_direct_path(pt_object_origin, pt_object_destination)
        r = self._call_geovelo('{}/{}'.format(self.service_url, 'api/v2/computedroutes?'
                                                                'instructions=true&'
                                                                'elevations=false&'
                                                                'geometry=true&'
                                                                'single_result=true&'
                                                                'bike_stations=false&'
                                                                'objects_as_ids=true&'),
                               requests.post, json.dumps(data))
        self._check_response(r)
        resp_json = r.json()

        if len(resp_json) != 1:
            logging.getLogger(__name__).error('Geovelo nb response != nb requested')
            raise UnableToParse('Geovelo nb response != nb requested')

        return self._get_response(resp_json, mode, pt_object_origin, pt_object_destination, fallback_extremity)

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
